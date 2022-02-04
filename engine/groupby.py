from engine.ast import ast_node
from engine.utils import base62uuid
from engine.expr import expr

class groupby(ast_node):
    name = '_groupby'
    def init(self, _):
        self.group = 'g' + base62uuid(7)
        self.datasource = self.parent.datasource
        self.datasource.rec = []
    def produce(self, node):
        if type(node) is not list:
            node = [node]
        g_contents = '('
        
        for i, g in enumerate(node):
            v = g['value']
            e = expr(self, v).k9expr
            # if v is compound expr, create tmp cols
            if type(v) is not str:
                tmpcol = 't' + base62uuid(7)
                self.emit(f'{tmpcol}:{e}')
                e = tmpcol

            g_contents += e + (';'if i < len(node)-1 else '')
            
        self.emit(f'{self.group}:'+g_contents+')')
        
        if len(node) <= 1:
            self.emit(f'{self.group}:={self.group}')
        else:
            self.emit(f'{self.group}:groupby[{self.group}[0];+{self.group}]')
    
    def consume(self, _):
        self.referenced = self.datasource.rec
        self.datasource.rec = None
        return super().consume(_)