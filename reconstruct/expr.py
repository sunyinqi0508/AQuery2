from reconstruct.ast import ast_node
from reconstruct.storage import ColRef
from engine.types import *

class expr(ast_node):
    name='expr'

    def __init__(self, parent, node, *, c_code = None):
        from reconstruct.ast import projection
        
        self.type = None
        self.raw_col = None
        self.inside_agg = False
        self.is_special = False
        if(type(parent) is expr):
            self.inside_agg = parent.inside_agg
        if(type(parent) is not expr):
            self.root = self
            self.c_code = type(parent) is projection
        else:
            self.root = parent.root
            self.c_code = parent.c_code
        
        if type(c_code) is bool:
            self.c_code = c_code
        
        ast_node.__init__(self, parent, node, None)

    def init(self, _):
        from reconstruct.ast import projection
        parent = self.parent
        self.isvector = parent.isvector if type(parent) is expr else False
        self.is_compound = parent.is_compound if type(parent) is expr else False
        if type(parent) in [projection, expr]:
            self.datasource = parent.datasource
        else:
            self.datasource = self.context.datasource
        self.udf_map = parent.context.udf_map
        self.func_maps = {**builtin_func, **self.udf_map}
        self.operators = {**builtin_operators, **self.udf_map}
    def produce(self, node):
        from engine.utils import enlist
        if type(node) is dict:
            for key, val in node.items():
                if key in self.operators:
                    op = self.operators[key]

                    val = enlist(val)
                    exp_vals = [expr(self, v, c_code = self.c_code) for v in val]
                    str_vals = [e.sql for e in exp_vals]
                    type_vals = [e.type for e in exp_vals]
                    
                    self.type = op.return_type(*type_vals)
                    self.sql = op(self.c_code, *str_vals)
                    special_func = [*self.context.udf_map.keys(), "maxs", "mins", "avgs", "sums"]
                    if key in special_func and not self.is_special:
                        self.is_special = True
                        p = self.parent
                        while type(p) is expr and not p.is_special:
                            p.is_special = True
                            p = p.parent
                else:
                    print(f'Undefined expr: {key}{val}')

        elif type(node) is str:
            p = self.parent
            while type(p) is expr and not p.isvector:
                p.isvector = True
                p = p.parent
            if self.datasource is not None:
                self.raw_col = self.datasource.parse_col_names(node)
                self.raw_col = self.raw_col if type(self.raw_col) is ColRef else None
            if self.raw_col is not None:
                self.sql = self.raw_col.name
                self.type = self.raw_col.type
            else:
                self.sql = node
                self.type = StrT
            if self.c_code and self.datasource is not None:
                self.sql = f'{{y(\"{self.sql}\")}}'
        elif type(node) is bool:
            self.type = IntT
            if self.c_code:
                self.sql = '1' if node else '0'
            else:
                self.sql = 'TRUE' if node else 'FALSE'
        else:
            self.sql = f'{node}'
            if type(node) is int:
                self.type = LongT
            elif type(node) is float:
                self.type = DoubleT

    def finalize(self, y):
        if self.c_code:
            x = self.is_special
            self.sql = eval('f\'' + self.sql + '\'')
        
    def __str__(self):
        return self.sql
    def __repr__(self):
        return self.__str__()
    def eval(self):
        x = self.c_code
        y = lambda x: x
        return eval('f\'' + self.sql + '\'')    

import engine.expr as cexpr
