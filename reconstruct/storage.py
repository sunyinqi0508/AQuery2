from engine.types import *
from engine.utils import enlist
class ColRef:
    def __init__(self, _ty, cobj, table:'TableInfo', name, id, compound = False, _ty_args = None):
        self.type : Types = AnyT
        if type(_ty) is str:
            self.type = builtin_types[_ty.lower()]
            if _ty_args:
                self.type = self.type(enlist(_ty_args))
        elif type(_ty) is Types:
            self.type = _ty
        self.cobj = cobj
        self.table = table
        self.name = name
        self.alias = set()
        self.id = id # position in table
        self.compound = compound # compound field (list as a field) 
        # e.g. order by, group by, filter by expressions
        
        self.__arr__ = (_ty, cobj, table, name, id)
    def __getitem__(self, key):
        if type(key) is str:
            return getattr(self, key)
        else:
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
        # keep track of temp vars
        self.rec = None 
        self.add_cols(cols)
        # runtime
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map

    def add_cols(self, cols, new = True):
        for i, c in enumerate(cols):
            self.add_col(c, new, i)
            
    def add_col(self, c, new = True, i = 0):
        _ty = c['type']
        _ty_args = None
        if type(_ty) is dict:
            _ty_val = list(_ty.keys())[0]
            _ty_args = _ty[_ty_val]
            _ty = _ty_val
        if new:
            col_object = ColRef(_ty, c, self, c['name'], len(self.columns), _ty_args = _ty_args)
        else:
            col_object = c
            c.table = self
        self.columns_byname[c['name']] = col_object
        self.columns.append(col_object)
        
    def add_alias(self, alias):
        if alias in self.cxt.tables_byname.keys():
            print("Error: table alias already exists")
            return
        self.cxt.tables_byname[alias] = self
        self.alias.add(alias)
        
    def parse_col_names(self, colExpr) -> ColRef:
        parsedColExpr = colExpr.split('.')
        if len(parsedColExpr) <= 1:
            col = self.columns_byname[colExpr]
            if type(self.rec) is set:
                self.rec.add(col)
            return col
        else:
            datasource = self.cxt.tables_byname[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                return datasource.parse_col_names(parsedColExpr[1])
            
            
class Context:
    def new(self):
        self.headers = set(['\"./server/libaquery.h\"', 
                            '\"./server/monetdb_conn.h\"'])
        self.ccode = ''
        self.sql = ''  
        self.finalized = False
        self.udf = None
        self.scans = []
        self.procs = []
        self.queries = []
        
    def __init__(self):
        self.tables_byname = dict()
        self.col_byname = dict()
        self.tables = []
        self.cols = []
        self.datasource = None
        self.module_stubs = ''
        self.module_map = {}
        self.udf_map = dict()
        self.udf_agg_map = dict()
        self.use_columnstore = False
        self.print = print
        self.has_dll = False
        self.dialect = 'MonetDB'
        self.have_hge = False
        self.Error = lambda *args: print(*args)
        self.Info = lambda *_: None

    def emit(self, sql:str):
        self.sql += sql + ' '
    def emitc(self, c:str):
        self.ccode += c + '\n'    
    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl
    def remove_scan(self, scan, str_scan):
        self.emitc(str_scan)
        self.scans.remove(scan)
        
    function_deco = '__AQEXPORT__(int) '
    function_head = ('(Context* cxt) {\n' +
        '\tusing namespace std;\n' +
        '\tusing namespace types;\n' + 
        '\tauto server = static_cast<Server*>(cxt->alt_server);\n')
    
    udf_head = ('#pragma once\n'
        '#include \"./server/libaquery.h\"\n'
        '#include \"./server/aggregations.h\"\n\n'
        )
    
    def get_init_func(self):
        if not self.module_map:
            return ''
        ret = '__AQEXPORT__(void) __builtin_init_user_module(Context* cxt){\n'
        for fname in self.module_map.keys():
            ret += f'{fname} = (decltype({fname}))(cxt->get_module_function("{fname}"));\n'
        self.queries.insert(0, f'P__builtin_init_user_module')
        return ret + '}\n'
    
    def sql_begin(self):
        self.sql = ''

    def sql_end(self):
        if self.sql.strip():
            self.queries.append('Q' + self.sql)
        self.sql = ''
        
    def postproc_begin(self, proc_name: str):
        self.ccode = self.function_deco + proc_name + self.function_head
    
    def postproc_end(self, proc_name: str):
        self.procs.append(self.ccode + 'return 0;\n}')
        self.ccode = ''
        self.queries.append('P' + proc_name)    
    
    def finalize_udf(self):
        if self.udf is not None:
            return (Context.udf_head 
                + self.module_stubs
                + self.get_init_func()
                + self.udf
            )
        else:
            return None
    
    def finalize(self):
        if not self.finalized:
            headers = ''
            for h in self.headers:
                if h[0] != '"':
                    headers += '#include <' + h + '>\n'
                else:
                    headers += '#include ' + h + '\n'
            self.ccode = headers + '\n'.join(self.procs)
            self.headers = set()
        return self.ccode
