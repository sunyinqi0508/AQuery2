from engine.ast import ast_node


class expr(ast_node):
    name='expr'
    builtin_func_maps = {
        'max': 'max',
        'min': 'min', 
        'avg': 'avg',
        'sum': 'sum',
        'mod':'mod',
        'mins': ['mins', 'minsw'],
        'maxs': ['maxs', 'maxsw'],
        'avgs': ['avgs', 'avgsw'],
        'sums': ['sums', 'sumsw'],
    }
    
    binary_ops = {
        'sub':'-',  
        'add':'+', 
        'mul':'*', 
        'div':'%',
        'and':'&',
        'or':'|',
        'gt':'>',
        'lt':'<',
    }

    compound_ops = {
        'ge' : [2, lambda x: f'~({x[0]}<{x[1]})'],
        'le' : [2, lambda x: f'~({x[0]}>{x[1]})'],
        'count' : [1, lambda x: f'#({x[0]})']
    }

    unary_ops = {
        'neg' : '-',
        'not' : '~'
    }
    
    coumpound_generating_ops = ['mod', 'mins', 'maxs', 'sums'] + \
       list( binary_ops.keys()) + list(compound_ops.keys()) + list(unary_ops.keys() )

    def __init__(self, parent, node, materialize_cols = True):
        self.materialize_cols = materialize_cols
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
        self.cexpr = ''
        self.func_maps = {**self.udf_map, **self.builtin_func_maps}

    def produce(self, node):
        if type(node) is dict:
            for key, val in node.items():
                if key in self.func_maps:
                    # if type(val) in [dict, str]:
                    if type(val) is list and len(val) > 1:
                        cfunc = self.func_maps[key]
                        cfunc = cfunc[len(val) - 1] if type(cfunc) is list else cfunc
                        self.cexpr += f"{cfunc}(" 
                        for i, p in enumerate(val):
                            self.cexpr += expr(self, p).cexpr + (';'if i<len(val)-1 else '')
                    else:
                        funcname = self.func_maps[key]
                        funcname = funcname[0] if type(funcname) is list else funcname
                        self.cexpr += f"{funcname}(" 
                        self.cexpr += expr(self, val).cexpr
                    self.cexpr += ')'
                elif key in self.binary_ops:
                    l = expr(self, val[0]).cexpr
                    r = expr(self, val[1]).cexpr
                    self.cexpr += f'({l}{self.binary_ops[key]}{r})'
                elif key in self.compound_ops:
                    x = []
                    if type(val) is list:
                        for v in val:
                            x.append(expr(self, v).cexpr)
                    self.cexpr = self.compound_ops[key][1](x)
                elif key in self.unary_ops:
                    self.cexpr += f'({expr(self, val).cexpr}{self.unary_ops[key]})'
                else:
                    print(f'Undefined expr: {key}{val}')

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
            self.cexpr = self.datasource.parse_tablenames(node, self.materialize_cols)
        elif type(node) is bool:
            self.cexpr = '1' if node else '0'
        else:
            self.cexpr = f'{node}'
    def __str__(self):
        return self.cexpr