# encoding: utf-8
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Contact: Kyle Lahnakoski (kyle@lahnakoski.com)
# Bill Sun  2022 - 2023

from __future__ import absolute_import, division, unicode_literals

import json
from threading import Lock

from aquery_parser.parser import scrub
from aquery_parser.utils import simple_op, normal_op
import aquery_parser.parser
parse_locker = Lock()  # ENSURE ONLY ONE PARSING AT A TIME
common_parser = None

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
            common_parser = aquery_parser.parser.common_parser()
        result = _parse(common_parser, sql, null, calls)
        return result

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

__all__ = ["parse", "format", "normal_op", "simple_op"]
