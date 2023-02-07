# encoding: utf-8
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Contact: Kyle Lahnakoski (kyle@lahnakoski.com)
# Bill Sun  2022 - 2023


# KNOWN TYPES
from mo_parsing import Forward, Group, Optional, MatchFirst, Literal, ZeroOrMore, export
from mo_parsing.infix import delimited_list, RIGHT_ASSOC, LEFT_ASSOC

from aquery_parser.keywords import (
    RB,
    LB,
    NEG,
    NOT,
    BINARY_NOT,
    NULL,
    EQ,
    KNOWN_OPS,
    LT,
    GT,
)
from aquery_parser.utils import (
    keyword,
    to_json_call,
    int_num,
    ansi_string,
    ansi_ident,
    assign,
    flag,
)

_size = Optional(LB + int_num("params") + RB)
_sizes = Optional(LB + delimited_list(int_num("params")) + RB)

simple_types = Forward()

BIGINT = Group(keyword("bigint")("op") + Optional(_size)+Optional(flag("unsigned"))) / to_json_call
BOOL = Group(keyword("bool")("op")) / to_json_call
BOOLEAN = Group(keyword("boolean")("op")) / to_json_call
DOUBLE = Group(keyword("double")("op")) / to_json_call
FLOAT64 = Group(keyword("float64")("op")) / to_json_call
FLOAT = Group(keyword("float")("op")) / to_json_call
GEOMETRY = Group(keyword("geometry")("op")) / to_json_call
INTEGER = Group(keyword("integer")("op")) / to_json_call
INT = (keyword("int")("op") + _size) / to_json_call
INT32 = Group(keyword("int32")("op")) / to_json_call
INT64 = Group(keyword("int64")("op")) / to_json_call
REAL = Group(keyword("real")("op")) / to_json_call
TEXT = Group(keyword("text")("op")) / to_json_call
SMALLINT = Group(keyword("smallint")("op")) / to_json_call
STRING = Group(keyword("string")("op")) / to_json_call

BLOB = (keyword("blob")("op") + _size) / to_json_call
BYTES = (keyword("bytes")("op") + _size) / to_json_call
CHAR = (keyword("char")("op") + _size) / to_json_call
NCHAR = (keyword("nchar")("op") + _size) / to_json_call
VARCHAR = (keyword("varchar")("op") + _size) / to_json_call
VARCHAR2 = (keyword("varchar2")("op") + _size) / to_json_call
VARBINARY = (keyword("varbinary")("op") + _size) / to_json_call
TINYINT = (keyword("tinyint")("op") + _size) / to_json_call
UUID = Group(keyword("uuid")("op")) / to_json_call

DECIMAL = (keyword("decimal")("op") + _sizes) / to_json_call
DOUBLE_PRECISION = (
    Group((keyword("double precision") / (lambda: "double_precision"))("op"))
    / to_json_call
)
NUMERIC = (keyword("numeric")("op") + _sizes) / to_json_call
NUMBER = (keyword("number")("op") + _sizes) / to_json_call

MAP_TYPE = (
    keyword("map")("op") + LB + delimited_list(simple_types("params")) + RB
) / to_json_call
ARRAY_TYPE = (keyword("array")("op") + LB + simple_types("params") + RB) / to_json_call

DATE = keyword("date")
DATETIME = keyword("datetime")
DATETIME_W_TIMEZONE = keyword("datetime with time zone")
TIME = keyword("time")
TIMESTAMP = keyword("timestamp")
TIMESTAMP_W_TIMEZONE = keyword("timestamp with time zone")
TIMESTAMPTZ = keyword("timestamptz")
TIMETZ = keyword("timetz")

time_functions = DATE | DATETIME | TIME | TIMESTAMP | TIMESTAMPTZ | TIMETZ

# KNOWNN TIME TYPES
_format = Optional((ansi_string | ansi_ident)("params"))

DATE_TYPE = (DATE("op") + _format) / to_json_call
DATETIME_TYPE = (DATETIME("op") + _format) / to_json_call
DATETIME_W_TIMEZONE_TYPE = (DATETIME_W_TIMEZONE("op") + _format) / to_json_call
TIME_TYPE = (TIME("op") + _format) / to_json_call
TIMESTAMP_TYPE = (TIMESTAMP("op") + _format) / to_json_call
TIMESTAMP_W_TIMEZONE_TYPE = (TIMESTAMP_W_TIMEZONE("op") + _format) / to_json_call
TIMESTAMPTZ_TYPE = (TIMESTAMPTZ("op") + _format) / to_json_call
TIMETZ_TYPE = (TIMETZ("op") + _format) / to_json_call

simple_types << MatchFirst([
    ARRAY_TYPE,
    BIGINT,
    BOOL,
    BOOLEAN,
    BLOB,
    BYTES,
    CHAR,
    DATE_TYPE,
    DATETIME_W_TIMEZONE_TYPE,
    DATETIME_TYPE,
    DECIMAL,
    DOUBLE_PRECISION,
    DOUBLE,
    FLOAT64,
    FLOAT,
    GEOMETRY,
    MAP_TYPE,
    INTEGER,
    INT,
    INT32,
    INT64,
    NCHAR,
    NUMBER,
    NUMERIC,
    REAL,
    TEXT,
    SMALLINT,
    STRING,
    TIME_TYPE,
    TIMESTAMP_W_TIMEZONE_TYPE,
    TIMESTAMP_TYPE,
    TIMESTAMPTZ_TYPE,
    TIMETZ_TYPE,
    TINYINT,
    UUID,
    VARCHAR,
    VARCHAR2,
    VARBINARY,
])

CASTING = (Literal("::").suppress() + simple_types("params")).set_parser_name("cast")
KNOWN_OPS.insert(0, CASTING)

unary_ops = {
    NEG: RIGHT_ASSOC,
    NOT: RIGHT_ASSOC,
    BINARY_NOT: RIGHT_ASSOC,
    CASTING: LEFT_ASSOC,
}


def get_column_type(expr, var_name, literal_string):
    column_definition = Forward()
    column_type = Forward().set_parser_name("column type")

    struct_type = (
        keyword("struct")("op")
        + LT.suppress()
        + Group(delimited_list(column_definition))("params")
        + GT.suppress()
    ) / to_json_call

    row_type = (
        keyword("row")("op")
        + LB
        + Group(delimited_list(column_definition))("params")
        + RB
    ) / to_json_call

    array_type = (
        keyword("array")("op")
        + (
            (
                LT.suppress()
                + Group(delimited_list(column_type))("params")
                + GT.suppress()
            )
            | (LB + Group(delimited_list(column_type))("params") + RB)
        )
    ) / to_json_call

    column_type << (struct_type | row_type | array_type | simple_types)

    column_def_identity = (
        assign(
            "generated",
            (keyword("always") | keyword("by default") / (lambda: "by_default")),
        )
        + keyword("as identity").suppress()
        + Optional(assign("start with", int_num))
        + Optional(assign("increment by", int_num))
    )

    column_def_references = assign(
        "references", var_name("table") + LB + delimited_list(var_name)("columns") + RB,
    )

    column_options = ZeroOrMore(
        ((NOT + NULL) / (lambda: False))("nullable")
        | (NULL / (lambda t: True))("nullable")
        | flag("unique")
        | flag("auto_increment")
        | assign("comment", literal_string)
        | assign("collate", Optional(EQ) + var_name)
        | flag("primary key")
        | column_def_identity("identity")
        | column_def_references
        | assign("check", LB + expr + RB)
        | assign("default", expr)
    ).set_parser_name("column_options")

    column_definition << Group(
        var_name("name") + (column_type | var_name)("type") + column_options
    ).set_parser_name("column_definition")

    return column_type, column_definition, column_def_references


export("aquery_parser.utils", unary_ops)
