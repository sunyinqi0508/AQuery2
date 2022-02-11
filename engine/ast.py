from typing import List

from engine.utils import base62uuid

# replace column info with this later.
class ColRef:
    def __init__(self, k9name, _ty, cobj, cnt, table, name, id, order = None, compound = False):
        self.k9name = k9name
        self.type = _ty
        self.cobj = cobj
        self.cnt = cnt
        self.table = table
        self.name = name
        self.id = id
        self.order = order # True -> asc, False -> dsc; None -> unordered
        self.compound = compound # compound field (list as a field) 
        self.views = []
        self.__arr__ = (k9name, _ty, cobj, cnt, table, name, id)
        
    def __getitem__(self, key):
        return self.__arr__[key]

    def __setitem__(self, key, value):
        self.__arr__[key] = value

class TableInfo:
    
    def __init__(self, table_name, cols, cxt:'Context'):
        # statics
        self.table_name = table_name
        self.alias = set([table_name])
        self.columns_byname = dict() # column_name, type
        self.columns = []
        self.cxt = cxt
        self.views = set()
        self.rec = None 
        self.groupinfo = None
        for c in cols:
            self.add_col(c)

        # runtime
        self.n_rows = 0 # number of cols
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map
        
    def add_col(self, c):
        if type(c) is ColRef:
            c = c.cobj
        k9name = 'c' + base62uuid(7)
        col_object =  ColRef(k9name, (list(c['type'].keys()))[0], c, 1, self,c['name'], len(self.columns))

        self.cxt.k9cols_byname[k9name] = col_object
        self.columns_byname[c['name']] = col_object
        self.columns.append(col_object)
        
    def construct(self):
        for c in self.columns:
            self.cxt.emit(f'{c.k9name}:()')
    @property
    def n_cols(self):
        return len(self.columns)

    def get_k9colname(self, col_name):
        col = self.columns_byname[col_name]
        if type(self.rec) is list:
            self.rec.append(col)
        return col.k9name
        
    def add_alias(self, alias):
        # TODO: Exception when alias already defined.
        # TODO: Scoping of alias should be constrainted in the query.
        self.cxt.tables_byname[alias] = self
        self.alias.add(alias)
        
    def parse_tablenames(self, colExpr):
        parsedColExpr = colExpr.split('.')
        if len(parsedColExpr) <= 1:
            return self.get_k9colname(colExpr)
        else: 
            datasource = self.cxt.tables_byname[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                return datasource.get_k9colname(parsedColExpr[1])

class View:
    def __init__(self, context, table = None, tmp = True):
        self.table: TableInfo = table
        self.name = 'v'+base62uuid(7)
        if type(table) is TableInfo:
            table.views.add(self)
        self.context = context
         
    def construct(self):
        self.context.emit(f'{self.name}:()')
            
class Context:
    def __init__(self): 
        self.tables:List[TableInfo] = []
        self.tables_byname = dict()
        self.k9cols_byname = dict()
        
        self.udf_map = dict()
        # read header
        self.k9code = ''
        with open('header.k', 'r') as outfile:
            self.k9code = outfile.read()         
        # datasource will be availible after `from' clause is parsed
        # and will be deactivated when the `from' is out of scope
        self.datasource = None


    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl

    def gen_tmptable(self):
        from engine.utils import base62uuid
        return f't{base62uuid(7)}'

    def emit(self, codelet):
        self.k9code += codelet + '\n'
    def emit_nonewline(self, codelet):
        self.k9code += codelet
    def __str__(self):
        return self.k9code

class ast_node:
    types = dict()
    def __init__(self, parent:"ast_node", node, context:Context = None):
        self.context = parent.context if context is None else context
        self.parent = parent
        self.init(node)
        self.produce(node)
        self.spawn(node)
        self.consume(node)
    
    def emit(self, code):
        self.context.emit(code)
    def emit_no_ln(self, code):
        self.context.emit_nonewline(code)

    name = 'null'

    # each ast node has 3 stages. 
    # `produce' generates info for child nodes
    # `spawn' populates child nodes
    # `consume' consumes info from child nodes and finalizes codegen
    # For simple operators, there may not be need for some of these stages
    def init(self, _):
        pass
    def produce(self, _):
        pass
    def spawn(self, _):
        pass
    def consume(self, _):
        pass

# include classes in module as first order operators
def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node) and not cls.name.startswith('_'):
            ast_node.types[cls.name] = cls