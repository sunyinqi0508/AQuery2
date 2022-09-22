import abc
from reconstruct.ast import ast_node
from typing import Optional 
from reconstruct.storage import Context, ColRef
from engine.utils import enlist
from engine.types import builtin_func, user_module_func, builtin_operators


class expr_base(ast_node, metaclass = abc.ABCMeta):
    def __init__(self, parent: Optional["ast_node"], node, context: Optional[Context] = None):
        self.node = node
        super().__init__(parent, node, context)
        
    def init(self, node):
        self.is_literal = False
        self.udf_map = self.context.udf_map
        self.func_maps = {**builtin_func, **self.udf_map, **user_module_func}
        self.operators = {**builtin_operators, **self.udf_map, **user_module_func}
        self.narrow_funcs = ['sum', 'avg', 'count', 'min', 'max', 'last']
 
    def get_literal(self, node):
        self.is_literal = True
    
    def process_child_nodes(self):
        pass
    def produce(self, node):
        from reconstruct.ast import udf
        if node and type(node) is dict:
            if 'litral' in node:
                self.get_literal(node['literal'])
            else:
                if len(node) > 1:
                    raise ValueError(f'Parse Error: more than 1 entry in {node}.')
                key, val = next(iter(node.items()))
                if key in self.operators:
                    self.child_exprs = [__class__(self, v) for v in val]
                    self.process_child_nodes()
    def consume(self, _):
        pass
    
    
class c_expr(expr_base):
    pass

class sql_expr(expr_base):
    pass

class udf_expr(c_expr):
    pass

class proj_expr(c_expr, sql_expr):
    pass

class orderby_expr(c_expr, sql_expr):
    pass

class groupby_expr(orderby_expr):
    pass

class from_expr(sql_expr):
    pass

class where_expr(sql_expr):
    pass


