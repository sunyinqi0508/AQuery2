# encoding: utf-8
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Contact: Kyle Lahnakoski (kyle@lahnakoski.com)
#

from __future__ import absolute_import, division, unicode_literals

import json
from threading import Lock

from aquery_parser.sql_parser import scrub
from aquery_parser.utils import ansi_string, simple_op, normal_op

parse_locker = Lock()  # ENSURE ONLY ONE PARSING AT A TIME
common_parser = None
mysql_parser = None
sqlserver_parser = None

SQL_NULL = {"null": {}}


def parse(sql, null=SQL_NULL, calls=simple_op):
    """
    :param sql: String of SQL
    :param null: What value to use as NULL (default is the null function `{"null":{}}`)
    :return: parse tree
    """
    global common_parser

    with parse_locker:
        if not common_parser:
            common_parser = sql_parser.common_parser()
        result = _parse(common_parser, sql, null, calls)
        return result


def parse_mysql(sql, null=SQL_NULL, calls=simple_op):
    """
    PARSE MySQL ASSUME DOUBLE QUOTED STRINGS ARE LITERALS
    :param sql: String of SQL
    :param null: What value to use as NULL (default is the null function `{"null":{}}`)
    :return: parse tree
    """
    global mysql_parser

    with parse_locker:
        if not mysql_parser:
            mysql_parser = sql_parser.mysql_parser()
        return _parse(mysql_parser, sql, null, calls)


def parse_sqlserver(sql, null=SQL_NULL, calls=simple_op):
    """
    PARSE MySQL ASSUME DOUBLE QUOTED STRINGS ARE LITERALS
    :param sql: String of SQL
    :param null: What value to use as NULL (default is the null function `{"null":{}}`)
    :return: parse tree
    """
    global sqlserver_parser

    with parse_locker:
        if not sqlserver_parser:
            sqlserver_parser = sql_parser.sqlserver_parser()
        return _parse(sqlserver_parser, sql, null, calls)


parse_bigquery = parse_mysql


def _parse(parser, sql, null, calls):
    utils.null_locations = []
    utils.scrub_op = calls
    sql = sql.rstrip().rstrip(";")
    parse_result = parser.parse_string(sql, parse_all=True)
    output = scrub(parse_result)
    for o, n in utils.null_locations:
        o[n] = null
    return output



_ = json.dumps

__all__ = ["parse", "format", "parse_mysql", "parse_bigquery", "normal_op", "simple_op"]
