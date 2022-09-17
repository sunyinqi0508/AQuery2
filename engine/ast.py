from engine.utils import base62uuid
from copy import copy
from typing import *
# replace column info with this later.
class ColRef:
    def __init__(self, cname, _ty, cobj, cnt, table:'TableInfo', name, id, compound = False):
        self.cname = cname # column object location
        self.cxt_name = None # column object in context
        self.type = _ty
        self.cobj = cobj
        self.cnt = cnt
        self.table = table
        self.name = name
        self.id = id # position in table
        self.order_pending = None # order_pending
        self.compound = compound # compound field (list as a field) 
        self.views = []
        self.aux_columns = [] # columns for temperary calculations 
        # e.g. order by, group by, filter by expressions
        
        self.__arr__ = (cname, _ty, cobj, cnt, table, name, id)
    
    def reference(self):
        cxt = self.table.cxt
        self.table.reference()
        if self not in cxt.columns_in_context:
            counter = 0
            base_name = self.table.table_name + '_' + self.name
            if base_name in cxt.columns_in_context.values():
                while (f'{base_name}_{counter}') in cxt.columns_in_context.values():
                    counter += 1
                base_name = f'{base_name}_{counter}'
            self.cxt_name = base_name
            cxt.columns_in_context[self] = base_name
            # TODO: change this to cname;
            cxt.emit(f'auto& {base_name} = *(ColRef<{self.type}> *)(&{self.table.cxt_name}->colrefs[{self.id}]);')
        elif self.cxt_name is None:
            self.cxt_name = cxt.columns_in_context[self]
            
        return self.cxt_name
             
    def __getitem__(self, key):
        if type(key) is str:
            return getattr(self, key)
        else:
            return self.__arr__[key]

    def __setitem__(self, key, value):
        self.__arr__[key] = value

    def __str__(self):
        return self.reference()
    def __repr__(self):
        return self.reference()

class TableInfo:
    
    def __init__(self, table_name, cols, cxt:'Context'):
        # statics
        self.table_name = table_name
        self.alias = set([table_name])
        self.columns_byname = dict() # column_name, type
        self.columns = []
        self.cxt = cxt
        self.cxt_name = None
        self.views = set()
        #keep track of temp vars
        self.local_vars = dict()
        self.rec = None # a hook on get_col_d to record tables being referenced in the process
        self.groupinfo = None
        self.add_cols(cols)
        # runtime
        self.n_rows = 0 # number of cols
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map
    def reference(self):
        if self not in self.cxt.tables_in_context:
            counter = 0
            base_name = self.table_name
            if base_name in self.cxt.tables_in_context.values():
                while (f'{base_name}_{counter}') in self.cxt.tables_in_context.values():
                    counter += 1
                base_name = f'{base_name}_{counter}'
            self.cxt_name = base_name
            self.cxt.tables_in_context[self] = base_name
            
            type_tags = '<'
            for c in self.columns:
                type_tags += c.type + ','
            if type_tags.endswith(','):
                type_tags = type_tags[:-1]
            type_tags += '>'
            
            self.cxt.emit(f'auto& {base_name} = *(TableInfo{type_tags} *)(cxt->tables["{self.table_name}"]);')
        return self.cxt_name
    def refer_all(self):
        self.reference()
        for c in self.columns:
            c.reference()
    def add_cols(self, cols, new = True):
        for i, c in enumerate(cols):
            self.add_col(c, new, i)
    def add_col(self, c, new = True, i = 0):
        _ty = c['type']
        if new:
            cname =f'get<{i}>({self.table_name})'
            _ty = _ty if type(c) is ColRef else list(_ty.keys())[0]
            col_object =  ColRef(cname, _ty, c, 1, self,c['name'], len(self.columns))
        else:
            col_object = c
            cname = c.cname
            c.table = self
        self.cxt.ccols_byname[cname] = col_object
        self.columns_byname[c['name']] = col_object
        self.columns.append(col_object)
    def get_size(self):
        size_tmp = 'tmp_sz_'+base62uuid(6)
        self.cxt.emit(f'const auto& {size_tmp} = {self.columns[0].reference()}.size;')
        return size_tmp
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
        if type(self.rec) is set:
            self.rec.add(col)
        return col

    def get_ccolname_d(self, col_name):
        return self.get_col_d(col_name).cname
        
    def get_col(self, col_name):
        self.materialize_orderbys()
        col = self.get_col_d(col_name)
        if type(col.order_pending) is str:
            self.cxt.emit_no_flush(f'{col.cname}:{col.cname}[{col.order_pending}]')
            col.order_pending = None
        return col
    def get_ccolname(self, col_name):
        return self.get_col(col_name).cname

    def add_alias(self, alias):
        # TODO: Scoping of alias should be constrainted in the query.
        if alias in self.cxt.tables_byname.keys():
            print("Error: table alias already exists")
            return
        self.cxt.tables_byname[alias] = self
        self.alias.add(alias)
        
    def parse_col_names(self, colExpr, materialize = True, raw = False):
        # get_col = self.get_col if materialize else self.get_col_d

        parsedColExpr = colExpr.split('.')
        ret = None
        if len(parsedColExpr) <= 1:
            ret = self.get_col_d(colExpr)
        else: 
            datasource = self.cxt.tables_byname[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                ret = datasource.parse_col_names(parsedColExpr[1], raw)
        from engine.expr import index_expr
        string = ret.reference() + index_expr
        if self.groupinfo is not None and ret and ret in self.groupinfo.raw_groups:
            string = f'get<{self.groupinfo.raw_groups.index(ret)}>({{y}})'
        return string, ret if raw else string

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
    function_head = '''
    extern "C" int __DLLEXPORT__ dllmain(Context* cxt) { 
        using namespace std;
        using namespace types;
        
    '''
    LOG_INFO = 'INFO'
    LOG_ERROR = 'ERROR'
    LOG_SILENT = 'SILENT'
    from engine.types import Types
    type_table : Dict[str, Types] = dict()
    
    def new(self):
        self.tmp_names = set()
        self.udf_map = dict()
        self.headers = set(['\"./server/libaquery.h\"'])
        self.finalized = False
        # read header
        self.ccode = str()
        self.ccodelet = str()
        with open('header.cxx', 'r') as outfile:
            self.ccode = outfile.read()         
        # datasource will be availible after `from' clause is parsed
        # and will be deactivated when the `from' is out of scope
        self.datasource = None
        self.ds_stack = []
        self.scans = []
        self.removing_scan = False

    def __init__(self): 
        self.tables:list[TableInfo] = []
        self.tables_byname = dict()
        self.ccols_byname = dict()
        self.gc_name = 'gc_' + base62uuid(4)
        self.tmp_names = set()
        self.udf_map = dict()
        self.headers = set(['\"./server/libaquery.h\"'])
        self.finalized = False
        self.log_level = Context.LOG_SILENT
        self.print = print
        # read header
        self.ccode = str()
        self.ccodelet = str()
        self.columns_in_context = dict()
        self.tables_in_context = dict()
        with open('header.cxx', 'r') as outfile:
            self.ccode = outfile.read()         
        # datasource will be availible after `from' clause is parsed
        # and will be deactivated when the `from' is out of scope
        self.datasource = None
        self.ds_stack = []
        self.scans = []
        self.removing_scan = False
    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl

    def gen_tmptable(self):
        from engine.utils import base62uuid
        return f't{base62uuid(7)}'
    def reg_tmp(self, name, f):
        self.tmp_names.add(name)
        self.emit(f"{self.gc_name}.reg({{{name}, 0,0{'' if f is None else ',{f}'}}});")

    def define_tmp(self, typename, isPtr = True, f = None):
        name = 'tmp_' + base62uuid()
        if isPtr:
            self.emit(f'auto* {name} = new {typename};')
            self.reg_tmp(name, f)
        else:
            self.emit(f'auto {name} = {typename};')
        return name
    def emit(self, codelet):
        self.ccode += self.ccodelet + codelet + '\n'
        self.ccodelet = ''
    def emit_no_flush(self, codelet):
        self.ccode += codelet + '\n'
    def emit_flush(self):
        self.ccode += self.ccodelet + '\n'
        self.ccodelet = ''
    def emit_nonewline(self, codelet):
        self.ccodelet += codelet

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
    def remove_scan(self, scan, str_scan):
        self.emit(str_scan)
        self.scans.remove(scan)
        
    def Info(self, msg):
        if self.log_level.upper() == Context.LOG_INFO:
            self.print(msg)
    def Error(self, msg):
        if self.log_level.upper() == Context.LOG_ERROR:
            self.print(msg)
        else:
            self.Info(self, msg)
    
    
    def finalize(self):
        if not self.finalized:
            headers = ''
            for h in self.headers:
                if h[0] != '"':
                    headers += '#include <' + h + '>\n'
                else:
                    headers += '#include ' + h + '\n'
            self.ccode = headers + self.function_head + self.ccode + 'return 0;\n}'
            self.headers = set()
        return self.ccode
    def __str__(self):
        self.finalize()
        return self.ccode
    def __repr__(self) -> str:
        return self.__str__()


class ast_node:
    types = dict()
    header = []
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