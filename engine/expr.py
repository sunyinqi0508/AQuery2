from engine.ast import ast_node, ColRef
start_expr = 'f"'
index_expr = '{\'\' if x is None and y is None else f\'[{x}]\'}'
end_expr = '"'

class expr(ast_node):
    name='expr'
    builtin_func_maps = {
        'max': 'max',
        'min': 'min', 
        'avg': 'avg',
        'sum': 'sum',
        'count' : 'count',
        'mins': ['mins', 'minw'],
        'maxs': ['maxs', 'maxw'],
        'avgs': ['avgs', 'avgw'],
        'sums': ['sums', 'sumw'],
    }
    
    binary_ops = {
        'sub':'-',  
        'add':'+', 
        'mul':'*', 
        'div':'/',
        'mod':'%',
        'and':'&&',
        'or':'||',
        'xor' : '^',
        'gt':'>',
        'lt':'<',
        'le':'<=',
        'gt':'>='
    }

    compound_ops = {
    }

    unary_ops = {
        'neg' : '-',
        'not' : '!'
    }
    
    coumpound_generating_ops = ['avgs', 'mins', 'maxs', 'sums'] + \
       list( binary_ops.keys()) + list(compound_ops.keys()) + list(unary_ops.keys() )

    def __init__(self, parent, node, materialize_cols = True, abs_col = False):
        self.materialize_cols = materialize_cols
        self.raw_col = None
        self.__abs = abs_col
        self.inside_agg = False
        if(type(parent) is expr):
            self.inside_agg = parent.inside_agg
            self.__abs = parent.__abs
        ast_node.__init__(self, parent, node, None)

    def init(self, _):
        from engine.projection import projection
        parent = self.parent
        self.isvector = parent.isvector if type(parent) is expr else False
        self.is_compound = parent.is_compound if type(parent) is expr else False
        if type(parent) in [projection, expr]:
            self.datasource = parent.datasource
        else:
            self.datasource = self.context.datasource
        self.udf_map = parent.context.udf_map
        self._expr = ''
        self.cexpr = None
        self.func_maps = {**self.udf_map, **self.builtin_func_maps}

    def produce(self, node):
        if type(node) is dict:
            for key, val in node.items():
                if key in self.func_maps:
                    # TODO: distinguish between UDF agg functions and other UDF functions.
                    self.inside_agg = True
                    self.context.headers.add('"./server/aggregations.h"')
                    if type(val) is list and len(val) > 1:
                        cfunc = self.func_maps[key]
                        cfunc = cfunc[len(val) - 1] if type(cfunc) is list else cfunc
                        self._expr += f"{cfunc}(" 
                        for i, p in enumerate(val):
                            self._expr += expr(self, p)._expr + (','if i<len(val)-1 else '')
                    else:
                        funcname = self.func_maps[key]
                        funcname = funcname[0] if type(funcname) is list else funcname
                        self._expr += f"{funcname}(" 
                        self._expr += expr(self, val)._expr
                    self._expr += ')'
                    self.inside_agg = False
                elif key in self.binary_ops:
                    l = expr(self, val[0])._expr
                    r = expr(self, val[1])._expr
                    self._expr += f'({l}{self.binary_ops[key]}{r})'
                elif key in self.compound_ops:
                    x = []
                    if type(val) is list:
                        for v in val:
                            x.append(expr(self, v)._expr)
                    self._expr = self.compound_ops[key][1](x)
                elif key in self.unary_ops:
                    self._expr += f'{self.unary_ops[key]}({expr(self, val)._expr})'
                else:
                    self.context.Error(f'Undefined expr: {key}{val}')

                if key in self.coumpound_generating_ops and not self.is_compound:
                    self.is_compound = True
                    p = self.parent
                    while type(p) is expr and not p.is_compound:
                        p.is_compound = True
                        p = p.parent

        elif type(node) is str:
            p = self.parent
            while type(p) is expr and not p.isvector:
                p.isvector = True
                p = p.parent
                
            self._expr, self.raw_col = self.datasource.parse_tablenames(node, self.materialize_cols, True)
            self.raw_col = self.raw_col if type(self.raw_col) is ColRef else None
            if self.__abs and self.raw_col:
                self._expr = self.raw_col.reference() + ("" if self.inside_agg else index_expr)
        elif type(node) is bool:
            self._expr = '1' if node else '0'
        else:
            self._expr = f'{node}'
    def toCExpr(_expr):
        return lambda x = None, y = None : eval(start_expr + _expr + end_expr)
    def consume(self, _):
        self.cexpr = expr.toCExpr(self._expr)
    def __str__(self):
        return self.cexpr