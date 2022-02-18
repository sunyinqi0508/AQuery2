from typing import List

from pyparsing import col

from engine.utils import base62uuid

# replace column info with this later.
class ColRef:
    def __init__(self, k9name, _ty, cobj, cnt, table, name, id, compound = False):
        self.k9name = k9name
        self.type = _ty
        self.cobj = cobj
        self.cnt = cnt
        self.table = table
        self.name = name
        self.id = id
        self.order_pending = None # order_pending
        self.compound = compound # compound field (list as a field) 
        self.views = []
        self.__arr__ = (k9name, _ty, cobj, cnt, table, name, id)
        
    def __getitem__(self, key):
        if type(key) is str:
            return getattr(self, key)
        else:
            return self.__arr__[key]

    def __setitem__(self, key, value):
        self.__arr__[key] = value

    def __str__(self):
        return self.k9name

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
        self.add_cols(cols)
        # runtime
        self.n_rows = 0 # number of cols
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map
    def add_cols(self, cols, new = True):
        for c in cols:
            self.add_col(c, new)
    def add_col(self, c, new = True):
        _ty = c['type']
        if new:
            k9name = 'c' + base62uuid(7)
            _ty = _ty if type(c) is ColRef else list(_ty.keys())[0]
            col_object =  ColRef(k9name, _ty, c, 1, self,c['name'], len(self.columns))
        else:
            col_object = c
            k9name = c.k9name
        self.cxt.k9cols_byname[k9name] = col_object
        self.columns_byname[c['name']] = col_object
        self.columns.append(col_object)
        
    def construct(self):
        for c in self.columns:
            self.cxt.emit(f'{c.k9name}:()')
    @property
    def n_cols(self):
        return len(self.columns)

    def materialize_orderbys(self):
        view_stack = ''
        stack_name = ''
        for o in self.order:
            o.materialize()
            if len(view_stack) == 0:
                view_stack = o.view.name
                stack_name = view_stack
            else:
                view_stack = view_stack+'['+ o.view.name +']'
        # TODO: Optimize by doing everything in a stmt
        if len(view_stack) > 0:
            if len(self.order) > 1:
                self.cxt.emit(f'{stack_name}:{view_stack}')
            for c in self.columns:
                c.order_pending = stack_name
            self.order[0].node.view = stack_name
        self.order.clear()

    def get_col_d(self, col_name):
        col = self.columns_byname[col_name]
        if type(self.rec) is list:
            self.rec.append(col)
        return col

    def get_k9colname_d(self, col_name):
        return self.get_col_d(col_name).k9name
        
    def get_col(self, col_name):
        self.materialize_orderbys()
        col = self.get_col_d(col_name)
        if type(col.order_pending) is str:
            self.cxt.emit_no_flush(f'{col.k9name}:{col.k9name}[{col.order_pending}]')
            col.order_pending = None
        return col
    def get_k9colname(self, col_name):
        return self.get_col(col_name).k9name

    def add_alias(self, alias):
        # TODO: Exception when alias already defined.
        # TODO: Scoping of alias should be constrainted in the query.
        self.cxt.tables_byname[alias] = self
        self.alias.add(alias)
        
    def parse_tablenames(self, colExpr, materialize = True):
        self.get_col = self.get_col if materialize else self.get_col_d

        parsedColExpr = colExpr.split('.')
        ret = None
        if len(parsedColExpr) <= 1:
            ret = self.get_col(colExpr)
        else: 
            datasource = self.cxt.tables_byname[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                ret = datasource.get_col(parsedColExpr[1])
        if self.groupinfo is not None and ret:
            ret = f"{ret.k9name}[{'start' if ret in self.groupinfo.referenced else 'range'}]"
        else:
            ret = ret.k9name
        return ret

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
        self.k9codelet = ''
        with open('header.k', 'r') as outfile:
            self.k9code = outfile.read()         
        # datasource will be availible after `from' clause is parsed
        # and will be deactivated when the `from' is out of scope
        self.datasource = None
        self.ds_stack = []

    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl

    def gen_tmptable(self):
        from engine.utils import base62uuid
        return f't{base62uuid(7)}'

    def emit(self, codelet):
        self.k9code += self.k9codelet + codelet + '\n'
        self.k9codelet = ''
    def emit_no_flush(self, codelet):
        self.k9code += codelet + '\n'
    def emit_flush(self):
        self.k9code += self.k9codelet + '\n'
        self.k9codelet = ''
    def emit_nonewline(self, codelet):
        self.k9codelet += codelet

    def datsource_top(self):
        if len(self.ds_stack) > 0:
            return self.ds_stack[-1]
        else:
            return None
    def datasource_pop(self):
        if len(self.ds_stack) > 0:
            self.ds_stack.pop()
            return self.ds_stack[-1]
        else:
            return None
    def datasource_push(self, ds):
        if type(ds) is TableInfo:
            self.ds_stack.append(ds)
            return ds
        else:
            return None

    def __str__(self):
        return self.k9code
    def __repr__(self) -> str:
        return self.__str__()


class ast_node:
    types = dict()
    def __init__(self, parent:"ast_node", node, context:Context = None):
        self.context = parent.context if context is None else context
        self.parent = parent
        self.datasource = None
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