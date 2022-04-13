from xmlrpc.client import Boolean
from engine.ast import ColRef, TableInfo, View, ast_node, Context
from engine.utils import base62uuid
from engine.expr import expr

class scan(ast_node):
    name = 'scan'
    def __init__(self, parent: "ast_node", node, size = None, context: Context = None):
        self.type = type
        self.size = size
        super().__init__(parent, node, context)
    def init(self, _):
        self.datasource = self.context.datasource
        self.start = ''
        self.body = ''
        self.end = '}'
        self.filter = None
        scan_vars = set(s.it_var for s in self.context.scans)
        self.it_ver = 'i' + base62uuid(2)
        while(self.it_ver in scan_vars):
            self.it_ver = 'i' + base62uuid(6)
        self.parent.context.scans.append(self)
    def produce(self, node):
        if type(node) is ColRef:
            if self.size is None:
                self.start += f'for (auto& {self.it_ver} : {node.reference()}) {{\n'
            else:
                self.start += f"for (uint32_t {self.it_ver} = 0; {self.it_ver} < {node.reference()}.size; ++{self.it_ver}){{\\n"
        elif type(node) is str:
            self.start+= f'for(auto& {self.it_ver} : {node}) {{\n'
        else:
            self.start += f"for (uint32_t {self.it_ver} = 0; {self.it_ver} < {self.size}; ++{self.it_ver}){{\n"
            
    def add(self, stmt):
        self.body+=stmt + '\n'
    def finalize(self):
        self.context.remove_scan(self, self.start + self.body + self.end)
        
class filter(ast_node):
    name = 'filter'
    def __init__(self, parent: "ast_node", node, materialize = False, context = None):
        self.materialize = materialize
        super().__init__(parent, node, context)
    def init(self, _):
        self.datasource = self.context.datasource
        self.view = View(self.context, self.datasource)
        self.value = None
        
    def spawn(self, node):
        # TODO: deal with subqueries
        self.modified_node = node
        return super().spawn(node)
    def __materialize__(self):
        if self.materialize:
            cols = [] if self.datasource is None else self.datasource.columns
            self.output = TableInfo('tn'+base62uuid(6), cols, self.context)
            self.output.construct()
            if type(self.value) is View: # cond filtered on tables.
                self.emit(f'{self.value.name}:&{self.value.name}')
                for o, c in zip(self.output.columns,self.value.table.columns):
                    self.emit(f'{o.cname}:{c.cname}[{self.value.name}]')
            elif self.value is not None: # cond is scalar
                tmpVar = 't'+base62uuid(7)
                self.emit(f'{tmpVar}:{self.value}')
                for o, c in zip(self.output.columns, self.datasource.columns):
                    self.emit(f'{o.cname}:$[{tmpVar};{c.cname};()]')
                
    def consume(self, node):
        # TODO: optimizations after converting expr to cnf
        self.expr = expr(self, self.modified_node)
        print(node)
    