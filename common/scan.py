from xmlrpc.client import Boolean
from common.ast import ColRef, TableInfo, View, ast_node, Context
from common.utils import base62uuid
from common.expr import expr

class scan(ast_node):
    name = 'scan'
    def __init__(self, parent: "ast_node", node, size = None, context: Context = None, const = False):
        self.type = type
        self.size = size
        self.const = "const " if const else ""
        super().__init__(parent, node, context)
    def init(self, _):
        self.datasource = self.context.datasource
        self.initializers = ''
        self.start = ''
        self.front = ''
        self.body = ''
        self.end = '}'
        self.mode = None
        self.filters = []
        scan_vars = set(s.it_var for s in self.context.scans)
        self.it_var = 'i' + base62uuid(2)
        while(self.it_var in scan_vars):
            self.it_var = 'i' + base62uuid(6)
        self.parent.context.scans.append(self)
    def produce(self, node):
        if type(node) is ColRef:
            self.colref = node
            if self.size is None:
                self.mode = ["col", node.table]
                self.start += f'for ({self.const}auto& {self.it_var} : {node.reference()}) {{\n'
            else:
                self.mode = ["idx", node.table]
                self.start += f"for (uint32_t {self.it_var} = 0; {self.it_var} < {node.reference()}.size; ++{self.it_var}){{\\n"
        elif type(node) is str:
            self.mode = ["idx", None]
            self.start+= f'for({self.const}auto& {self.it_var} : {node}) {{\n'
        else:
            self.mode = ["idx", node] # Node is the TableInfo
            self.start += f"for (uint32_t {self.it_var} = 0; {self.it_var} < {self.size}; ++{self.it_var}){{\n"
            
    def add(self, stmt, position = "body"):
        if position == "body":
            self.body += stmt + '\n'
        elif position == "init":
            self.initializers += stmt + '\n'
        else:
            self.front += stmt + '\n'
            
    def finalize(self):
        for f in self.filters:
            self.start += f
            self.end += '}'
        self.context.remove_scan(self, self.initializers + self.start + self.front + self.body + self.end)
        
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
    
    def finalize(self):
        self.scanner.finalize()
    def consume(self, _):
        # TODO: optimizations after converting expr to cnf
        self.scanner = None
        for s in self.context.scans:
            if self.datasource == s.mode[1]:
                self.scanner = s
                break
        if self.scanner is None:
            self.scanner = scan(self, self.datasource, self.datasource.get_size())
        self.expr = expr(self, self.modified_node)
        self.scanner.filters.append(f'if ({self.expr.cexpr(self.scanner.it_var)}) {{\n')
    