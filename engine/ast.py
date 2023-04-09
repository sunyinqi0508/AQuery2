from copy import deepcopy
from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, List, Optional, Set, Tuple, Union

from common.types import *
from common.utils import (base62alp, base62uuid, enlist, 
                          get_innermost, get_legal_name, 
                          Backend_Type, backend_strings)
from engine.storage import ColRef, Context, TableInfo

class ast_node:
    header = []
    types = dict()
    first_order = False
    
    def __init__(self, parent:Optional["ast_node"], node, context:Optional[Context] = None):
        self.context = parent.context if context is None else context
        self.parent = parent
        self.sql = ''
        self.ccode = ''
        if hasattr(parent, 'datasource'):
            self.datasource = parent.datasource
        else:
            self.datasource = None
        self.init(node)
        self.produce(node)
        self.spawn(node)
        self.consume(node)
    
    def emit(self, code):
        self.context.emit(code)
    def add(self, code, sp = ' '):
        self.sql += code + sp
    def addc(self, code):
        self.ccode += code + '\n'

    name = 'null'

    def init(self, _):
        if self.parent is None:
            self.context.sql_begin()
        self.add(self.__class__.name.upper())

    def produce(self, _):
        pass
    def spawn(self, _):
        pass

    def consume(self, _):
        if self.parent is None:
            self.emit(self.sql+';\n')
            self.context.sql_end()
        
from engine.expr import expr, fastscan
class SubqType(Enum):
    WITH = auto()
    FROM = auto()
    PROJECTION = auto()
    FILTER = auto()
    GROUPBY = auto()
    ORDERBY = auto()
    NONE = auto()
class projection(ast_node):
    name = 'projection'
    first_order = 'select'

        
    def __init__(self, 
                 parent : Optional["ast_node"],
                 node, 
                 context : Optional[Context] = None,
                 force_use_spgb : bool = False,
                 subq_type: SubqType = SubqType.NONE
                ):
        self.force_use_spgb = ( 
            force_use_spgb | 
            (context.system_state.cfg.backend_type == 
             Backend_Type.BACKEND_AQuery.value)
        )
        self.subq_type = subq_type
        super().__init__(parent, node, context)
        
    def init(self, _):
        # skip default init
        pass
    
    def produce(self, node):
        self.add('SELECT')
        self.has_postproc = 'into' in node
        if 'select' in node:
            p = node['select']
            self.distinct = False
        elif 'select_distinct' in node:
            p = node['select_distinct']
            self.distinct = True
        else:
            raise NotImplementedError('AST node is not a projection node')
        
        if 'with' in node:
            with_table = node['with']['name']
            with_table_name = tuple(with_table.keys())[0]
            with_table_cols = tuple(with_table.values())[0]
            self.with_clause = projection(self, node['with']['value'], subq_type=SubqType.WITH)
            self.with_clause.out_table.add_alias(with_table_name)
            for new_name, col in zip(with_table_cols, self.with_clause.out_table.columns):
                col.rename(new_name)
            self.with_clause.out_table.contextname_cpp 
            # in monetdb, in cxt 
        else:
            self.with_clause = None
        
        self.limit = None
        if 'limit' in node:
            self.limit = node['limit']
            
        self.projections = p if type(p) is list else [p]
        if self.parent is None:
            self.context.sql_begin()
            self.postproc_fname = 'dll_' + base62uuid(6)
            self.context.postproc_begin(self.postproc_fname)
        
    def spawn(self, node):
        self.datasource = join(self, [], self.context) # datasource is Join instead of TableInfo
        self.assumptions = []
        if 'from' in node:
            from_clause = node['from']['table_source']
            self.datasource = join(self, from_clause)
            if 'assumptions' in node['from']:
                self.assumptions = enlist(node['from']['assumptions'])
        
        if self.datasource is not None:
            self.datasource_changed = True
            self.prev_datasource = self.context.datasource
            self.context.datasource = self.datasource     
                   
        if 'where' in node:
            self.where = filter(self, node['where'])
        else:
            self.where = None    

        if type(self.datasource) is join:
            self.datasource.process_join_conditions()
        
        self.context.special_gb = self.force_use_spgb
        if 'groupby' in node: # if groupby clause contains special stuff
            self.context.special_gb |= groupby.check_special(self, node['groupby'])

    def consume(self, node):
        # deal with projections
        out_table_varname = 'out_'+base62uuid(6)
        if 'into' in node:
            out_table_name = node['into']
        else:
            out_table_name = out_table_varname
            
        self.out_table : TableInfo = TableInfo(out_table_name, [], self.context)
        self.out_table.contextname_cpp = out_table_varname
        
        cols = []
        self.col_ext : Set[ColRef]= set()
        col_exprs : List[Tuple[str, Types]] = []
        
        proj_map : Dict[int, List[Union[Types, int, str, expr]]]= dict()
        self.var_table = dict()
        # self.sp_refs = set()
        i = 0
        for proj in self.projections:
            compound = False
            self.datasource.rec = set()
            name = ''
            this_type = AnyT
            if type(proj) is dict or proj == '*':
                if 'value' in proj:
                    e = proj['value']
                elif proj == '*':
                    e = '*'
                else:
                    print('unknown projection', proj) 
                proj_expr = expr(self, e)
                sql_expr = expr(self, e, c_code=False)
                this_type = proj_expr.type
                name = proj_expr.sql
                compound = [proj_expr.is_compound > 1] # compound column
                proj_expr.cols_mentioned = self.datasource.rec
                alias = ''
                if 'name' in proj: # renaming column by AS keyword
                    alias = proj['name']
                        
                if not proj_expr.is_special:
                    if str(proj_expr.node).strip().endswith('*'):
                        _datasource = self.datasource
                        if '.' in proj_expr.node:
                            tbl = proj_expr.node.split('.')[0]
                            if tbl in self.datasource.tables_dir:
                                _datasource = self.datasource.tables_dir[tbl]
                        _datasource = _datasource.all_cols(ordered = True, stripped = True)
                        name = [(c.get_name()
                                 if self.datasource.single_table
                                 else c.get_full_name()
                                 ) for c in _datasource]
                        this_type = [c.type for c in _datasource]
                        compound = [c.compound for c in _datasource]
                        proj_expr = [expr(self, c.name) for c in _datasource]
                        for pe in proj_expr:
                            if pe.is_ColExpr:
                                pe.cols_mentioned = {pe.raw_col}
                            else:
                                pe.cols_mentioned = set()
                    else:
                        y = lambda x:x
                        count = lambda : 'count(*)'
                        name = enlist(sql_expr.eval(False, y, count=count))
                        this_type = enlist(this_type)
                        proj_expr = enlist(proj_expr)
                    for t, n, pexpr, cp in zip(this_type, name, proj_expr, compound):
                        if cp:
                            # Force postporcessing for compound columns
                            t = VectorT(t)
                            self.has_postproc = True
                        offset = len(col_exprs)
                        if n not in self.var_table:
                            self.var_table[n] = offset
                            if pexpr.is_ColExpr and type(pexpr.raw_col) is ColRef:
                                for _alias in (pexpr.raw_col.table.alias):
                                    self.var_table[f'{_alias}.'+n] = offset
                        proj_map[i] = [t, offset, pexpr]
                        col_expr = n + ' AS ' + alias if alias else n
                        if alias:
                            self.var_table[alias] = offset
                            
                        col_exprs.append((col_expr, t))
                        i += 1
                else:
                    self.context.headers.add('"./server/aggregations.h"')
                    self.has_postproc = True
                    if self.datasource.rec is not None:
                        self.col_ext = self.col_ext.union(self.datasource.rec)
                    proj_map[i] = [this_type, proj_expr.sql, proj_expr]
                    i += 1
                name = enlist(name)
                disp_name = [get_legal_name(alias if alias else n) for n in name]
                this_type = enlist(this_type)
                
            elif type(proj) is str:
                col = self.datasource.get_cols(proj)
                this_type = col.type
                disp_name = proj
                print('Unknown behavior:', proj, 'is str')    
                # name = col.name
            self.datasource.rec = None
            # TODO: Type deduction in Python
            for t, n, c in zip(this_type, disp_name, compound):
                cols.append(ColRef(t, None, self.out_table, n, len(cols), compound=c))
        
        self.out_table.add_cols(cols, new = False)
        
        self.proj_map = proj_map
        
        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
            if self.group_node.terminate:
                self.context.abandon_query()
                projection(self.parent, node, self.context, True, subq_type=self.subq_type)
                return
            if self.group_node.use_sp_gb:
                self.has_postproc = True
        else:
            self.group_node = None

        if not self.has_postproc and self.distinct:
            self.add('DISTINCT')
        self.col_ext = [c for c in self.col_ext if c.name not in self.var_table] # remove duplicates in self.var_table
        col_ext_names = [c.name for c in self.col_ext]
        self.add(', '.join([c[0] for c in col_exprs] + col_ext_names))
    
        _base_offset = len(col_exprs)
        for i, col in enumerate(self.col_ext):
            if col.name not in self.var_table:
                offset = i + _base_offset
                self.var_table[col.name] = offset
                for n in (col.table.alias):
                    self.var_table[f'{n}.'+col.name] = offset
        # monetdb doesn't support select into table
        # if 'into' in node:
        #     self.into_stub = f'{{INTOSTUB{base62uuid(20)}}}'
        #     self.add(self.into_stub, '')
            
        def finalize(astnode:ast_node):
            if(astnode is not None):
                self.add(astnode.sql)
        finalize(self.datasource)                
        finalize(self.where)
        if self.group_node and not self.group_node.use_sp_gb:
            self.add(self.group_node.sql)

        if self.col_ext or self.group_node and self.group_node.use_sp_gb:
            self.has_postproc = True
        
        if self.group_node and self.group_node.use_sp_gb :
            self.group_node.dedicated_glist
            ...
        o = self.assumptions
        if 'orderby' in node:
            o.extend(enlist(node['orderby']))
        if o:
            self.add(orderby(self, o).sql)
        
        if 'outfile' in node:
            self.outfile = outfile(self, node['outfile'], sql = self.sql)
            if not self.has_postproc:
                self.sql = self.outfile.sql
        else:
            self.outfile = None
        
        # reset special_gb in case of subquery.
        self.context.special_gb = False
        if self.parent is None:
            self.emit(self.sql+';\n')
        else: 
            # TODO: subquery, name create tmp-table from subquery w/ alias as name 
            pass
        
        
        # cpp module codegen
        # extract typed-columns from result-set
        vid2cname = [0]*len(self.var_table)
        self.pyname2cname = dict()
        typenames = [c[1] for c in col_exprs] + [c.type for c in self.col_ext]
        length_name = 'len_' + base62uuid(6)
        self.context.emitc(f'auto {length_name} = server->cnt;')
        
        for v, idx in self.var_table.items():
            vname = get_legal_name(v) + '_' + base62uuid(3)
            self.pyname2cname[v] = vname
            self.context.emitc(f'auto {vname} = ColRef<{typenames[idx].cname}>({length_name}, server->getCol({idx}, {typenames[idx].ctype_name}));')
            vid2cname[idx] = vname
        # Create table into context
        out_typenames = [None] * len(proj_map)
        
        def get_proj_name(proj_name):
            if '*' in proj_name:
                lst_names = self.datasource.get_cols(proj_name)
                return ', '.join([self.pyname2cname[n.name] for n in lst_names])
            else:
                return self.pyname2cname[proj_name]
        gb_tovec = [False] * len(proj_map)
        for i, (key, val) in enumerate(proj_map.items()):
            if type(val[1]) is str:
                x = True
                y = get_proj_name
                count = lambda : '0'
                if vid2cname:
                    count = lambda : f'{vid2cname[0]}.size'
                val[1] = val[2].eval(x, y, count=count)
                if callable(val[1]):
                    val[1] = val[1](False)
                
            if val[0] == LazyT:
                decltypestring = val[2].eval(y=y,gettype=True,c_code=True)(True)
                decltypestring = f'value_type<decays<decltype({decltypestring})>>'
                out_typenames[key] = decltypestring
            else:
                out_typenames[key] = val[0].cname
            elemental_ret_udf = (
                    type(val[2].udf_called) is udf and # should bulkret also be colref?
                    val[2].udf_called.return_pattern == udf.ReturnPattern.elemental_return
            )
            folding_vector_groups = (
                self.group_node and 
                (
                    self.group_node.use_sp_gb and
                    val[2].cols_mentioned.intersection(
                        self.datasource.all_cols().difference(
                            self.datasource.get_joint_cols(self.group_node.refs)
                        )
                    )
                ) and 
                val[2].is_compound # compound val not in key
            )
            if (elemental_ret_udf or folding_vector_groups):
                out_typenames[key] = f'vector_type<{out_typenames[key]}>'
                self.out_table.columns[key].compound = True
                if self.group_node is not None and self.group_node.use_sp_gb:
                    gb_tovec[i] = True
        outtable_col_nameslist = ', '.join([f'"{c.name}"' for c in self.out_table.columns])
        self.outtable_col_names = 'names_' + base62uuid(4)
        self.context.emitc(f'const char* {self.outtable_col_names}[] = {{{outtable_col_nameslist}}};')
        # out_typenames = [v[0].cname for v in proj_map.values()]
        self.context.emitc(f'auto {self.out_table.contextname_cpp} = new TableInfo<{",".join(out_typenames)}>("{self.out_table.table_name}", {self.outtable_col_names});')
        # TODO: Inject custom group by code here and flag them in proj_map
        # Type of UDFs? Complex UDFs, ones with static vars?
        if self.group_node is not None and self.group_node.use_sp_gb:
            gb_vartable : Dict[str, Union[str, int]] = deepcopy(self.pyname2cname)
            gb_cexprs : List[str] = []
            gb_colnames : List[str] = []
            gb_types : List[Types] = []
            for key, val in proj_map.items():
                col_name = 'col_' + base62uuid(6)
                self.context.emitc(f'decltype(auto) {col_name} = {self.out_table.contextname_cpp}->get_col<{key}>();')
                gb_cexprs.append((col_name, val[2]))
                gb_colnames.append(col_name)
                gb_types.append(val[0])
            self.group_node.finalize(gb_cexprs, gb_vartable, gb_colnames, gb_types, gb_tovec)
        else:
            for i, (key, val) in enumerate(proj_map.items()):
                if type(val[1]) is int:
                    self.context.emitc(
                        f'{self.out_table.contextname_cpp}->get_col<{key}>().initfrom({vid2cname[val[1]]}, "{cols[i].name}");'
                    )
                else:
                    # for funcs evaluate f_i(x, ...)
                    self.context.emitc(f'{self.out_table.contextname_cpp}->get_col<{key}>().initfrom({val[1]}, "{cols[i].name}");')
        # print out col_is
        
        if 'into' not in node and self.subq_type == SubqType.NONE:
            if self.limit is None:
                self.context.emitc(f'print(*{self.out_table.contextname_cpp});')
            else:
                self.context.emitc(f'{self.out_table.contextname_cpp}->printall(" ","\\n", nullptr, nullptr, {self.limit});')
        
        if self.outfile and self.has_postproc:
                self.outfile.finalize()

        if not self.distinct:
            self.finalize(node)
        
    def deal_with_into(self, node):
        if 'into' in node: 
            self.context.emitc(select_into(self, node['into']).ccode)            
    def finalize(self, node):
        self.deal_with_into(node)
        self.context.emitc(f'puts("done.");')
        self.context.emitc('printf("done: %lld\\n", (chrono::high_resolution_clock::now() - timer).count());timer = chrono::high_resolution_clock::now();')

        if self.parent is None:
            self.context.sql_end()
            if self.has_postproc:
                self.context.has_dll = True
                self.context.postproc_end(self.postproc_fname)
            else:
                self.context.ccode = ''
                if self.limit != 0 and not self.outfile:
                    self.context.direct_output(self.limit)
        
class select_distinct(projection):
    first_order = 'select_distinct'
    def consume(self, node):
        super().consume(node)
        if self.has_postproc:
            self.context.emitc(
                f'{self.out_table.contextname_cpp}->distinct();'
            )
        self.finalize(node)
        
class select_into(ast_node):
    def init(self, _):
        if isinstance(self.parent, projection):
            # if self.parent.has_postproc:
            #     # has postproc put back to monetdb
            self.produce = self.produce_cpp
            # else:
            #     self.produce = self.produce_sql
        else:
            raise ValueError('parent must be projection')
        
    def produce_cpp(self, node):
        if not hasattr(self.parent, 'out_table'):
            raise Exception('No out_table found.')
        else:
            self.context.headers.add('"./server/table_ext_monetdb.hpp"')
            self.ccode = f'{self.parent.out_table.contextname_cpp}->monetdb_append_table(cxt->curr_server, \"{node.lower()}\");'
            
    def produce_sql(self, node):
        self.context.sql = self.context.sql.replace(
            self.parent.into_stub, f'INTO {node}', 1)
    

class orderby(ast_node):
    name = 'order by'
    def produce(self, node):
        if node is None:
            self.sql = ''
            return
        
        node = enlist(node)
        o_list = []
        
        for o in node:
            o_str = expr(self, o['value']).sql
            if 'sort' in o and f'{o["sort"]}'.lower() == 'desc':
                o_str += ' ' + 'DESC'
            o_list.append(o_str)
        self.add(', '.join(o_list))


class scan(ast_node):
    class Position(Enum):
        init = auto()
        front = auto()
        body = auto()
        back = auto()
        fin = auto()
        # TODO: use this for positions for scanner
    class LoopStyle(Enum):
        forloop = auto()
        foreach = auto()
        
    name = 'scan'
    def __init__(self, parent: "ast_node", node, loop_style = 'for', context: Context = None, const = False, it_name = None):
        self.it_var = it_name
        self.const = "const " if const else ""
        self.loop_style = loop_style
        super().__init__(parent, node, context)
        
    def init(self, _):
        self.it_var = self.context.get_scan_var() if not self.it_var else self.it_var
        self.datasource = self.context.datasource
        self.initializers = ''
        self.start = ''
        self.front = ''
        self.body = ''
        self.end = '}'
        self.parent.context.scans.append(self)
        
    def produce(self, node):
        if self.loop_style == 'for_each':
            self.colref = node
            self.start += f'for ({self.const}auto& {self.it_var} : {node}) {{\n'
        else:
            self.start += f"for (uint32_t {self.it_var} = 0; {self.it_var} < {node}; ++{self.it_var}){{\n"
            
    def add(self, stmt, position = "body"):
        if position == "body":
            self.body += stmt + '\n'
        elif position == "init":
            self.initializers += stmt + '\n'
        else:
            self.front += stmt + '\n'
            
    def finalize(self):
        self.context.remove_scan(self, self.initializers + self.start + self.front + self.body + self.end)
    
class groupby_c(ast_node):
    name = '_groupby'
    def init(self, node : List[Tuple[expr, Set[ColRef]]]):
        self.proj : projection = self.parent
        self.glist : List[Tuple[expr, Set[ColRef]]] = node
        self.vecs : str = 'vecs_' + base62uuid(3)
        return super().init(node)
    
    def produce(self, node : List[Tuple[expr, Set[ColRef]]]):
        self.context.headers.add('"./server/hasher.h"')
        # self.context.headers.add('unordered_map')
        self.group = 'g' + base62uuid(7)
        self.group_type = 'record_type' + base62uuid(7)
        self.datasource = self.proj.datasource
        self.scanner = None
        self.datasource.rec = set()
        
        g_contents = ''
        g_contents_list = []
        first_col = ''
        scanner_itname = self.context.get_scan_var()
        for g in self.glist:
            e = expr(self, g[0].node, c_code=True)
            g_str = e.eval(c_code = True, y = lambda c: self.proj.pyname2cname[c])
            # if v is compound expr, create tmp cols
            if not e.is_ColExpr:
                self.context.headers.add('"./server/aggregations.h"')                    
                tmpcol = 't' + base62uuid(7)
                self.context.emitc(f'auto {tmpcol} = {g_str};')
                e = tmpcol
            else:
                e = g_str
            g_contents_list.append(e)
        first_col = g_contents_list[0]
        self.total_sz = 'len_' + base62uuid(4)
        self.context.emitc(f'uint32_t {self.total_sz} = {first_col}.size;')
        g_contents_decltype = [f'decays<decltype({c})::value_t>' for c in g_contents_list]
        g_contents = ', '.join(
            [f'{c}[{scanner_itname}]' for c in g_contents_list]
        )
        self.context.emitc('printf("init_time: %lld\\n", (chrono::high_resolution_clock::now() - timer).count()); timer = chrono::high_resolution_clock::now();')
        self.context.emitc(f'typedef record<{",".join(g_contents_decltype)}> {self.group_type};')
        self.context.emitc(f'AQHashTable<{self.group_type}, '
            f'transTypes<{self.group_type}, hasher>> {self.group} {{{self.total_sz}}};')
        self.n_grps = len(self.glist)
        self.scanner = scan(self, self.total_sz, it_name=scanner_itname)
        self.scanner.add(f'{self.group}.hashtable_push(forward_as_tuple({g_contents}), {self.scanner.it_var});')

    def consume(self, _):
        self.scanner.finalize()
        self.context.emitc('printf("ht_construct: %lld\\n", (chrono::high_resolution_clock::now() - timer).count()); timer = chrono::high_resolution_clock::now();')
        self.context.emitc(f'auto {self.vecs} = {self.group}.ht_postproc({self.total_sz});')
        self.context.emitc('printf("ht_postproc: %lld\\n", (chrono::high_resolution_clock::now() - timer).count()); timer = chrono::high_resolution_clock::now();')
    # def deal_with_assumptions(self, assumption:assumption, out:TableInfo):
    #     gscanner = scan(self, self.group)
    #     val_var = 'val_'+base62uuid(7)
    #     gscanner.add(f'auto &{val_var} = {gscanner.it_var}.second;')
    #     gscanner.add(f'{self.datasource.cxt_name}->order_by<{assumption.result()}>(&{val_var});')
    #     gscanner.finalize()
        
    def finalize(self, cexprs : List[Tuple[str, expr]], var_table : Dict[str, Union[str, int]], 
                 col_names : List[str], col_types : List[Types], col_tovec : List[bool]):
        tovec_columns = set()
        for i, c in enumerate(col_names):
            if col_tovec[i]: # and type(col_types[i]) is VectorT:
                self.context.emitc(f'{c}.resize({self.group}.size());')
                typename : Types = col_types[i] # .inner_type
                self.context.emitc(f'auto buf_{c} = static_cast<{typename.cname} *>(calloc({self.total_sz}, sizeof({typename.cname})));')
                tovec_columns.add(c)
            else:
                self.context.emitc(f'{c}.resize({self.group}.size());')
                
        self.arr_len = 'arrlen_' + base62uuid(3)
        self.arr_values = 'arrvals_' + base62uuid(3)
        
        self.context.emitc(f'auto {self.arr_len} = {self.group}.size();')
        self.context.emitc(f'auto {self.arr_values} = {self.group}.values();')
        
        if len(tovec_columns):
            preproc_scanner = scan(self, self.arr_len)
            preproc_scanner_it = preproc_scanner.it_var
            for c in tovec_columns:
                preproc_scanner.add(f'{c}[{preproc_scanner_it}].init_from'
                                    f'({self.vecs}[{preproc_scanner_it}].size,'
                                    f' {"buf_" + c} + {self.group}.ht_base'
                                    f'[{preproc_scanner_it}]);'
                )
            preproc_scanner.finalize()
        
        self.context.emitc('GC::scratch_space = GC::gc_handle ? &(GC::gc_handle->scratch) : nullptr;')
        # gscanner = scan(self, self.group, loop_style = 'for_each')
        gscanner = scan(self, self.arr_len)
        key_var = 'key_'+base62uuid(7)
        val_var = 'val_'+base62uuid(7)
        
        # gscanner.add(f'auto &{key_var} = {gscanner.it_var}.first;', position = 'front')
        # gscanner.add(f'auto &{val_var} = {gscanner.it_var}.second;', position = 'front')
        gscanner.add(f'auto &{key_var} = {self.arr_values}[{gscanner.it_var}];', position = 'front')
        gscanner.add(f'auto &{val_var} = {self.vecs}[{gscanner.it_var}];', position = 'front')
        len_var = None
        def define_len_var():
            nonlocal len_var
            if len_var is None:
                len_var = 'len_'+base62uuid(7)
                gscanner.add(f'auto &{len_var} = {val_var}.size;', position = 'front')
        
        def get_key_idx (varname : str):
            ex = expr(self, varname)
            joint_cols = set()
            if ex.is_ColExpr and ex.raw_col:
                joint_cols = self.datasource.get_joint_cols([ex.raw_col])
            for i, g in enumerate(self.glist):
                if (varname == g[0].eval()) or (g[0].is_ColExpr and g[0].raw_col and 
                      g[0].raw_col in joint_cols):
                    return i
            return var_table[varname]
        
        def get_var_names (varname : str):
            var = get_key_idx(varname)
            if type(var) is str:
                return f'{var}[{val_var}]'
            else:
                return f'get<{var}>({key_var})'
        
        def get_var_names_ex (varex : expr):
            sql_code = varex.eval(c_code = False)
            if (sql_code in var_table):
                return get_var_names(sql_code)
            else:
                return varex.eval(c_code=True, y = get_var_names, 
                        materialize_builtin = materialize_builtin, 
                        count=lambda:f'{val_var}.size')
                
        for i, ce in enumerate(cexprs):
            ex = ce[1]
            materialize_builtin = {}
            if type(ex.udf_called) is udf:
                if '_builtin_len' in ex.udf_called.builtin_used:
                    define_len_var()
                    materialize_builtin['_builtin_len'] = len_var
                if '_builtin_ret' in ex.udf_called.builtin_used:
                    define_len_var()
                    gscanner.add(f'{ce[0]}[{gscanner.it_var}] = {len_var};\n')
                    materialize_builtin['_builtin_ret'] = f'{ce[0]}.back()'
                    gscanner.add(f'{ex.eval(c_code = True, y=get_var_names, materialize_builtin = materialize_builtin)};\n')
                    continue
            if col_tovec[i]:
                if ex.remake_binary(f'{ce[0]}[{gscanner.it_var}]'):
                    gscanner.add(f'{get_var_names_ex(ex)};\n')
                else:
                    gscanner.add(f'{ce[0]}[{gscanner.it_var}] = {get_var_names_ex(ex)};\n')
            else:
                gscanner.add(f'{ce[0]}[{gscanner.it_var}] = ({get_var_names_ex(ex)});\n')
        
        gscanner.add(f'GC::scratch_space->release();')
        self.context.emitc('printf("ht_initfrom: %lld\\n", (chrono::high_resolution_clock::now() - timer).count());timer = chrono::high_resolution_clock::now();')
        
        gscanner.finalize()
        self.context.emitc(f'GC::scratch_space = nullptr;')
        self.context.emitc('printf("agg: %lld\\n", (chrono::high_resolution_clock::now() - timer).count());timer = chrono::high_resolution_clock::now();')
        
        self.datasource.groupinfo = None

            
class groupby(ast_node):
    name = 'group by'
    
    @staticmethod
    def check_special(parent : projection, node):
        node = enlist(node)
        for g in node:
            if ('value' in g and 
                expr(parent, g['value']).is_special
            ):
                return True
        return False

    def init(self, _):
        self.terminate = False
        super().init(_)
        
    def produce(self, node):
        if not isinstance(self.parent, projection):
            raise ValueError('groupby can only be used in projection')
        
        node = enlist(node)
        o_list = []
        self.refs = set()
        self.gb_cols = set()
        # dedicated_glist -> cols populated for special group by
        self.dedicated_glist : List[Tuple[expr, Set[ColRef]]] = []
        self.use_sp_gb = self.parent.force_use_spgb
        for g in node:
            self.datasource.rec = set()
            g_expr = expr(self, g['value'])
            self.use_sp_gb = self.use_sp_gb or g_expr.is_special
            refs : Set[ColRef] = self.datasource.rec
            self.datasource.rec = None
            if self.parent.col_ext:
                this_sp_ref = refs.difference(self.parent.col_ext)
                self.use_sp_gb = self.use_sp_gb or len(this_sp_ref) > 0
            self.refs.update(refs)
            self.dedicated_glist.append((g_expr, refs))
            g_str = g_expr.eval(c_code = False)
            if 'sort' in g and f'{g["sort"]}'.lower() == 'desc':
                g_str = g_str + ' ' + 'DESC'
            o_list.append(g_str)
            if g_expr.is_ColExpr:
                self.gb_cols.add(g_expr.raw_col)
            else:
                self.gb_cols.add(g_expr.sql)
                
        for projs in self.parent.proj_map.values():
            if self.use_sp_gb:
                break
            if (projs[2].is_compound and 
                not ((projs[2].is_ColExpr and projs[2].raw_col in self.gb_cols) or
                projs[2].sql in self.gb_cols)
                ) and (not self.parent.force_use_spgb):
                    self.use_sp_gb = True
                    break
                
        if self.use_sp_gb and not self.parent.force_use_spgb:
            self.terminate = True
            return
        if not self.use_sp_gb:
            self.dedicated_gb = None
            self.add(', '.join(o_list))
        else:
            for l in self.dedicated_glist:
                # l_exist = l[1].difference(self.parent.col_ext)
                # for l in l_exist:
                #     self.parent.var_table.
                self.parent.col_ext.update(l[1])    
                
    def finalize(self, cexprs : List[Tuple[str, expr]], var_table : Dict[str, Union[str, int]], 
                 col_names : List[str], col_types : List[Types], col_tovec : List[bool]):
        if self.use_sp_gb:
            self.dedicated_gb = groupby_c(self.parent, self.dedicated_glist)
            self.dedicated_gb.finalize(cexprs, var_table, col_names, col_types, col_tovec)


class join(ast_node):
    name = 'join'
    
    def get_joint_cols(self, cols : List[ColRef]):
        joint_cols = set(cols)
        for col in cols:
            joint_cols |= self.joint_cols.get(col, set())
        return joint_cols

    def strip_joint_cols(self, cols : Set[ColRef]):
        stripped = type(cols)(cols)
        for c in cols:
            if c in stripped:
                jc = self.get_joint_cols([c])
                for j in jc:
                    if j != c and j in stripped:
                        stripped.remove(j)
        return stripped
    
    def init(self, _):
        self.joins : List[join] = []
        self.tables : List[TableInfo] = []
        self.tables_dir = dict()
        self.rec = None
        self.top_level = self.parent and isinstance(self.parent, projection)
        self.have_sep = False
        self.joint_cols : Dict[ColRef, Set]= dict() # columns that are joined with this column
        self.children : Set(join) = {}
        self.join_conditions = []
        # self.tmp_name = 'join_' + base62uuid(4)
        # self.datasource = TableInfo(self.tmp_name, [], self.context)
        
    def append(self, tbls, __alias = ''):
        alias = lambda t : t + ' ' + __alias if len(__alias) else t
        if type(tbls) is join:
            self.joins.append((alias(tbls.__str__()), tbls.have_sep))
            self.tables += tbls.tables
            self.tables_dir = {**self.tables_dir, **tbls.tables_dir}
            self.join_conditions += tbls.join_conditions
            
        elif type(tbls) is TableInfo:
            self.joins.append((alias(tbls.table_name), False))
            self.tables.append(tbls)
            self.tables_dir[tbls.table_name] = tbls
            for a in tbls.alias:
                self.tables_dir[a] = tbls
            
        elif type(tbls) is projection:
            self.joins.append((alias(tbls.finalize()), False))
            
    def produce(self, node):
        if type(node) is list:
            for d in node:
                self.append(join(self, d))

        elif type(node) is dict:
            alias = ''
            if 'value' in node:
                table_name = node['value']
                tbl = None
                if 'name' in node:
                    alias = node['name']
                if type(table_name) is dict:
                    if 'select' in table_name:
                        # TODO: subquery, create and register TableInfo in projection
                        tbl = projection(self, table_name).finalize()
                else:
                    tbl = self.context.tables_byname[table_name]
                    if 'name' in node:
                        tbl.add_alias(node['name'])
                self.append(tbl, alias)
            else:
                keys : List[str] = list(node.keys())
                if keys[0].lower().endswith('join'):
                    self.have_sep = True
                    j = join(self, node[keys[0]])
                    self.tables += j.tables
                    self.tables_dir = {**self.tables_dir, **j.tables_dir}
                    self.join_conditions += j.join_conditions
                    _tbl_union = _tmp_join_union(self.context, self.parent, self)
                    tablename = f' {keys[0]} {j}'
                    if len(keys) > 1 :
                        jcond = node[keys[1]]
                        sqoute = '\''
                        if type(jcond) is list:
                            _ex = [expr(_tbl_union, j) for j in jcond]
                        else:
                            _ex = expr(_tbl_union, jcond)
                        if keys[1].lower() == 'on': # postpone join condition evaluation after consume
                            self.join_conditions += _ex.join_conditions
                            tablename += f" ON {_ex.eval().replace(sqoute, '')}" 
                        elif keys[1].lower() == 'using':
                            _ex = enlist(_ex)
                            lst_jconds = []
                            for _e in _ex:
                                if _e.is_ColExpr:
                                    cl = _e.raw_col
                                    cr = j.get_cols(_e.raw_col.name)
                                    self.join_conditions.append( (cl, cr) )
                                    lst_jconds += [f'{cl.get_full_name()} = {cr.get_full_name()}']
                            tablename += f' ON {" and ".join(lst_jconds)}'
                            
                    if keys[0].lower().startswith('natural'):
                        ltbls : List[TableInfo] = []
                        if isinstance(self.parent, join):
                            ltbls = self.parent.tables
                        elif isinstance(self.parent, TableInfo):
                            ltbls = [self.parent]
                        for tl in ltbls:
                            for cl in tl.columns:
                                cr = j.get_cols(cl.name)
                                if cr:
                                    self.join_conditions.append( (cl, cr) )
                    self.joins.append((tablename, self.have_sep))

                
        elif type(node) is str:
            if node in self.context.tables_byname:
                self.append(self.context.tables_byname[node])
            else:
                print(f'Error: table {node} not found.')
    
    def get_cols(self, colExpr: str) -> Optional[ColRef]:
        if '*' in colExpr:
            if colExpr == '*':
                return self.all_cols(ordered = True, stripped = True)
            elif colExpr.endswith('.*'):
                tbl = colExpr.split('.')
                if len(tbl) > 2:
                    raise KeyError(f'Invalid expression: {colExpr}')
                if tbl[0] in self.tables_dir:
                    tbl : TableInfo= self.tables_dir[tbl[0]]
                    return tbl.all_cols(ordered = True)
                else:
                    raise KeyError(f'Invalid table name: {colExpr}') 
        for t in self.tables:
            if colExpr in t.columns_byname:
                col = t.columns_byname[colExpr]
                if type(self.rec) is set:
                    self.rec.add(col)
                return col
        return None
            
    def parse_col_names(self, colExpr:str) -> ColRef:
        parsedColExpr = colExpr.split('.')
        if len(parsedColExpr) <= 1:
            return self.get_cols(colExpr)
        else:
            datasource = self.tables_dir[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                datasource.rec = self.rec
                ret = datasource.parse_col_names(parsedColExpr[1])
                datasource.rec = None
                return ret

    @property
    def single_table(self):
        return len(self.tables) == 1
    
#    @property
    def all_cols(self, ordered = False, stripped = True):
        from ordered_set import OrderedSet
        ret = OrderedSet() if ordered else set()
        for table in self.tables:
            rec = table.rec
            table.rec = self.rec
            ret.update(table.all_cols(ordered = ordered))
            table.rec = rec
        if stripped:
            return self.strip_joint_cols(ret)
        return ret
    
    # TODO: join condition awareness
    def process_join_conditions(self):
        # This is done after both from 
        # and where clause are processed
        for j in self.join_conditions:
            l = j[0]
            r = j[1]
            for k in (0, 1):
                if j[k] not in self.joint_cols:
                    self.joint_cols[j[k]] = set([l, r])
            jr = self.joint_cols[r]
            jl = self.joint_cols[l]
            if jl != jr:
                jl |= jr
                for c, jc in self.joint_cols.items():
                    if jc == jr:
                        self.joint_cols[c] = jl
            
        # print(self.join_conditions)
        # print(self.joint_cols)
    
    def consume(self, node):
        self.sql = ''
        for j in self.joins:
            if not self.sql or j[1]:
                self.sql += j[0] # using JOIN keyword
            else:
                self.sql += ', ' + j[0] # using comma
                    
        if node and self.sql and self.top_level:
            self.sql = ' FROM ' + self.sql 
        return super().consume(node)
        
    def __str__(self):
        return self.sql
    def __repr__(self):
        return self.__str__()
    
class _tmp_join_union(join):
    name = '__tmp_join_union'
    
    def __init__(self, context, l: join, r: join):
        self.tables = l.tables + r.tables
        self.tables_dir = {**l.tables_dir, **r.tables_dir}
        self.datasource = self
        self.context = context
        self.parent = self
        self.rec = None
        
class filter(ast_node):
    name = 'where'
    def produce(self, node):
        filter_expr = expr(self, node)
        self.add(filter_expr.sql)
        if self.datasource is not None:
            self.datasource.join_conditions += filter_expr.join_conditions

class union_all(ast_node):
    name = 'union_all'
    first_order = name
    sql_name = 'UNION ALL'
    def produce(self, node):
        queries = node[self.name]
        self.generated_queries : List[Optional[projection]] = [None] * len(queries)
        is_standard = True
        for i, q in enumerate(queries):
            if 'select' in q:
                self.generated_queries[i] = projection(self, q)
                is_standard &= not self.generated_queries[i].has_postproc
        if is_standard:
            self.sql = f' {self.sql_name} '.join([q.sql for q in self.generated_queries])
        else:
            raise NotImplementedError(f"{self.sql_name} only support standard sql for now")
    def consume(self, node):
        if 'into' in node:
            outtable = TableInfo(node['into'], [], self.context)
            lst_cols = [None] * len(self.generated_queries[0].out_table.columns)
            for i, c in enumerate(self.generated_queries[0].out_table.columns):
                lst_cols[i] = ColRef(c.type, None, outtable, c.name, i, c.compound)
            outtable.add_cols(lst_cols, new = False)
            
            col_names = [c.name for c in outtable.columns]
            col_names = '(' + ', '.join(col_names) + ')'
            self.sql = f'CREATE TABLE {node["into"]} {col_names} AS {self.sql}'
        super().consume(node)
        if 'into' not in node:
            self.context.direct_output()
        

class except_clause(union_all):
    name = 'except'
    first_order = name
    sql_name = 'EXCEPT'

class create_table(ast_node):
    name = 'create_table'
    first_order = name
    allowed_subq = {
                        'select_distinct': select_distinct, 
                        'select': projection, 
                        'union_all': union_all, 
                        'except': except_clause
                    }
    def init(self, node):
        node = node[self.name]
        if 'query' in node:
            if 'name' not in node:
                raise ValueError("Table name not specified")
            projection_node = node['query']
            projection_node['into'] = node['name']
            proj_cls = projection
            for k in create_table.allowed_subq.keys():
                if k in projection_node:
                    proj_cls = create_table.allowed_subq[k]
                    break
            proj_cls(None, projection_node, self.context)
            self.produce = lambda *_: None
            self.spawn = lambda *_: None
            self.consume = lambda *_: None
            return
        if self.parent is None:
            self.context.sql_begin()
        self.sql = 'CREATE TABLE '
    
    def produce(self, node):
        ct = node[self.name]
        tbl = self.context.add_table(ct['name'], ct['columns'])
        self.sql = f'CREATE TABLE IF NOT EXISTS {tbl.table_name}('
        columns = []
        for c in tbl.columns:
            columns.append(f'{c.name} {c.type.sqlname}')
        self.sql += ', '.join(columns)
        self.sql += ')'
        if self.context.use_columnstore:
            self.sql += ' engine=ColumnStore'

class create_trigger(ast_node):
    name = 'create_trigger'
    first_order = name
    class Type (Enum):
        Interval = auto()
        Callback = auto()
    def init(self, _):
        # overload init to prevent automatic sql generation
        pass
    def consume(self, _):
        # overload consume to prevent automatic sqlend action
        pass
    
    def produce(self, node):
        from common.utils import send_to_server, get_storedproc
        node = node['create_trigger']
        self.trigger_name = node['name']
        self.action_name = node['action']
        self.action = get_storedproc(self.action_name)
        if self.trigger_name in self.context.triggers:
            raise ValueError(f'trigger {self.trigger_name} exists')
        elif not self.action:
            raise ValueError(f'Stored Procedure {self.action_name} do not exist')

        if 'interval' in node: # executed periodically from server
            self.type = self.Type.Interval
            self.interval = node['interval']
            self.context.queries.append(f'TI{self.trigger_name}\0{self.action_name}\0{self.interval}')
        else: # executed from sql backend
            self.type = self.Type.Callback
            self.query_name = node['query']
            self.table_name = node['table']
            self.procedure = get_storedproc(self.query_name)
            if self.procedure and self.table_name in self.context.tables_byname:
                self.table = self.context.tables_byname[self.table_name]
                self.table.triggers.add(self)
                self.context.queries.append(
                    f'TC{self.trigger_name}\0{self.table_name}\0'
                    f'{self.query_name}\0{self.action_name}'
                )                
            else:
                return
        self.context.triggers[self.trigger_name] = self

    # manually execute trigger
    def register(self): 
        if self.type != self.Type.Callback:
            self.context.triggers.pop(self.trigger_name)
            raise ValueError(f'Trigger {self.trigger_name} is not a callback based trigger')
        self.context.triggers_active.add(self)

    def execute(self): 
        from common.utils import send_to_server
        self.context.queries.append(f'TA{self.query_name}\0{self.action_name}')

    def remove(self):
        from common.utils import send_to_server
        self.context.queries.append(f'TR{self.trigger_name}')


class drop_trigger(ast_node):
    name = 'drop_trigger'
    first_order = name
    def produce(self, node):
        ...

class drop(ast_node):
    name = 'drop'
    first_order = name
    def produce(self, node):
        node = node['drop']
        tbl_name = node['table']
        if tbl_name in self.context.tables_byname:
            tbl_obj : TableInfo = self.context.tables_byname[tbl_name]
            for a in tbl_obj.alias:
                self.context.tables_byname.pop(a, None) 
            # TODO: delete in postproc engine
            self.context.tables_byname.pop(tbl_name, None)
            self.context.tables.discard(tbl_obj)
            self.sql += 'TABLE IF EXISTS ' + tbl_name
            return
        
        elif 'if_exists' not in node or not node['if_exists']:
            print(f'Error: table {tbl_name} not found.')
        self.sql = ''
        
class insert(ast_node):
    name = 'insert'
    first_order = name
    def init(self, node):
        if 'query' in node:
            values = node['query']
            complex_query_kw = ['from', 'where', 'groupby', 'having', 'orderby', 'limit']
            if any([kw in values for kw in complex_query_kw]):
                values['into'] = node['insert']
                proj_cls = (
                    select_distinct 
                    if 'select_distinct' in values 
                    else projection
                )
                proj_cls(None, values, self.context)
                self.produce = lambda*_:None
                self.spawn = lambda*_:None
                self.consume = lambda*_:None
        else:
            super().init(node)
            
    def produce(self, node):
        keys = []
        if 'query' in node:
            if 'select' in node['query']:
                values = enlist(node['query']['select'])
                if 'columns' in node:
                    keys = node['columns']
                values = [v['value'] for v in values]

            elif 'union_all' in node['query']:
                values = [[v['select']['value']] for v in node['query']['union_all']]
                if 'columns' in node:
                    keys = node['columns']
        else:
            values = enlist(node['values'])
            _vals = []
            for v in values:
                if isinstance(v, dict):
                    keys = v.keys()
                    v = list(v.values())
                v = [f"'{vv}'" if type(vv) is str else vv for vv in v]
                _vals.append(v)
            values = _vals
            
        keys = f'({", ".join(keys)})' if keys else ''
        tbl = node['insert']
        if tbl not in self.context.tables_byname:
            print('Warning: {tbl} not registered in aquery compiler.')
        tbl_obj = self.context.tables_byname[tbl]
        for t in tbl_obj.triggers:
            t.register()
        self.sql = f'INSERT INTO {tbl}{keys} VALUES'
        # if len(values) != table.n_cols:
        #     raise ValueError("Column Mismatch")
        values = [values] if isinstance(values, list) and not isinstance(values[0], list) else values
        list_values = []
        for l in values:
            inner_list_values = []
            for s in enlist(l):
                if type(s) is dict and 'value' in s:
                    s = s['value']
                inner_list_values.append(f"{get_innermost(s)}")
            list_values.append(f"({', '.join(inner_list_values)})")
            
        self.sql += ', '.join(list_values) 


class delete_from(ast_node):
    name = 'delete'
    first_order = name
    def init(self, node):
        super().init(node)
            
    def produce(self, node):
        tbl = node['delete']
        self.sql = f'DELETE FROM {tbl} '
        if 'where' in node:
            self.sql += filter(self, node['where']).sql
    
class load(ast_node):
    name="load"
    first_order = name
    def init(self, node):
        from common.utils import Backend_Type
        self.module = False
        if node['load']['file_type'] == 'module':
            self.produce = self.produce_module
            self.module = True
        elif 'complex' in node['load']:
            self.produce = self.produce_cpp
            self.consume = lambda *_: None
        elif self.context.system_state.cfg.backend_type == Backend_Type.BACKEND_MonetDB.value:
            self.produce = self.produce_monetdb
        elif self.context.system_state.cfg.backend_type == Backend_Type.BACKEND_DuckDB.value:
            self.produce = self.produce_duckdb
        else: 
            self.produce = self.produce_aq
        if self.parent is None:
            self.context.sql_begin()
    
    def produce_module(self, node):
        # create command for exec engine -> done
        # create c++ stub 
        # create dummy udf obj for parsing
        # def decode_type(ty : str) -> str:
        #     ret = ''
        #     back = ''
        #     while(ty.startswith('vec')):
        #         ret += 'ColRef<'
        #         back += '>'
        #         ty = ty[3:]
        #     ret += ty
        #     return ret + back
        node = node['load']
        file = node['file']['literal']
        self.context.queries.append(f'M{file}')
        self.module_name = file
        self.functions = {}
        if 'funcs' in node:
            for f in enlist(node['funcs']):
                fname = f['fname']
                self.context.queries.append(f'F{fname}')
                ret_type = VoidT
                if 'ret_type' in f:
                    ret_type = Types.decode(f['ret_type'], vector_type='vector_type')
                nargs = 0
                arglist = ''
                if 'vars' in f:
                    arglist = []
                    for v in enlist(f['vars']):
                        arglist.append(f'{Types.decode(v["type"]).cname} {v["arg"]}')
                    nargs = len(arglist)
                    arglist = ', '.join(arglist)
                # create c++ stub 
                cpp_stub = f'{"vectortype_cstorage" if isinstance(ret_type, VectorT) else ret_type.cname} (*{fname})({arglist}) = nullptr;'
                self.context.module_stubs += cpp_stub + '\n'
                self.context.module_map[fname] = cpp_stub
                #registration for parser
                self.functions[fname] = user_module_function(fname, nargs, ret_type, self.context)
        self.context.module_init_loc = len(self.context.queries)
        
    def produce_aq(self, node):
        node = node['load']
        s1 = 'LOAD DATA INFILE '
        s2 = 'INTO TABLE '
        s3 = 'FIELDS TERMINATED BY '
        self.sql = f'{s1} \"{node["file"]["literal"]}\" {s2} {node["table"]}'
        if 'term' in node:
            self.sql += f' {s3} \"{node["term"]["literal"]}\"'
            
    def produce_monetdb(self, node):
        node = node['load']
        s1 = f'COPY OFFSET 2 INTO {node["table"]} FROM '
        s2 = ' ON SERVER '
        s3 = ' USING DELIMITERS '
        import os
        p = os.path.abspath(node['file']['literal']).replace('\\', '/')
        
        self.sql = f'{s1} \'{p}\' {s2} '
        if 'term' in node:
            self.sql += f' {s3} \'{node["term"]["literal"]}\''
    
    def produce_duckdb(self, node):
        node = node['load']
        s1 = f'COPY {node["table"]} FROM '
        import os
        p = os.path.abspath(node['file']['literal']).replace('\\', '/')
        s2 = f" DELIMITER '{node['term']['literal']}', " if 'term' in node else ''
        self.sql = f'{s1} \'{p}\' ( {s2}HEADER )'
        
    
    def produce_cpp(self, node):
        self.context.has_dll = True
        self.context.headers.add('"csv.h"')
        node = node['load']
        self.postproc_fname = 'ld_' + base62uuid(5)
        self.context.postproc_begin(self.postproc_fname)
        
        table:TableInfo = self.context.tables_byname[node['table']]
        # self.sql = F"SELECT {', '.join([c.name for c in table.columns])} FROM {table.table_name};"
        # self.emit(self.sql+';\n')
        # self.context.sql_end()
        # length_name = 'len_' + base62uuid(6)
        # self.context.emitc(f'auto {length_name} = server->cnt;')
        
        out_typenames = [t.type.cname for t in table.columns]
        outtable_col_nameslist = ', '.join([f'"{c.name}"' for c in table.columns])
        
        self.outtable_col_names = 'names_' + base62uuid(4)
        self.context.emitc(f'const char* {self.outtable_col_names}[] = {{{outtable_col_nameslist}}};')
        
        self.out_table = 'tbl_' + base62uuid(4)
        self.context.emitc(f'auto {self.out_table} = new TableInfo<{",".join(out_typenames)}>("{table.table_name}", {self.outtable_col_names});')
        for i, c in enumerate(table.columns):
            c.cxt_name = 'c_' + base62uuid(6) 
            self.context.emitc(f'decltype(auto) {c.cxt_name} = {self.out_table}->get_col<{i}>();')
            self.context.emitc(f'{c.cxt_name}.init("{table.columns[i].name}");')
            #self.context.emitc(f'{c.cxt_name}.initfrom({length_name}, server->getCol({i}), "{table.columns[i].name}");')
        csv_reader_name = 'csv_reader_' + base62uuid(6)
        col_types = [c.type.cname for c in table.columns]
        col_tmp_names = ['tmp_'+base62uuid(8) for _ in range(len(table.columns))]
        #col_names = ','.join([f'"{c.name}"' for c in table.columns])
        term_field = ',' if 'term' not in node else node['term']['literal']
        term_ele = ';' if 'ele' not in node else node['ele']['literal']
        self.context.emitc(f'AQCSVReader<{len(col_types)}, \'{term_field.strip()[0]}\', \'{term_ele.strip()[0]}\'> {csv_reader_name}("{node["file"]["literal"]}");')
        # self.context.emitc(f'{csv_reader_name}.read_header(io::ignore_extra_column, {col_names});')
        self.context.emitc(f'{csv_reader_name}.next_line();')

        for t, n in zip(col_types, col_tmp_names):
            self.context.emitc(f'{t} {n};')
        self.context.emitc(f'while({csv_reader_name}.read_row({",".join(col_tmp_names)})) {{ \n')
        for i, c in enumerate(table.columns):
            # self.context.emitc(f'print({col_tmp_names[i]});')
            self.context.emitc(f'{c.cxt_name}.emplace_back({col_tmp_names[i]});')
            
        self.context.emitc('}')
        # self.context.emitc(f'print(*{self.out_table});')
        self.context.emitc(f'{self.out_table}->monetdb_append_table(cxt->curr_server, "{table.table_name}");')
        
        self.context.postproc_end(self.postproc_fname)

class outfile(ast_node):
    name="_outfile"
    def __init__(self, parent, node, context = None, *, sql = None):
        self.node = node
        self.sql = sql if sql else ''
        super().__init__(parent, node, context)
        
    def init(self, _):
        assert(isinstance(self.parent, projection))
        if not self.parent.has_postproc:
            if self.context.dialect == 'MonetDB':
                self.produce = self.produce_monetdb
            else:
                self.produce = self.produce_aq

        return super().init(_)
    def produce_aq(self, node):
        filename = node['loc']['literal'] if 'loc' in node else node['literal']
        self.sql += f'INTO OUTFILE "{filename}"'
        if 'term' in node:
            self.sql += f' FIELDS TERMINATED BY \"{node["term"]["literal"]}\"'

    def produce_monetdb(self, node):
        filename = node['loc']['literal'] if 'loc' in node else node['literal']
        import os
        p =  os.path.abspath('.').replace('\\', '/') + '/' + filename
        print('Warning: file {p} exists and will be overwritten')
        if os.path.exists(p):
            try:
                os.remove(p)
            except OSError:
                print(f'Error: file {p} exists and cannot be removed')
                
        self.sql = f'COPY {self.parent.sql} INTO \'{p}\''
        d = ','
        e = '\\n'
        if 'term' in node:
            d = node['term']['literal']
        self.sql += f' delimiters \'{d}\', \'{e}\''

    def finalize(self):
        filename = self.node['loc']['literal'] if 'loc' in self.node else self.node['literal']
        sep = ',' if 'term' not in self.node else self.node['term']['literal']
        file_pointer = 'fp_' + base62uuid(6)
        self.addc(f'FILE* {file_pointer} = fopen("{filename}", "wb");')
        self.addc(f'{self.parent.out_table.contextname_cpp}->printall("{sep}", "\\n", nullptr, {file_pointer});')
        # if self.context.use_gc:
        #     self.addc(f'GC::gc_handle->reg({file_pointer}, 65536, fclose_gc);')
        # else:
        self.addc(f'fclose({file_pointer});')  
            
        self.context.ccode += self.ccode

class udf(ast_node):
    name = 'udf'
    first_order = name
    @staticmethod
    def try_init_udf(context : Context):
        if context.udf is None:
            context.udf = '/*UDF Start*/\n'
            context.headers.add('\"./udf.hpp\"')
            
    @dataclass
    class builtin_var:
        enabled : bool = False
        _type : Types = AnyT  
        all = ('_builtin_len', '_builtin_ret')
        
    def decltypecall(self, c_code = False, *args):
        from common.types import fn_behavior
        class dummy:
            def __init__(self, name):
                self.cname = name + '_gettype'
                self.sqlname = self.cname
        return fn_behavior(dummy(self.cname), c_code, *args)
    
    def __call__(self, c_code = False, *args):
        from common.types import fn_behavior
        builtin_args = [f'{{{n}()}}' for n, v in self.builtin.items() if v.enabled]
        return fn_behavior(self, c_code, *args, *builtin_args)
    
    def return_type(self, *_ : Types):
        return LazyT
    
    def init(self, _):
        self.builtin : Dict[str, udf.builtin_var] = {
            '_builtin_len' : udf.builtin_var(False, UIntT), 
            '_builtin_ret' : udf.builtin_var(False, Types(
                255, name = 'generic_ref', cname = 'auto&'
            ))
        }
        self.ccode = ''
        self.var_table = {}
        self.args = []
        udf.try_init_udf(self.context)
        self.vecs = set()
        self.code_list = []
        self.builtin_used = None
        
    def add(self, *code):
        ccode = ''
        for c in code:
            if type(c) is str:
                ccode += c
            else:
                self.code_list.append(ccode)
                self.code_list.append(c)
                ccode = ''
        if ccode:
            self.code_list.append(ccode)        
                
        
    def produce(self, node):
        from common.utils import check_legal_name, get_legal_name
        node = node[self.name]
        # register udf
        self.agg = 'Agg' in node
        self.cname = get_legal_name(node['fname'])
        self.sqlname = self.cname
        self.context.udf_map[self.cname] = self
        if self.agg:
            self.context.udf_agg_map[self.cname] = self
        self.add(f'auto {self.cname} = [](')
    
    def get_block(self, ind, node):
        if 'stmt' in node:
            old_ind = ind
            ind += '\t'
            next_stmt = enlist(node['stmt'])
            if len(next_stmt) > 1:
                self.add(f' {{\n')
                self.get_stmt(ind ,next_stmt)
                self.add(f'{old_ind}}}\n')
            else:
                self.get_stmt(ind, next_stmt)
                
    def get_cname(self, x:str):
        return self.var_table[x]
    
    def get_assignment(self, ind, node, *, types = 'auto', sep = ';\n'):
        var_ex = expr(self, node['var'], c_code=True, supress_undefined = True)
        ex = expr(self, node['expr'], c_code=True)
        var = var_ex.eval(y=self.get_cname)
        if var in self.var_table or hasattr(var_ex, 'builtin_var'):
            op = '='
            if 'op' in node and node['op'] != ':=':
                op = node['op']
            e = ex.eval(y=self.get_cname)
            def assign_behavior(decltypestr = False):
                nonlocal ind, var, op, e, sep
                v = var(decltypestr) if callable(var) else var
                _e = e(decltypestr) if callable(e) else e
                if v == '_builtin_ret':
                    return f'{ind}return {_e}{sep}'
                elif '_builtin_ret' not in _e:
                    return f'{ind}{v} {op} {_e}{sep}'
                else:
                    return ''
            self.add(assign_behavior)
        else:
            cvar = get_legal_name(var)
            self.var_table[var] = cvar
            self.add(f'{ind}{types} {cvar} = ', ex.eval(y=self.get_cname), sep)
            
    def get_stmt(self, ind, node):
        node = enlist(node)
        for n in node:
            if 'if' in n:
                _ifnode = n['if']
                self.add(f'{ind}if(', expr(self, _ifnode["cond"]).eval(y=self.get_cname), ')')
                if 'stmt' in _ifnode:
                    self.get_block(ind, _ifnode)
                else:
                    self.add('\n')
                    self.get_stmt(ind + '\t', _ifnode)
                if  'elif' in _ifnode:
                    for e in n['elif']:
                        self.add(f'{ind}else if(', expr(self, e["cond"]).eval(y=self.get_cname), ')')
                        self.get_block(ind, e)
                if 'else' in _ifnode:
                    self.add(f'{ind}else ')
                    self.get_block(ind, _ifnode['else'])
                        
            elif 'for' in n:
                _fornode = n['for']
                defs = _fornode['defs']
                self.add(f'{ind}for({"auto " if len(enlist(defs["op"])) != 0 else ";"}')
                def get_inline_assignments(node, end = '; '):
                    var = enlist(node['var'])
                    op = enlist(node['op'])
                    expr = enlist(node['expr'])
                    len_node = len(enlist(op))
                    for i, (v, o, e) in enumerate(zip(var, op, expr)):
                        self.get_assignment('', {'var' : v, 'op' : o, 'expr' : e}, types = '', sep = ', ' if i != len_node - 1 else end)
                get_inline_assignments(defs)
                self.add(expr(self, _fornode["cond"]).eval(y=self.get_cname), '; ')
                get_inline_assignments(_fornode['tail'], ') ')                
                if 'stmt' in _fornode:
                    self.get_block(ind, _fornode)
                else:
                    self.add('\n')
                    self.get_stmt(ind + '\t', _fornode)
            elif 'assignment' in n:
                assign = n['assignment']
                self.get_assignment(ind, assign)
                    
                    
    def consume(self, node):
        from common.utils import check_legal_name, get_legal_name
        node = node[self.name]
                    
        if 'params' in node:
            for args in node['params']:
                cname = get_legal_name(args)
                self.var_table[args] = cname
                self.args.append(cname)
        front = [*self.code_list, ', '.join([f'const auto& {a}' for a in self.args])]
        self.code_list = []
        
        self.with_storage = False
        self.with_statics = False
        self.static_decl : Optional[List[str]] = None
        ind = '\t'
        if 'static_decl' in node:
            self.add(') {\n')
            curr = node['static_decl']
            self.with_statics = True
            if 'var' in curr and 'expr' in curr:
                if len(curr['var']) != len(curr['expr']):
                    print("Error: every static variable must be initialized.")
                self.static_decl = []
                for v, e in zip(curr['var'], curr['expr']):
                    cname = get_legal_name(v)
                    self.var_table[v] = cname
                    self.static_decl.append(f'{cname} = ', expr(self, e, c_code=True).eval(self.get_cname))
                self.add(f'{ind}static auto {"; static auto ".join(self.static_decl)};\n')
                self.add(f'{ind}auto reset = [=]() {{ {"; ".join(self.static_decl)}; }};\n')
                self.add(f'{ind}auto call = []({", ".join([f"decltype({a}) {a}" for a in self.args])}')
                ind = '\t\t'
                front = [*front, *self.code_list]
                self.code_list = []
        if 'stmt' in node:
            self.get_stmt(ind, node['stmt'])
            # first scan to determine vec types
            # if self.agg:    
            #     for assign in node['assignment']:
            #         var = fastscan(assign['var'])
            #         ex = fastscan(assign['expr'])
            #         self.vecs.union(var.vec_vars)
            #         self.vecs.union(var.requested_lens)
            #         self.vecs.union(ex.vec_vars)
            #         self.vecs.union(ex.requested_lens)
            # if len(self.vecs) != 0:
            #     self.idx_var = 'idx_' + base62uuid(5)
            #     self.ccode += f'{ind}auto {self.idx_var} = 0;\n'
             
        ret = node['ret']
        def return_call(decltypestr = False):
            if (decltypestr):
                return ''
            ret = ''
            for r in self.return_call:
                if callable(r):
                    ret += r(False)
                else:
                    ret += r
            return ret
        self.return_call = (f'{ind}return ', expr(self, ret, c_code=True).eval(self.get_cname), ';\n')
        self.add(return_call)
        if self.with_statics:
            self.add('\t};\n')
            self.add('\treturn std::make_pair(reset, call);\n')
        self.add('};\n')
        
        #print(self.ccode)
        self.builtin_args = [(name, var._type.cname) for name, var in self.builtin.items() if var.enabled]
        # self.context.udf += front + builtin_argstr + self.ccode + '\n'
        self.finalize(front)
        
    def finalize(self, front):
        builtin_argstr = ', ' if len(self.builtin_args) and len(self.args) else ''
        builtin_argstr += ', '.join([f'{t} {n}' for (n, t) in self.builtin_args])
        self.builtin_used = [b for b, v in self.builtin.items() if v.enabled]
        ccode = ''
        def process_recursion(l, decltypestr = False):
            nonlocal ccode
            for c in l:
                if type(c) is str:
                    ccode += c
                elif callable(c):
                    ccode += c(decltypestr) # a callback function
                else:
                    raise ValueError(f'Illegal operation in udf code generation: {c}')
        process_recursion(front)
        ccode += builtin_argstr + ') {\n'
        process_recursion(self.code_list)
        self.ccode += ccode + '\n'
        ccode = ''
        if self.return_pattern == udf.ReturnPattern.elemental_return:
            ccode += f'auto {self.cname}_gettype = []('
            process_recursion(front[1:], True)
            ccode += ') {\n\tuint32_t _builtin_len = 0;\n' 
            process_recursion(self.code_list, True)
            self.ccode += ccode + '\n'
            
    class ReturnPattern(Enum):
        bulk_return = auto()
        elemental_return = auto()
        
    @property
    def return_pattern(self):
        if '_builtin_ret' in self.builtin_used:
            return udf.ReturnPattern.elemental_return
        else:
            return udf.ReturnPattern.bulk_return

class passthru_sql(ast_node):
    name = 'sql'
    first_order = name
    import re
    # escapestr = r'''(?:((?:[^;"']|"[^"]*"|'[^']*')+)|(?:--[^\r\n]*[\r|\n])+)'''
    # escape_comment = fr'''(?:{escapestr}|{escapestr}*-{escapestr}*)'''
    seprator = re.compile(r'''((?:[^;"']|"[^"]*"|'[^']*')+)''')
    def __init__(self, _, node, context:Context):
        sqls = passthru_sql.seprator.split(node['sql'])
        try:
            if callable(context.parser):
                parsed = context.parser(node['sql'])
        except BaseException:
            parsed = None
        for sql in sqls:
            sq = sql.strip(' \t\n\r;')
            if sq:
                context.queries.append('Q' + sql.strip('\r\n\t ;') + ';')
                lq = sq.lower()
                if lq.startswith('select'):
                    context.direct_output()

class user_module_function(OperatorBase):
    def __init__(self, name, nargs, ret_type, context : Context):
        super().__init__(name, nargs, lambda *_: ret_type, call=fn_behavior)
        user_module_func[name] = self
        # builtin_operators[name] = self
        udf.try_init_udf(context)
        
def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node) and type(cls.first_order) is str:
            ast_node.types[cls.first_order] = cls
            
            
import sys

include(sys.modules[__name__])
