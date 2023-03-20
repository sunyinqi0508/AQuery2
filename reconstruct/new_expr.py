import abc
from reconstruct.ast import ast_node
from typing import Optional 
from reconstruct.storage import Context, ColRef
from common.utils import enlist
from common.types import builtin_func, user_module_func, builtin_operators


class expr_base(ast_node, metaclass = abc.ABCMeta):
    def __init__(self, parent: Optional["ast_node"], node, context: Optional[Context] = None):
        self.node = node
        super().__init__(parent, node, context)
        
    def init(self, node):
        self.is_literal = False
        self.udf_map = self.context.udf_map
        self.func_maps = {**builtin_func, **self.udf_map, **user_module_func}
        self.operators = {**builtin_operators, **self.udf_map, **user_module_func}
        self.narrow_funcs = ['sum', 'avg', 'count', 'min', 'max', 'last', 'first']
 
    def get_variable(self):
        pass
    def str_literal(self, node):
        pass
    def int_literal(self, node):
        pass
    def bool_literal(self, node):
        pass
    
    def get_literal(self, node):
        if not self.get_variable(node):
            self.is_literal = True
            if type(node) is str:
                self.str_literal(node)
            elif type(node) is int:
                self.int_literal(node)
            elif type(node) is float:
                self.float_literal(node)
            elif type(node) is bool:
                self.bool_literal(node)
                
    def process_child_nodes(self):
        if not hasattr(self, 'child_exprs'):
            raise ValueError(f'Internal Error: process_child_nodes called without child_exprs.')
    
    def process_non_operator(self, key, value):
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
                    self.child_exprs = [self.__class__(self, v) for v in val]
                    self.process_child_nodes()
                else:
                    self.process_non_operator(key, val)
        else:
            self.get_literal(node['literal'])
                    
    def consume(self, _):
        pass
    
    
class c_expr(expr_base):
    def init(self, node):
        super().init(node)
        self.ccode = ''
        
    def emit(self, snippet : str):
        self.ccode += snippet
        
    def eval(self):
        return self.ccode

class sql_expr(expr_base):
    def init(self):
        super().init()
        self.sql = ''
        
    def emit(self, snippet):
        self.sql += snippet
        
    def eval(self):
        return self.sql

class udf_expr(c_expr):
    pass

class proj_expr(c_expr, sql_expr):
    def init(self, node):
        super(c_expr).init()
        super(sql_expr).init()

class orderby_expr(c_expr, sql_expr):
    def init(self, node):
        super(c_expr).init()
        super(sql_expr).init()

class groupby_expr(orderby_expr):
    pass

class from_expr(sql_expr):
    pass

class where_expr(sql_expr):
    pass


