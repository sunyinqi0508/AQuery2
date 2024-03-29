from typing import Dict, List, Optional, Set

from common.types import *
from common.utils import CaseInsensitiveDict, base62uuid, enlist


class ColRef:
    def __init__(self, _ty, cobj, table:'TableInfo', name, id, compound = False, _ty_args = None):
        self.type : Types = AnyT
        if type(_ty) is str:
            self.type = Types.decode(_ty)
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
        self.cxt_name = ''
        # e.g. order by, group by, filter by expressions
        
        self.__arr__ = (_ty, cobj, table, name, id)
        
    def get_name(self):
        it_alias = iter(self.alias)
        alias = next(it_alias, self.name)
        try:
            while alias == self.name:
                alias = next(it_alias)
        except StopIteration:
            alias = self.name
        return alias
    
    def get_full_name(self):
        table_name = self.table.table_name
        it_alias = iter(self.table.alias)
        alias = next(it_alias, table_name)
        try:
            while alias == table_name:
                alias = next(it_alias)
        except StopIteration:
            alias = table_name
        return f'{alias}.{self.get_name()}'
    
    def rename(self, name):
        self.alias.discard(self.name)
        self.table.columns_byname.pop(self.name, None)
        self.name = name
        self.table.columns_byname[name] = self
        
        return self
    
    def __getitem__(self, key):
        if type(key) is str:
            return getattr(self, key)
        else:
            return self.__arr__[key]

    def __setitem__(self, key, value):
        self.__arr__[key] = value

class TableInfo:
    def __init__(self, table_name, cols, cxt:'Context'):
        from engine.ast import create_trigger
        # statics
        self.table_name : str = table_name
        self.contextname_cpp : str = ''
        self.alias : Set[str] = set([table_name])
        self.columns_byname : CaseInsensitiveDict[str, ColRef] = CaseInsensitiveDict() # column_name, type
        self.columns : List[ColRef] = []
        self.triggers : Set[create_trigger] = set()
        self.cxt = cxt
        self.cached = False
        # keep track of temp vars
        self.rec = None 
        self.add_cols(cols)
        # runtime
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map
        cxt.tables.add(self)
        
    def add_cols(self, cols, new = True):
        for c in enlist(cols):
            self.add_col(c, new)
        
    def add_col(self, c, new = True):
        _ty = c['type']
        _ty_args = None
        if type(_ty) is dict:
            _ty_val = list(_ty.keys())[0]
            _ty_args = _ty[_ty_val]
            _ty = _ty_val
        if new or type(c) is not ColRef:
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
    
    def rename(self, name):
        if name in self.cxt.tables_byname.keys():
            print(f"Error: table name {name} already exists")
            return
        
        self.cxt.tables_byname.pop(self.table_name, None)
        self.alias.discard(self.table_name)
        self.table_name = name
        self.cxt.tables_byname[name] = self
        self.alias.add(name)
        
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
    
    def all_cols(self, ordered = False):
        from ordered_set import OrderedSet
        _ret_set_t = OrderedSet if ordered else set
        if type(self.rec) is set:
            self.rec.update(self.columns)
        return _ret_set_t(self.columns)
            
    @property
    def single_table(self):
        return True
    
class Context:
    def new(self):
        self.headers = set(['\"./server/monetdb_conn.h\"'])
        self.ccode = ''

        self.sql = ''  
        self.finalized = False
        self.udf = None
        self.module_stubs = ''
        self.scans = []
        self.procs = []
        self.queries = []
        self.module_init_loc = 0
        self.special_gb = False
        self.has_dll = False
        self.triggers_active.clear()
        self.has_payload = True
        
    def __init__(self, state = None):
        from prompt import PromptState
        from .ast import create_trigger
        from aquery_config import compile_use_gc
        self.tables_byname : Dict[str, TableInfo] = dict()
        self.col_byname = dict()
        self.tables : Set[TableInfo] = set()
        self.cols = []
        self.datasource = None
        self.module_map = {}
        self.udf_map = dict()
        self.udf_agg_map = dict()
        self.use_columnstore = False
        self.print = print
        self.dialect = 'MonetDB'
        self.is_msvc = False
        self.have_hge = False
        self.Error = lambda *args: print(*args)
        self.Info = lambda *_: None
        self.triggers : Dict[str, create_trigger] = dict()
        self.triggers_active = set()
        self.stored_proceudres = dict()
        self.force_compiled = False
        self.use_gc = compile_use_gc 
        self.system_state: Optional[PromptState] = state 
        self.use_cached_tables = True
        self.use_omp_simd = True
        # self.new() called everytime new query batch is started

    def get_scan_var(self):
        it_var = 'i' + base62uuid(2)
        scan_vars = set(s.it_var for s in self.scans)
        while(it_var in scan_vars):
            it_var = 'i' + base62uuid(6)
        return it_var
    
    def emit(self, sql:str):
        self.sql += sql + ' '
    def emitc(self, c:str):
        self.ccode += c + '\n'    
    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.add(tbl)
        return tbl
    def remove_scan(self, scan, str_scan):
        self.emitc(str_scan)
        self.scans.remove(scan)
        
    function_deco = '__AQEXPORT__(int) '
    function_head = ('(Context* cxt) {\n' +
        '\tusing namespace std;\n' +
        '\tusing namespace types;\n' + 
        '\tauto server = static_cast<DataSource*>(cxt->curr_server);\n'
        '\tauto timer = chrono::high_resolution_clock::now();\n'
        
        )
    
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
        self.queries.insert(self.module_init_loc, 'P__builtin_init_user_module')
        return ret + '}\n'
    
    def finalize_query(self):
        # clear aliases
        for t in self.tables:
            for a in t.alias:
                if a != t.table_name:
                    self.tables_byname.pop(a, None)
            t.alias.clear()
            t.alias.add(t.table_name)
    
    def sql_begin(self):
        self.sql = ''

    def sql_end(self):
        # eliminate empty queries
        s = self.sql.strip()
        while(s and s[-1] == ';'):
            s = s[:-1].strip()
        if s and s.lower() != 'select':
            self.queries.append('Q' + self.sql)
        self.sql = ''
        
    def postproc_begin(self, proc_name: str):
        self.ccode = self.function_deco + proc_name + self.function_head
    
    def postproc_end(self, proc_name: str):
        self.procs.append(self.ccode + 'return 0;\n}')
        self.ccode = ''
        self.queries.append('P' + proc_name)    
        self.finalize_query()
        
    def abandon_query(self):
        self.sql = ''
        self.ccode = ''
        self.finalize_query()
    
    def direct_output(self, limit = -1, sep = ' ', end = '\n'):
        from common.utils import encode_integral
        if type(limit) is not int or limit > 2**32 - 1 or limit < 0:
            limit = 2**32 - 1
        limit = encode_integral(limit)
        self.queries.append(
            'O' + limit + sep + end)
        
    def remove_trigger(self, name : str):
        from engine.ast import create_trigger
        val = self.triggers.pop(name, None)
        if val.type == create_trigger.Type.Callback:
            val.table.triggers.remove(val)
        val.remove()

    def post_exec_triggers(self):
        for t in self.triggers_active:
            t.execute()
        self.triggers_active.clear()

    def abandon_postproc(self):
        self.ccode = ''
        self.finalize_query()
    
    def finalize_udf(self):
        if self.udf:
            self.udf += '\n'.join([
                u.ccode for u in self.udf_map.values()
            ])
            self.module_stubs = '\n'.join(
                [m for m in self.module_map.values()
            ])
            return (Context.udf_head
                + self.module_stubs
                + self.get_init_func()
                + self.udf
            )
        else:
            return None
    
    def finalize(self):
        from aquery_config import build_driver, os_platform
        if not self.finalized:
            headers = ''
            # if build_driver == 'MSBuild':
                # headers ='#include \"./server/pch.hpp\"\n'
            with open('header.cxx', 'r') as header:
                headers += header.read() 
            for h in self.headers:
                if h[0] != '"':
                    headers += '#include <' + h + '>\n'
                else:
                    headers += '#include ' + h + '\n'
            if os_platform == 'win':
                headers += '#undef max\n'
                headers += '#undef min\n'

            self.ccode += headers + '\n'.join(self.procs)
            self.headers = set()
        return self.ccode

    @property
    def omp_simd(self):
        if self.use_omp_simd: 
            return '#pragma omp simd\n'
        else: 
            return ''
