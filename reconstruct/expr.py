from reconstruct.ast import ast_node
from reconstruct.storage import ColRef, TableInfo


class expr(ast_node):
    name='expr'
    builtin_func_maps = {
        'max': 'MAX',
        'min': 'MIN', 
        'avg': 'AVG',
        'sum': 'SUM',
        'count' : 'COUNT',
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
        'and':' AND ',
        'or':' OR ',
        'xor' : ' XOR ',
        'gt':'>',
        'lt':'<',
        'le':'<=',
        'gt':'>='
    }

    compound_ops = {
    }

    unary_ops = {
        'neg' : '-',
        'not' : ' NOT '
    }
    
    coumpound_generating_ops = ['avgs', 'mins', 'maxs', 'sums'] + \
       list(binary_ops.keys()) + list(compound_ops.keys()) + list(unary_ops.keys() )

    def __init__(self, parent, node):
        self.raw_col = None
        self.inside_agg = False
        if(type(parent) is expr):
            self.inside_agg = parent.inside_agg
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
        self.func_maps = {**self.udf_map, **self.builtin_func_maps}

    def produce(self, node):
        if type(node) is dict:
            for key, val in node.items():
                if key in self.func_maps:
                    # TODO: distinguish between UDF agg functions and other UDF functions.
                    self.inside_agg = True
                    if type(val) is list and len(val) > 1:
                        cfunc = self.func_maps[key]
                        cfunc = cfunc[len(val) - 1] if type(cfunc) is list else cfunc
                        self.sql += f"{cfunc}(" 
                        for i, p in enumerate(val):
                            self.sql += expr(self, p).sql + (',' if i < len(val) - 1 else '')
                    else:
                        funcname = self.func_maps[key]
                        funcname = funcname[0] if type(funcname) is list else funcname
                        self.sql += f"{funcname}(" 
                        self.sql += expr(self, val).sql
                    self.sql += ')'
                    self.inside_agg = False
                elif key in self.binary_ops:
                    l = expr(self, val[0]).sql
                    r = expr(self, val[1]).sql
                    self.sql += f'({l}{self.binary_ops[key]}{r})'
                elif key in self.compound_ops:
                    x = []
                    if type(val) is list:
                        for v in val:
                            x.append(expr(self, v).sql)
                    self.sql = self.compound_ops[key][1](x)
                elif key in self.unary_ops:
                    self.sql += f'{self.unary_ops[key]}({expr(self, val).sql})'
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
            
            self.raw_col = self.datasource.parse_col_names(node)
            self.raw_col = self.raw_col if type(self.raw_col) is ColRef else None
            if self.raw_col is not None:
                self.sql = self.raw_col.name
            else:
                self.sql = node
                
        elif type(node) is bool:
            self.sql = '1' if node else '0'
        else:
            self.sql = f'{node}'
            
    def __str__(self):
        return self.sql
    def __repr__(self):
        return self.__str__()
    
    