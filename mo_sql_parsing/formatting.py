# encoding: utf-8
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Author: Beto Dealmeida (beto@dealmeida.net)
#

from __future__ import absolute_import, division, unicode_literals

import re

from mo_dots import split_field
from mo_future import first, is_text, string_types, text
from mo_parsing import listwrap

from mo_sql_parsing.keywords import RESERVED, join_keywords, precedence
from mo_sql_parsing.utils import binary_ops, is_set_op

MAX_PRECEDENCE = 100
VALID = re.compile(r"^[a-zA-Z_]\w*$")


def is_keyword(identifier):
    try:
        RESERVED.parse_string(identifier)
        return True
    except Exception:
        return False


def should_quote(identifier):
    """
    Return true if a given identifier should be quoted.

    This is usually true when the identifier:

      - is a reserved word
      - contain spaces
      - does not match the regex `[a-zA-Z_]\\w*`

    """
    return identifier != "*" and (not VALID.match(identifier) or is_keyword(identifier))


def escape(ident, ansi_quotes, should_quote):
    """
    Escape identifiers.

    ANSI uses double quotes, but many databases use back quotes.

    """

    def esc(identifier):
        if not should_quote(identifier):
            return identifier

        quote = '"' if ansi_quotes else "`"
        identifier = identifier.replace(quote, 2 * quote)
        return "{0}{1}{2}".format(quote, identifier, quote)

    return ".".join(esc(f) for f in split_field(ident))


def Operator(_op):
    op_prec = precedence[binary_ops[_op]]
    op = " {0} ".format(_op).replace("_", " ").upper()

    def func(self, json, prec):
        acc = []

        if isinstance(json, dict):
            # {VARIABLE: VALUE} FORM
            k, v = first(json.items())
            json = [k, {"literal": v}]

        for i, v in enumerate(listwrap(json)):
            if i == 0:
                acc.append(self.dispatch(v, op_prec + 0.25))
            else:
                acc.append(self.dispatch(v, op_prec))
        if prec >= op_prec:
            return op.join(acc)
        else:
            return f"({op.join(acc)})"

    return func


def isolate(expr, sql, prec):
    """
    RETURN sql IN PARENTHESIS IF PREEDENCE > prec
    :param expr: expression to isolate
    :param sql: sql to return
    :param prec: current precedence
    """
    if is_text(expr):
        return sql
    ps = [p for k in expr.keys() for p in [precedence.get(k)] if p is not None]
    if not ps:
        return sql
    elif min(ps) >= prec:
        return f"({sql})"
    else:
        return sql


unordered_clauses = [
    "with",
    "distinct_on",
    "select_distinct",
    "select",
    "from",
    "where",
    "groupby",
    "having",
]

ordered_clauses = [
    "orderby",
    "limit",
    "offset",
    "fetch",
]


class Formatter:
    # infix operators
    _concat = Operator("||")
    _mul = Operator("*")
    _div = Operator("/")
    _mod = Operator("%")
    _add = Operator("+")
    _sub = Operator("-")
    _neq = Operator("<>")
    _gt = Operator(">")
    _lt = Operator("<")
    _gte = Operator(">=")
    _lte = Operator("<=")
    _eq = Operator("=")
    _or = Operator("or")
    _and = Operator("and")
    _binary_and = Operator("&")
    _binary_or = Operator("|")
    _like = Operator("like")
    _not_like = Operator("not like")
    _rlike = Operator("rlike")
    _not_rlike = Operator("not rlike")
    _union = Operator("union")
    _union_all = Operator("union all")
    _intersect = Operator("intersect")
    _minus = Operator("minus")
    _except = Operator("except")

    def __init__(self, ansi_quotes=True, should_quote=should_quote):
        self.ansi_quotes = ansi_quotes
        self.should_quote = should_quote

    def format(self, json):
        return self.dispatch(json, 50)

    def dispatch(self, json, prec=100):
        if isinstance(json, list):
            return self.sql_list(json, prec=precedence["list"])
        if isinstance(json, dict):
            if len(json) == 0:
                return ""
            elif "value" in json:
                return self.value(json, prec)
            elif "join" in json:
                return self._join_on(json)
            elif "insert" in json:
                return self.insert(json)
            elif json.keys() & set(ordered_clauses):
                return self.ordered_query(json, prec)
            elif json.keys() & set(unordered_clauses):
                return self.unordered_query(json, prec)
            elif "null" in json:
                return "NULL"
            elif "trim" in json:
                return self._trim(json, prec)
            elif "extract" in json:
                return self._extract(json, prec)
            else:
                return self.op(json, prec)
        if isinstance(json, string_types):
            return escape(json, self.ansi_quotes, self.should_quote)
        if json == None:
            return "NULL"

        return text(json)

    def sql_list(self, json, prec=precedence["from"] - 1):
        sql = ", ".join(self.dispatch(element, prec=MAX_PRECEDENCE) for element in json)
        if prec >= precedence["from"]:
            return sql
        else:
            return f"({sql})"

    def value(self, json, prec=precedence["from"]):
        parts = [self.dispatch(json["value"], prec)]
        if "over" in json:
            over = json["over"]
            parts.append("OVER")
            window = []
            if "partitionby" in over:
                window.append("PARTITION BY")
                window.append(self.dispatch(over["partitionby"]))
            if "orderby" in over:
                window.append(self.orderby(over, precedence["window"]))
            if "range" in over:

                def wordy(v):
                    if v < 0:
                        return [text(abs(v)), "PRECEDING"]
                    elif v > 0:
                        return [text(v), "FOLLOWING"]

                window.append("ROWS")
                range = over["range"]
                min = range.get("min")
                max = range.get("max")

                if min is None:
                    if max is None:
                        window.pop()  # not expected, but deal
                    elif max == 0:
                        window.append("UNBOUNDED PRECEDING")
                    else:
                        window.append("BETWEEN")
                        window.append("UNBOUNDED PRECEDING")
                        window.append("AND")
                        window.extend(wordy(max))
                elif min == 0:
                    if max is None:
                        window.append("UNBOUNDED FOLLOWING")
                    elif max == 0:
                        window.append("CURRENT ROW")
                    else:
                        window.extend(wordy(max))
                else:
                    if max is None:
                        window.append("BETWEEN")
                        window.extend(wordy(min))
                        window.append("AND")
                        window.append("UNBOUNDED FOLLOWING")
                    elif max == 0:
                        window.extend(wordy(min))
                    else:
                        window.append("BETWEEN")
                        window.extend(wordy(min))
                        window.append("AND")
                        window.extend(wordy(max))

            window = " ".join(window)
            parts.append(f"({window})")
        if "name" in json:
            parts.extend(["AS", self.dispatch(json["name"])])

        return " ".join(parts)

    def op(self, json, prec):
        if len(json) > 1:
            raise Exception("Operators should have only one key!")
        key, value = list(json.items())[0]

        # check if the attribute exists, and call the corresponding method;
        # note that we disallow keys that start with `_` to avoid giving access
        # to magic methods
        attr = f"_{key}"
        if hasattr(self, attr) and not key.startswith("_"):
            method = getattr(self, attr)
            op_prec = precedence.get(key, MAX_PRECEDENCE)
            if prec >= op_prec:
                return method(value, op_prec)
            else:
                return f"({method(value, op_prec)})"

        # treat as regular function call
        if isinstance(value, dict) and len(value) == 0:
            return (
                key.upper() + "()"
            )  # NOT SURE IF AN EMPTY dict SHOULD BE DELT WITH HERE, OR IN self.format()
        else:
            params = ", ".join(self.dispatch(p) for p in listwrap(value))
            return f"{key.upper()}({params})"

    def _binary_not(self, value, prec):
        return "~{0}".format(self.dispatch(value))

    def _exists(self, value, prec):
        return "{0} IS NOT NULL".format(self.dispatch(value, precedence["is"]))

    def _missing(self, value, prec):
        return "{0} IS NULL".format(self.dispatch(value, precedence["is"]))

    def _collate(self, pair, prec):
        return "{0} COLLATE {1}".format(
            self.dispatch(pair[0], precedence["collate"]), pair[1]
        )

    def _in(self, json, prec):
        member, set = json
        if "literal" in set:
            set = {"literal": listwrap(set["literal"])}
        sql = (
            self.dispatch(member, precedence["in"])
            + " IN "
            + self.dispatch(set, precedence["in"])
        )
        if prec < precedence["in"]:
            sql = f"({sql})"
        return sql

    def _nin(self, json, prec):
        member, set = json
        if "literal" in set:
            set = {"literal": listwrap(set["literal"])}
        sql = (
            self.dispatch(member, precedence["in"])
            + " NOT IN "
            + self.dispatch(set, precedence["in"])
        )
        if prec < precedence["in"]:
            sql = f"({sql})"
        return sql

    def _case(self, checks, prec):
        parts = ["CASE"]
        for check in checks if isinstance(checks, list) else [checks]:
            if isinstance(check, dict):
                if "when" in check and "then" in check:
                    parts.extend(["WHEN", self.dispatch(check["when"])])
                    parts.extend(["THEN", self.dispatch(check["then"])])
                else:
                    parts.extend(["ELSE", self.dispatch(check)])
            else:
                parts.extend(["ELSE", self.dispatch(check)])
        parts.append("END")
        return " ".join(parts)

    def _cast(self, json, prec):
        expr, type = json

        type_name, params = first(type.items())
        if not params:
            type = type_name.upper()
        else:
            type = {type_name.upper(): params}

        return f"CAST({self.dispatch(expr)} AS {self.dispatch(type)})"

    def _extract(self, json, prec):
        interval, value = json["extract"]
        i = self.dispatch(interval).upper()
        v = self.dispatch(value)
        return f"EXTRACT({i} FROM {v})"

    def _interval(self, json, prec):
        amount = self.dispatch(json[0], precedence["and"])
        type = self.dispatch(json[1], precedence["and"])
        return f"INTERVAL {amount} {type.upper()}"

    def _literal(self, json, prec=0):
        if isinstance(json, list):
            return "({0})".format(", ".join(
                self._literal(v, precedence["literal"]) for v in json
            ))
        elif isinstance(json, string_types):
            return "'{0}'".format(json.replace("'", "''"))
        else:
            return str(json)

    def _get(self, json, prec):
        v, i = json
        v_sql = self.dispatch(v, prec=precedence["literal"])
        i_sql = self.dispatch(i)
        return f"{v_sql}[{i_sql}]"

    def _between(self, json, prec):
        return "{0} BETWEEN {1} AND {2}".format(
            self.dispatch(json[0], precedence["between"]),
            self.dispatch(json[1], precedence["between"]),
            self.dispatch(json[2], precedence["between"]),
        )

    def _trim(self, json, prec):
        c = json.get("characters")
        d = json.get("direction")
        v = json["trim"]
        acc = ["TRIM("]
        if d:
            acc.append(d.upper())
            acc.append(" ")
        if c:
            acc.append(self.dispatch(c))
            acc.append(" ")
        if c or d:
            acc.append("FROM ")
        acc.append(self.dispatch(v))
        acc.append(")")
        return "".join(acc)

    def _not_between(self, json, prec):
        return "{0} NOT BETWEEN {1} AND {2}".format(
            self.dispatch(json[0], precedence["between"]),
            self.dispatch(json[1], precedence["between"]),
            self.dispatch(json[2], precedence["between"]),
        )

    def _distinct(self, json, prec):
        return "DISTINCT " + ", ".join(
            self.dispatch(v, precedence["select"]) for v in listwrap(json)
        )

    def _select_distinct(self, json, prec):
        return "SELECT DISTINCT " + ", ".join(self.dispatch(v) for v in listwrap(json))

    def _distinct_on(self, json, prec):
        return (
            "DISTINCT ON (" + ", ".join(self.dispatch(v) for v in listwrap(json)) + ")"
        )

    def _join_on(self, json, prec):
        detected_join = join_keywords & set(json.keys())
        if len(detected_join) == 0:
            raise Exception(
                'Fail to detect join type! Detected: "{}" Except one of: "{}"'.format(
                    [on_keyword for on_keyword in json if on_keyword != "on"][0],
                    '", "'.join(join_keywords),
                )
            )

        join_keyword = detected_join.pop()

        acc = []
        acc.append(join_keyword.upper())
        acc.append(self.dispatch(json[join_keyword], precedence["join"]))

        if json.get("on"):
            acc.append("ON")
            acc.append(self.dispatch(json["on"]))
        if json.get("using"):
            acc.append("USING")
            acc.append(self.dispatch(json["using"]))
        return " ".join(acc)

    def ordered_query(self, json, prec):
        if json.keys() & set(unordered_clauses) - {"from"}:
            # regular query
            acc = [self.unordered_query(json, precedence["order"])]
        else:
            # set-op expression
            acc = [self.dispatch(json["from"], precedence["order"])]

        acc.extend(
            part
            for clause in ordered_clauses
            if clause in json
            for part in [getattr(self, clause)(json, precedence["order"])]
            if part
        )
        sql = " ".join(acc)
        if prec >= precedence["order"]:
            return sql
        else:
            return f"({sql})"

    def unordered_query(self, json, prec):
        sql = " ".join(
            part
            for clause in unordered_clauses
            if clause in json
            for part in [getattr(self, clause)(json, precedence["from"])]
            if part
        )
        if prec >= precedence["from"]:
            return sql
        else:
            return f"({sql})"

    def with_(self, json, prec):
        if "with" in json:
            with_ = json["with"]
            if not isinstance(with_, list):
                with_ = [with_]
            parts = ", ".join(
                "{0} AS ({1})".format(part["name"], self.dispatch(part["value"]))
                for part in with_
            )
            return "WITH {0}".format(parts)

    def select(self, json, prec):
        param = ", ".join(self.dispatch(s) for s in listwrap(json["select"]))
        if "top" in json:
            top = self.dispatch(json["top"])
            return f"SELECT TOP ({top}) {param}"
        if "distinct_on" in json:
            return param
        else:
            return f"SELECT {param}"

    def distinct_on(self, json, prec):
        param = ", ".join(self.dispatch(s) for s in listwrap(json["distinct_on"]))
        return f"SELECT DISTINCT ON ({param})"

    def select_distinct(self, json, prec):
        param = ", ".join(self.dispatch(s) for s in listwrap(json["select_distinct"]))
        return f"SELECT DISTINCT {param}"

    def from_(self, json, prec):
        is_join = False
        from_ = json["from"]
        if isinstance(from_, dict) and is_set_op & from_.keys():
            source = self.op(from_, precedence["from"])
            return f"FROM {source}"

        from_ = listwrap(from_)
        parts = []
        for v in from_:
            if join_keywords & set(v):
                is_join = True
                parts.append(self._join_on(v, precedence["from"] - 1))
            else:
                parts.append(self.dispatch(v, precedence["from"] - 1))
        joiner = " " if is_join else ", "
        rest = joiner.join(parts)
        return f"FROM {rest}"

    def where(self, json, prec):
        expr = self.dispatch(json["where"])
        return f"WHERE {expr}"

    def groupby(self, json, prec):
        param = ", ".join(self.dispatch(s) for s in listwrap(json["groupby"]))
        return f"GROUP BY {param}"

    def having(self, json, prec):
        return "HAVING {0}".format(self.dispatch(json["having"]))

    def orderby(self, json, prec):
        param = ", ".join(
            (
                self.dispatch(s["value"], precedence["order"])
                + " "
                + s.get("sort", "").upper()
            ).strip()
            for s in listwrap(json["orderby"])
        )
        return f"ORDER BY {param}"

    def limit(self, json, prec):
        num = self.dispatch(json["limit"], precedence["order"])
        return f"LIMIT {num}"

    def offset(self, json, prec):
        num = self.dispatch(json["offset"], precedence["order"])
        return f"OFFSET {num}"

    def fetch(self, json, prec):
        num = self.dispatch(json["offset"], precedence["order"])
        return f"FETCH {num} ROWS ONLY"

    def insert(self, json, prec=precedence["from"]):
        acc = ["INSERT"]
        if "overwrite" in json:
            acc.append("OVERWRITE")
        else:
            acc.append("INTO")
        acc.append(json["insert"])

        if "columns" in json:
            acc.append(self.sql_list(json))
        if "values" in json:
            values = json["values"]
            if all(isinstance(row, dict) for row in values):
                columns = list(sorted(set(k for row in values for k in row.keys())))
                acc.append(self.sql_list(columns))
                if "if exists" in json:
                    acc.append("IF EXISTS")
                acc.append("VALUES")
                acc.append(",\n".join(
                    "(" + ", ".join(self._literal(row[c]) for c in columns) + ")"
                    for row in values
                ))
            else:
                if "if exists" in json:
                    acc.append("IF EXISTS")
                acc.append("VALUES")
                for row in values:
                    acc.append("(" + ", ".join(self._literal(row)) + ")")

        else:
            if json["if exists"]:
                acc.append("IF EXISTS")
            acc.append(self.dispatch(json["query"]))
        return " ".join(acc)


setattr(Formatter, "with", Formatter.with_)
setattr(Formatter, "from", Formatter.from_)
