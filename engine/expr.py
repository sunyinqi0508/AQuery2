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
        self.k9expr = ''
        self.func_maps = {**self.udf_map, **self.builtin_func_maps}

    def produce(self, node):
        if type(node) is dict:
            for key, val in node.items():
                if key in self.func_maps:
                    # if type(val) in [dict, str]:
                    if type(val) is list and len(val) > 1:
                        k9func = self.func_maps[key]
                        k9func = k9func[len(val) - 1] if type(k9func) is list else k9func
                        self.k9expr += f"{k9func}[" 
                        for i, p in enumerate(val):
                            self.k9expr += expr(self, p).k9expr + (';'if i<len(val)-1 else '')
                    else:
                        funcname = self.func_maps[key]
                        funcname = funcname[0] if type(funcname) is list else funcname
                        self.k9expr += f"{funcname}[" 
                        self.k9expr += expr(self, val).k9expr
                    self.k9expr += ']'
                elif key in self.binary_ops:
                    l = expr(self, val[0]).k9expr
                    r = expr(self, val[1]).k9expr
                    self.k9expr += f'({l}{self.binary_ops[key]}{r})'
                elif key in self.compound_ops:
                    x = []
                    if type(val) is list:
                        for v in val:
                            x.append(expr(self, v).k9expr)
                    self.k9expr = self.compound_ops[key][1](x)
                elif key in self.unary_ops:
                    self.k9expr += f'({expr(self, val).k9expr}{self.unary_ops[key]})'
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
            self.k9expr = self.datasource.parse_tablenames(node, self.materialize_cols)
        elif type(node) is bool:
            self.k9expr = '1' if node else '0'
        else:
            self.k9expr = f'{node}'
    def __str__(self):
        return self.k9expr