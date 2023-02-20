from engine.ast import ColRef, TableInfo, View, ast_node, Context
from engine.utils import base62uuid, seps
from engine.expr import expr

class order_item:
    def __init__(self, name, node, order = True):
        self.name = name
        self.order = order
        self.node = node
        self.materialized = False
        
    def materialize(self):
        if not self.materialized:
            self.name = expr(self.node, self.name, False).cexpr
            self.materialized = True
        return ('' if self.order else '-') + f'({self.name})'
        
    def __str__(self):
        return self.name
    def __repr__(self):
        return self.__str__()

class orderby(ast_node):
    name = '_orderby'
    def __init__(self, parent: "ast_node", node, context: Context = None):
        self.col_list = []
        super().__init__(parent, node, context)
    def init(self, _):
        self.datasource = self.parent.datasource
        self.order = []
        self.view = ''
    def produce(self, node):
        if type(node) is not list:
            node = [node]
        for n in node:
            order =  not ('sort' in n and n['sort'] == 'desc')
            col_id = self.datasource.columns_byname[n['value']].id
            col_id = col_id if order else -col_id-1
            if col_id not in self.col_list:
                self.col_list.append(col_id)
                self.order.append(order_item(n['value'], self, order))
            
    def merge(self, node):
        self.produce(node)
        
    def finalize(self, references):
        self.order = [ o for o in self.order if o.name in references ]
    
    def result(self, sep:str = ','):
        return sep.join([f"{c}" for c in self.col_list])

class assumption(orderby):
    name = '_assumption'
    def __init__(self, parent: "ast_node", node, context: Context = None, exclude = []):
        self.exclude = exclude
        super().__init__(parent, node, context)
    
    def produce(self, node):
        if type(node) is not list:
            node = [node]
        [n for n in node if n not in self.exclude]
        return super().produce(node)
    
    def empty(self):
        return len(self.col_list) == 0