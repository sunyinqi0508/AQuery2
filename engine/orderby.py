from engine.ast import ColRef, TableInfo, View, ast_node, Context
from engine.utils import base62uuid, seps
from engine.expr import expr
import k

class order_item:
    def __init__(self, name, node, order = True):
        self.name = name
        self.order = order
        self.node = node
        self.materialized = False
        
    def materialize(self):
        if not self.materialized:
            self.name = expr(self.node, self.name, False).k9expr
            self.materialized = True
        return ('' if self.order else '-') + f'({self.name})'
        
    def __str__(self):
        return self.materialize()
    def __repr__(self):
        return self.__str__()

class orders:
    def __init__(self, node, datasource):
        self.order_items = []
        self.materialized = False
        self.view = None
        self.node = node 
        self.datasource = datasource
        self.n_attrs = -1

    def materialize(self):
        if not self.materialized:
            self.view = View(self.node.context, self.datasource, False)
            keys = ';'.join([f'{o}' for o in self.order_items])
            self.n_attrs = len(self.order_items)
            self.node.emit(f"{self.view.name}: > +`j (({',' if self.n_attrs == 1 else ''}{keys}))")
            self.materialized = True

    def append(self, o):
        self.order_items.append(o)

class orderby(ast_node):
    name = '_orderby'
    
    def init(self, _):
        self.datasource = self.parent.datasource
        self.order = orders(self, self.datasource)
        self.view = ''
    def produce(self, node):
        if type(node) is not list:
            node = [node]
        for n in node:
            order =  not ('sort' in n and n['sort'] == 'desc')
            self.order.append(order_item(n['value'], self, order))

    def consume(self, _):
        self.datasource.order.append(self.order)