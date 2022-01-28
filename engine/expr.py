from engine.ast import ast_node


class expr(ast_node):
    name='expr'
    builtin_func_maps = {
        'max': 'max',
        'min': 'min', 
        'avg':'avg',
        'sum':'sum',
        'mins': 'mins',
        'maxs': 'maxs'
    }
    binary_ops = {
        'sub':'-', 
        'add':'+', 
        'mul':'*', 
        'div':'%',
        'mod':'mod',
        'and':'&',
        'or':'|',
        'gt':'>',
        'lt':'<',
    }
    unary_ops = {
        'neg' : '-',
        'not' : '~'
    }
    def __init__(self, parent, node):
        ast_node.__init__(self, parent, node, None)

    def init(self, _):
        from engine.projection import projection
        parent = self.parent
        self.isvector = parent.isvector if type(parent) is expr else False
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
                    self.k9expr += f"{self.func_maps[key]}(" 
                    # if type(val) in [dict, str]:
                    self.k9expr += expr(self, val).k9expr
                    self.k9expr += ')'
                elif key in self.binary_ops:
                    l = expr(self, val[0]).k9expr
                    r = expr(self, val[1]).k9expr
                    self.k9expr += f'({l}{self.binary_ops[key]}{r})'
                    
                elif key in self.unary_ops:
                    self.k9expr += f'({expr(self, val).k9expr}{self.unary_ops[key]})'
                else:
                    print(f'Undefined expr: {key}{val}')
        elif type(node) is str:
            p = self.parent
            while type(p) is expr and not p.isvector:
                p.isvector = True
                p = p.parent
            self.k9expr = self.datasource.parse_tablenames(node)
        elif type(node) is bool:
            self.k9expr = '1' if node else '0'
        else:
            self.k9expr = f'{node}'
    def __str__(self):
        return self.k9expr