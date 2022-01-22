from engine.ast import ast_node


class expr(ast_node):
    name='expr'
    builtin_func_maps = {
        'max': 'max',
        'min': 'min', 
        'avg':'avg',
        'sum':'sum',

    }
    binary_ops = {
        'sub':'-', 
        'add':'+', 
        'mul':'*', 
        'div':'%',

    }
    unary_ops = {
        'neg' : '-',
        
    }
    def __init__(self, parent, node):
        from engine.projection import projection
        if type(parent) in [projection, expr]:
            self.datasource = parent.datasource
        else:
            self.datasource = None
        self.udf_map = parent.context.udf_map
        self.k9expr = ''
        self.func_maps = {**self.udf_map, **self.builtin_func_maps}
        ast_node.__init__(self, parent, node, None)

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
            self.k9expr = self.datasource.parse_tablenames(node)
        else:
            self.k9expr = f'{node}'
    def __str__(self):
        return self.k9expr