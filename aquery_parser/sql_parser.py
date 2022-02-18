# encoding: utf-8
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Contact: Kyle Lahnakoski (kyle@lahnakoski.com)
#

from mo_parsing.helpers import restOfLine
from mo_parsing.infix import delimited_list
from mo_parsing.whitespaces import NO_WHITESPACE, Whitespace

from aquery_parser.keywords import *
from aquery_parser.types import get_column_type, time_functions
from aquery_parser.utils import *
from aquery_parser.windows import window

digit = Char("0123456789")
simple_ident = (
    Char(FIRST_IDENT_CHAR)
    + Char(IDENT_CHAR)[...] # let's not support dashes in var_names.
)
simple_ident = Regex(simple_ident.__regex__()[1]) 


def common_parser():
    combined_ident = Combine(delimited_list(
        ansi_ident | mysql_backtick_ident | simple_ident, separator=".", combine=True,
    )).set_parser_name("identifier")

    return parser(ansi_string | mysql_doublequote_string, combined_ident)


def mysql_parser():
    mysql_string = ansi_string | mysql_doublequote_string
    mysql_ident = Combine(delimited_list(
        mysql_backtick_ident | sqlserver_ident | simple_ident,
        separator=".",
        combine=True,
    )).set_parser_name("mysql identifier")

    return parser(mysql_string, mysql_ident)


def sqlserver_parser():
    combined_ident = Combine(delimited_list(
        ansi_ident
        | mysql_backtick_ident
        | sqlserver_ident
        | Word(FIRST_IDENT_CHAR, IDENT_CHAR),
        separator=".",
        combine=True,
    )).set_parser_name("identifier")

    return parser(ansi_string, combined_ident, sqlserver=True)


def parser(literal_string, ident, sqlserver=False):
    with Whitespace() as engine:
        engine.add_ignore(Literal("--") + restOfLine)
        engine.add_ignore(Literal("#") + restOfLine)
        engine.add_ignore(Literal("/*") + SkipTo("*/", include=True))

        var_name = ~RESERVED + ident
        
        inline_kblock = (L_INLINE + SkipTo(R_INLINE, include=True))("k9")
        # EXPRESSIONS
        expr = Forward()
        column_type, column_definition, column_def_references = get_column_type(
            expr, var_name, literal_string
        )

        # CASE
        case = (
            CASE
            + Group(ZeroOrMore(
                (WHEN + expr("when") + THEN + expr("then")) / to_when_call
            ))("case")
            + Optional(ELSE + expr("else"))
            + END
        ) / to_case_call

        switch = (
            CASE
            + expr("value")
            + Group(ZeroOrMore(
                (WHEN + expr("when") + THEN + expr("then")) / to_when_call
            ))("case")
            + Optional(ELSE + expr("else"))
            + END
        ) / to_switch_call

        cast = (
            Group(CAST("op") + LB + expr("params") + AS + column_type("params") + RB)
            / to_json_call
        )

        trim = (
            Group(
                keyword("trim").suppress()
                + LB
                + Optional(
                    (keyword("both") | keyword("trailing") | keyword("leading"))
                    / (lambda t: t[0].lower())
                )("direction")
                + (
                    assign("from", expr)
                    | expr("chars") + Optional(assign("from", expr))
                )
                + RB
            ).set_parser_name("trim")
            / to_trim_call
        )

        _standard_time_intervals = MatchFirst([
            keyword(d) / (lambda t: durations[t[0].lower()]) for d in durations.keys()
        ]).set_parser_name("duration")("params")

        duration = (
            real_num | int_num | literal_string
        )("params") + _standard_time_intervals

        interval = (
            INTERVAL + ("'" + delimited_list(duration) + "'" | duration)
        ) / to_interval_call

        timestamp = (
            time_functions("op")
            + (
                literal_string("params")
                | MatchFirst([
                    keyword(t) / (lambda t: t.lower()) for t in times
                ])("params")
            )
        ) / to_json_call

        extract = (
            keyword("extract")("op")
            + LB
            + (_standard_time_intervals | expr("params"))
            + FROM
            + expr("params")
            + RB
        ) / to_json_call

        alias = Optional((
            (
                AS
                + (var_name("name") + Optional(LB + delimited_list(ident("col")) + RB))
                | (
                    var_name("name")
                    + Optional(
                        (LB + delimited_list(ident("col")) + RB)
                        | (AS + delimited_list(var_name("col")))
                    )
                )
            )
            / to_alias
        )("name"))

        named_column = Group(Group(expr)("value") + alias)

        stack = (
            keyword("stack")("op")
            + LB
            + int_num("width")
            + ","
            + delimited_list(expr)("args")
            + RB
        ) / to_stack

        # ARRAY[foo],
        # ARRAY < STRING > [foo, bar], INVALID
        # ARRAY < STRING > [foo, bar],
        create_array = (
            keyword("array")("op")
            + Optional(LT.suppress() + column_type("type") + GT.suppress())
            + (
                LB + delimited_list(Group(expr))("args") + RB
                | (Literal("[") + delimited_list(Group(expr))("args") + Literal("]"))
            )
        )

        if not sqlserver:
            # SQL SERVER DOES NOT SUPPORT [] FOR ARRAY CONSTRUCTION (USED FOR IDENTIFIERS)
            create_array = (
                Literal("[") + delimited_list(Group(expr))("args") + Literal("]")
                | create_array
            )

        create_array = create_array / to_array

        create_map = (
            keyword("map")
            + Literal("[")
            + expr("keys")
            + ","
            + expr("values")
            + Literal("]")
        ) / to_map

        create_struct = (
            keyword("struct")("op")
            + Optional(
                LT.suppress() + delimited_list(column_type)("types") + GT.suppress()
            )
            + LB
            + delimited_list(Group((expr("value") + alias) / to_select_call))("args")
            + RB
        ).set_parser_name("create struct") / to_struct

        distinct = (
            DISTINCT("op") + delimited_list(named_column)("params")
        ) / to_json_call

        query = Forward().set_parser_name("query")

        call_function = (
            ident("op")
            + LB
            + Optional(Group(query) | delimited_list(Group(expr)))("params")
            + Optional(
                (keyword("respect") | keyword("ignore"))("nulls")
                + keyword("nulls").suppress()
            )
            + RB
        ).set_parser_name("call function") / to_json_call

        with NO_WHITESPACE:

            def scale(tokens):
                return {"mul": [tokens[0], tokens[1]]}

            scale_function = ((real_num | int_num) + call_function) / scale
            scale_ident = ((real_num | int_num) + ident) / scale

        compound = (
            NULL
            | TRUE
            | FALSE
            | NOCASE
            | interval
            | timestamp
            | extract
            | case
            | switch
            | cast
            | distinct
            | trim
            | stack
            | create_array
            | create_map
            | create_struct
            | (LB + Group(query) + RB)
            | (LB + Group(delimited_list(expr)) / to_tuple_call + RB)
            | literal_string.set_parser_name("string")
            | hex_num.set_parser_name("hex")
            | scale_function
            | scale_ident
            | real_num.set_parser_name("float")
            | int_num.set_parser_name("int")
            | call_function
            | Combine(var_name + Optional(".*"))
        )

        sort_column = (
            expr("value").set_parser_name("sort1")
            + Optional(DESC("sort") | ASC("sort"))
            + Optional(assign("nulls", keyword("first") | keyword("last")))
        )

        window_clause, over_clause = window(expr, var_name, sort_column)

        expr << (
            (
                Literal("*")
                | infix_notation(
                    compound,
                    [
                        (
                            Literal("[").suppress() + expr + Literal("]").suppress(),
                            1,
                            LEFT_ASSOC,
                            to_offset,
                        ),
                        (
                            Literal(".").suppress() + simple_ident,
                            1,
                            LEFT_ASSOC,
                            to_offset,
                        ),
                        (window_clause, 1, LEFT_ASSOC, to_window_mod),
                        (
                            assign("filter", LB + WHERE + expr + RB),
                            1,
                            LEFT_ASSOC,
                            to_window_mod,
                        ),
                    ]
                    + [
                        (
                            o,
                            1 if o in unary_ops else (3 if isinstance(o, tuple) else 2),
                            unary_ops.get(o, LEFT_ASSOC),
                            to_lambda if o is LAMBDA else to_json_operator,
                        )
                        for o in KNOWN_OPS
                    ],
                )
            )("value").set_parser_name("expression")
        )

        select_column = (
            Group(
                expr("value") + alias | Literal("*")("value")
            ).set_parser_name("column")
            / to_select_call
        )

        table_source = Forward()

        join = (
            Group(joins)("op")
            + table_source("join")
            + Optional((ON + expr("on")) | (USING + expr("using")))
            | (
                Group(WINDOW)("op")
                + Group(var_name("name") + AS + over_clause("value"))("join")
            )
        ) / to_join_call
        
        fassign = Group(var_name("var") + Suppress(FASSIGN) + expr("expr") + Suppress(";"))("assignment")
        fassigns = fassign + ZeroOrMore(fassign, Whitespace(white=" \t"))

        fbody = (Optional(fassigns) + expr("ret"))

        udf = (
            FUNCTION 
            + var_name("fname") 
            + LB 
            + Optional(delimited_list(var_name)("params")) 
            + RB 
            + LBRACE
            + fbody
            + RBRACE
        )

        selection = (
            (SELECT + DISTINCT + ON + LB)
            + delimited_list(select_column)("distinct_on")
            + RB
            + delimited_list(select_column)("select")
            | SELECT + DISTINCT + delimited_list(select_column)("select_distinct")
            | (
                SELECT
                + Optional(
                    TOP
                    + expr("value")
                    + Optional(keyword("percent"))("percent")
                    + Optional(WITH + keyword("ties"))("ties")
                )("top")
                / to_top_clause
                + delimited_list(select_column)("select")
            )
        )

        row = (LB + delimited_list(Group(expr)) + RB) / to_row
        values = VALUES + delimited_list(row) / to_values

        unordered_sql = Group(
            values
            | selection
            + Optional(
                (FROM + delimited_list(table_source) + ZeroOrMore(join))("from")
                + Optional(WHERE + expr("where"))
                + Optional(GROUP_BY + delimited_list(Group(named_column))("groupby"))
                + Optional(HAVING + expr("having"))
            )
        ).set_parser_name("unordered sql")

        with NO_WHITESPACE:

            def mult(tokens):
                amount = tokens["bytes"]
                scale = tokens["scale"].lower()
                return {
                    "bytes": amount
                    * {"b": 1, "k": 1_000, "m": 1_000_000, "g": 1_000_000_000}[scale]
                }

            ts_bytes = (
                (real_num | int_num)("bytes") + Char("bBkKmMgG")("scale")
            ) / mult

        tablesample = assign(
            "tablesample",
            LB
            + (
                (
                    keyword("bucket")("op")
                    + int_num("params")
                    + keyword("out of")
                    + int_num("params")
                    + Optional(ON + expr("on"))
                )
                / to_json_call
                | (real_num | int_num)("percent") + keyword("percent")
                | int_num("rows") + keyword("rows")
                | ts_bytes
            )
            + RB,
        )

        assumption = Group((ASC|DESC) ("sort") + var_name("value"))
        assumptions = (ASSUMING + Group(delimited_list(assumption))("assumptions"))

        table_source << Group(
            ((LB + query + RB) | stack | call_function | var_name)("value")
            + Optional(assumptions)
            + Optional(flag("with ordinality"))
            + Optional(tablesample)
            + alias
        ).set_parser_name("table_source") / to_table

        rows = Optional(keyword("row") | keyword("rows"))
        limit = (
            Optional(assign("offset", expr) + rows)
            & Optional(
                FETCH
                + Optional(keyword("first") | keyword("next"))
                + expr("fetch")
                + rows
                + Optional(keyword("only"))
            )
            & Optional(assign("limit", expr))
        )

        outfile = Optional( 
            (
                INTO
                + keyword("outfile").suppress()
                + literal_string ("loc") 
                + Optional (
                    keyword("fields")
                    + keyword("terminated") 
                    + keyword("by") 
                    + literal_string ("term")
                ) 
            )("outfile")
        )
        ordered_sql = (
            (
                (unordered_sql | (LB + query + RB))
                + ZeroOrMore(
                    Group(
                        (UNION | INTERSECT | EXCEPT | MINUS) + Optional(ALL | DISTINCT)
                    )("op")
                    + (unordered_sql | (LB + query + RB))
                )
            )("union")
            + Optional(ORDER_BY + delimited_list(Group(sort_column))("orderby"))
            + limit
            + outfile
        ).set_parser_name("ordered sql") / to_union_call

        with_expr = delimited_list(Group(
            (
                (var_name("name") + Optional(LB + delimited_list(ident("col")) + RB))
                / to_alias
            )("name")
            + (AS + LB + (query | expr)("value") + RB)
        ))

        query << (
            Optional(assign("with recursive", with_expr) | assign("with", with_expr))
            + Group(ordered_sql)("query")
        ) / to_query

        #####################################################################
        # DML STATEMENTS
        #####################################################################

        # MySQL's index_type := Using + ( "BTREE" | "HASH" )
        index_type = Optional(assign("using", ident("index_type")))

        index_column_names = LB + delimited_list(var_name("columns")) + RB

        column_def_delete = assign(
            "on delete",
            (keyword("cascade") | keyword("set null") | keyword("set default")),
        )

        table_def_foreign_key = FOREIGN_KEY + Optional(
            Optional(var_name("index_name"))
            + index_column_names
            + column_def_references
            + Optional(column_def_delete)
        )

        index_options = ZeroOrMore(var_name)("table_constraint_options")

        table_constraint_definition = Optional(CONSTRAINT + var_name("name")) + (
            assign("primary key", index_type + index_column_names + index_options)
            | (
                Optional(flag("unique"))
                + Optional(INDEX | KEY)
                + Optional(var_name("name"))
                + index_type
                + index_column_names
                + index_options
            )("index")
            | assign("check", LB + expr + RB)
            | table_def_foreign_key("foreign_key")
        )

        table_element = (
            column_definition("columns") | table_constraint_definition("constraint")
        )

        create_table = (
            keyword("create")
            + Optional(keyword("or") + flag("replace"))
            + Optional(flag("temporary"))
            + TABLE
            + Optional((keyword("if not exists") / (lambda: False))("replace"))
            + var_name("name")
            + Optional(LB + delimited_list(table_element) + RB)
            + ZeroOrMore(
                assign("engine", EQ + var_name)
                | assign("collate", EQ + var_name)
                | assign("auto_increment", EQ + int_num)
                | assign("comment", EQ + literal_string)
                | assign("default character set", EQ + var_name)
                | assign("default charset", EQ + var_name)
            )
            + Optional(AS.suppress() + infix_notation(query, [])("query"))
        )("create_table")

        create_view = (
            keyword("create")
            + Optional(keyword("or") + flag("replace"))
            + Optional(flag("temporary"))
            + VIEW.suppress()
            + Optional((keyword("if not exists") / (lambda: False))("replace"))
            + var_name("name")
            + AS
            + query("query")
        )("create_view")

        #  CREATE INDEX a ON u USING btree (e);
        create_index = (
            keyword("create index")
            + Optional(keyword("or") + flag("replace"))(INDEX | KEY)
            + Optional((keyword("if not exists") / (lambda: False))("replace"))
            + var_name("name")
            + ON
            + var_name("table")
            + index_type
            + index_column_names
            + index_options
        )("create index")

        cache_options = Optional((
            keyword("options").suppress()
            + LB
            + Dict(delimited_list(Group(
                literal_string / (lambda tokens: tokens[0]["literal"])
                + Optional(EQ)
                + var_name
            )))
            + RB
        )("options"))

        create_cache = (
            keyword("cache").suppress()
            + Optional(flag("lazy"))
            + TABLE
            + var_name("name")
            + cache_options
            + Optional(AS + query("query"))
        )("cache")

        drop_table = (
            keyword("drop table") + Optional(flag("if exists")) + var_name("table")
        )("drop")

        drop_view = (
            keyword("drop view") + Optional(flag("if exists")) + var_name("view")
        )("drop")

        drop_index = (
            keyword("drop index") + Optional(flag("if exists")) + var_name("index")
        )("drop")

        insert = (
            keyword("insert").suppress()
            + (
                flag("overwrite") + keyword("table").suppress()
                | keyword("into").suppress() + Optional(keyword("table").suppress())
            )
            + var_name("table")
            + Optional(LB + delimited_list(var_name)("columns") + RB)
            + Optional(flag("if exists"))
            + (values | query)("query")
        ) / to_insert_call

        update = (
            keyword("update")("op")
            + var_name("params")
            + assign("set", Dict(delimited_list(Group(var_name + EQ + expr))))
            + Optional(assign("where", expr))
        ) / to_json_call

        delete = (
            keyword("delete")("op")
            + keyword("from").suppress()
            + var_name("params")
            + Optional(assign("where", expr))
        ) / to_json_call

        load = (
            keyword("load")("op") 
            + keyword("data").suppress() 
            + keyword("infile")("loc")  
            + literal_string ("file")
            + INTO
            + keyword("table").suppress()
            + var_name ("table")
            + Optional(
                  keyword("fields").suppress()
                  + keyword("terminated").suppress()
                  + keyword("by").suppress() 
                  + literal_string ("term")
            )
        ) ("load")



        sql_stmts = delimited_list( (
            query
            | (insert | update | delete | load)
            | (create_table | create_view | create_cache | create_index)
            | (drop_table | drop_view | drop_index)
        )("stmts"), ";")

        other_stmt = (
            inline_kblock
            | udf
        ) ("stmts")
        
        stmts = ZeroOrMore(
            sql_stmts
            |other_stmt
            | keyword(";").suppress() # empty stmt
        )
        
        return stmts.finalize()
