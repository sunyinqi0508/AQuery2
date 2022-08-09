from copy import deepcopy
from dataclasses import dataclass
from enum import Enum, auto
from typing import Set, Tuple, Dict, Union, List, Optional
from engine.types import *
from engine.utils import enlist, base62uuid, base62alp, get_legal_name
from reconstruct.storage import Context, TableInfo, ColRef
    
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
    def add(self, code):
        self.sql += code + ' '
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
        
from reconstruct.expr import expr, fastscan


class projection(ast_node):
    name = 'projection'
    first_order = 'select'
    
    def init(self, _):
        # skip default init
        pass
    
    def produce(self, node):
        p = node['select']
        self.projections = p if type(p) is list else [p]
        self.add('SELECT')
        if self.parent is None:
            self.context.sql_begin()
            self.postproc_fname = 'dll_' + base62uuid(6)
            self.context.postproc_begin(self.postproc_fname)
        
    def spawn(self, node):
        self.datasource = None # datasource is Join instead of TableInfo
        if 'from' in node:
            from_clause = node['from']
            self.datasource = join(self, from_clause)
            if 'assumptions' in from_clause:
                self.assumptions = enlist(from_clause['assumptions'])
            else:
                self.assumptions = []
        if self.datasource is not None:
            self.datasource_changed = True
            self.prev_datasource = self.context.datasource
            self.context.datasource = self.datasource     
                   
        if 'where' in node:
            self.where = filter(self, node['where'])
        else:
            self.where = None    


    def consume(self, node):
        # deal with projections
        self.out_table = TableInfo('out_'+base62uuid(4), [], self.context)
        cols = []
        self.col_ext : Set[ColRef]= set()
        col_exprs : List[Tuple[str, Types]] = []
        
        proj_map : Dict[int, List[Union[Types, int, str, expr]]]= dict()
        self.var_table = dict()
        # self.sp_refs = set()
        for i, proj in enumerate(self.projections):
            compound = False
            self.datasource.rec = set()
            name = ''
            this_type = AnyT
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    proj_expr = expr(self, e)
                    this_type = proj_expr.type
                    name = proj_expr.sql
                    compound = True # compound column
                    proj_expr.cols_mentioned = self.datasource.rec
                    if not proj_expr.is_special:
                        y = lambda x:x
                        name = eval('f\'' + name + '\'')
                        if name not in self.var_table:
                            self.var_table[name] = len(col_exprs)
                        proj_map[i] = [this_type, len(col_exprs), proj_expr]
                        col_exprs.append((name, proj_expr.type))
                    else:
                        self.context.headers.add('"./server/aggregations.h"')
                        if self.datasource.rec is not None:
                            self.col_ext = self.col_ext.union(self.datasource.rec)
                        proj_map[i] = [this_type, proj_expr.sql, proj_expr]
                    if 'name' in proj: # renaming column by AS keyword
                        name += ' AS ' +  proj['name']
                        if not proj_expr.is_special:
                            self.var_table[proj['name']] = len(col_exprs)
                    
                    disp_name = get_legal_name(name)
                    
            elif type(proj) is str:
                col = self.datasource.get_col(proj)
                this_type = col.type
                # name = col.name
            self.datasource.rec = None
            # TODO: Type deduction in Python
            cols.append(ColRef(this_type, self.out_table, None, disp_name, i, compound=compound))
        
        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
        else:
            self.group_node = None
        
        self.col_ext = [c for c in self.col_ext if c.name not in self.var_table] # remove duplicates in self.var_table
        col_ext_names = [c.name for c in self.col_ext]
        self.add(', '.join([c[0] for c in col_exprs] + col_ext_names))
    
        _base_offset = len(col_exprs)
        for i, col in enumerate(col_ext_names):
            if col not in self.var_table:
                self.var_table[col] = i + _base_offset
    
    
        def finialize(astnode:ast_node):
            if(astnode is not None):
                self.add(astnode.sql)
        self.add('FROM')
        finialize(self.datasource)                
        finialize(self.where)
        if self.group_node and not self.group_node.use_sp_gb:
            self.add(self.group_node.sql)

        if self.col_ext or self.group_node and self.group_node.use_sp_gb:
            self.use_postproc = True
        
        o = self.assumptions
        if 'orderby' in node:
            o.extend(enlist(node['orderby']))
        if o:
            self.add(orderby(self, o).sql)
        
        if 'outfile' in node:
            self.outfile = outfile(self, node['outfile'], sql = self.sql)
            if not self.use_postproc:
                self.sql += self.outfile.sql
        else:
            self.outfile = None
            
        if self.parent is None:
            self.emit(self.sql+';\n')
        else: 
            # TODO: subquery, name create tmp-table from subquery w/ alias as name 
            pass
        
        
        # cpp module codegen
        self.context.has_dll = True
        # extract typed-columns from result-set
        vid2cname = [0]*len(self.var_table)
        self.pyname2cname = dict()
        typenames = [c[1] for c in col_exprs] + [c.type for c in self.col_ext]
        length_name = 'len_' + base62uuid(6)
        self.context.emitc(f'auto {length_name} = server->cnt;')
        
        for v, idx in self.var_table.items():
            vname = get_legal_name(v) + '_' + base62uuid(3)
            self.pyname2cname[v] = vname
            self.context.emitc(f'auto {vname} = ColRef<{typenames[idx].cname}>({length_name}, server->getCol({idx}));')
            vid2cname[idx] = vname
        # Create table into context
        self.outtable_name = 'out_' + base62uuid(6)
        out_typenames = [None] * len(proj_map)
        
        for key, val in proj_map.items():
            if type(val[1]) is str:
                x = True
                y = lambda t: self.pyname2cname[t]
                val[1] = val[2].eval(x, y, gettype=True)
                if callable(val[1]):
                    val[1] = val[1](True)
                decltypestring = val[1] 
                
            if val[0] == LazyT:
                decltypestring = f'value_type<decays<decltype({decltypestring})>>'
                out_typenames[key] = decltypestring
            else:
                out_typenames[key] = val[0].cname
            if (type(val[2].udf_called) is udf and 
                    val[2].udf_called.return_pattern == udf.ReturnPattern.elemental_return
                    or 
                    self.group_node and self.group_node.use_sp_gb and
                    val[2].cols_mentioned.intersection(
                        self.datasource.all_cols.difference(self.group_node.refs))
                    ):
                    out_typenames[key] = f'ColRef<{out_typenames[key]}>'
            
        # out_typenames = [v[0].cname for v in proj_map.values()]
        self.context.emitc(f'auto {self.outtable_name} = new TableInfo<{",".join(out_typenames)}>("{self.outtable_name}");')
        # TODO: Inject custom group by code here and flag them in proj_map
        # Type of UDFs? Complex UDFs, ones with static vars?
        if self.group_node is not None and self.group_node.use_sp_gb:
            gb_vartable : Dict[str, Union[str, int]] = deepcopy(self.pyname2cname)
            gb_cexprs : List[str] = []
            
            for key, val in proj_map.items():
                col_name = 'col_' + base62uuid(6)
                self.context.emitc(f'decltype(auto) {col_name} = {self.outtable_name}->get_col<{key}>();')
                gb_cexprs.append((col_name, val[2]))
            self.group_node.finalize(gb_cexprs, gb_vartable)
        else:
            for key, val in proj_map.items():
                if type(val[1]) is int:
                    self.context.emitc(f'{self.outtable_name}->get_col<{key}>().initfrom({vid2cname[val[1]]});')
                else:
            # for funcs evaluate f_i(x, ...)
                    self.context.emitc(f'{self.outtable_name}->get_col<{key}>() = {val[1]};')
        # print out col_is
        self.context.emitc(f'print(*{self.outtable_name});')
        
        if self.outfile:
            self.outfile.finalize()
        self.context.emitc(f'puts("done.");')

        if self.parent is None:
            self.context.sql_end()
            self.context.postproc_end(self.postproc_fname)
            
        
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
    def __init__(self, parent: "ast_node", node, loop_style = 'for', context: Context = None, const = False):
        self.const = "const " if const else ""
        self.loop_style = loop_style
        super().__init__(parent, node, context)
        
    def init(self, _):
        self.datasource = self.context.datasource
        self.initializers = ''
        self.start = ''
        self.front = ''
        self.body = ''
        self.end = '}'
        scan_vars = set(s.it_var for s in self.context.scans)
        self.it_ver = 'i' + base62uuid(2)
        while(self.it_ver in scan_vars):
            self.it_ver = 'i' + base62uuid(6)
        self.parent.context.scans.append(self)
        
    def produce(self, node):
        if self.loop_style == 'for_each':
            self.colref = node
            self.start += f'for ({self.const}auto& {self.it_ver} : {node}) {{\n'
        else:
            self.start += f"for (uint32_t {self.it_ver} = 0; {self.it_ver} < {node}; ++{self.it_ver}){{\n"
            
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
        return super().init(node)
    def produce(self, node : List[Tuple[expr, Set[ColRef]]]):
        self.context.headers.add('"./server/hasher.h"')
        self.context.headers.add('unordered_map')
        self.group = 'g' + base62uuid(7)
        self.group_type = 'record_type' + base62uuid(7)
        self.datasource = self.proj.datasource
        self.scanner = None
        self.datasource.rec = set()
        
        g_contents = ''
        g_contents_list = []
        first_col = ''
        
        for g in self.glist:
            e = expr(self, g[0].node, c_code=True)
            g_str = e.eval(c_code = True, y = lambda c: self.proj.pyname2cname[c])
            # if v is compound expr, create tmp cols
            if e.is_ColExpr:
                tmpcol = 't' + base62uuid(7)
                self.context.emitc(f'auto {tmpcol} = {g_str};')
                e = tmpcol
            g_contents_list.append(e)
        first_col = g_contents_list[0]
        g_contents_decltype = [f'decays<decltype({c})::value_t>' for c in g_contents_list]
        g_contents = ','.join(g_contents_list)
        self.context.emitc(f'typedef record<{",".join(g_contents_decltype)}> {self.group_type};')
        self.context.emitc(f'unordered_map<{self.group_type}, vector_type<uint32_t>, '
            f'transTypes<{self.group_type}, hasher>> {self.group};')
        self.n_grps = len(self.glist)
        self.scanner = scan(self, first_col + '.size')
        self.scanner.add(f'{self.group}[forward_as_tuple({g_contents}[{self.scanner.it_ver}])].emplace_back({self.scanner.it_ver});')

    def consume(self, _):
        self.scanner.finalize()
        
    # def deal_with_assumptions(self, assumption:assumption, out:TableInfo):
    #     gscanner = scan(self, self.group)
    #     val_var = 'val_'+base62uuid(7)
    #     gscanner.add(f'auto &{val_var} = {gscanner.it_ver}.second;')
    #     gscanner.add(f'{self.datasource.cxt_name}->order_by<{assumption.result()}>(&{val_var});')
    #     gscanner.finalize()
        
    def finalize(self, cexprs : List[Tuple[str, expr]], var_table : Dict[str, Union[str, int]]):
        gscanner = scan(self, self.group, loop_style = 'for_each')
        key_var = 'key_'+base62uuid(7)
        val_var = 'val_'+base62uuid(7)
        
        gscanner.add(f'auto &{key_var} = {gscanner.it_ver}.first;', position = 'front')
        gscanner.add(f'auto &{val_var} = {gscanner.it_ver}.second;', position = 'front')
        len_var = None
        def define_len_var():
            nonlocal len_var
            if len_var is None:
                len_var = 'len_'+base62uuid(7)
                gscanner.add(f'auto &{len_var} = {val_var}.size;', position = 'front')
        
        def get_key_idx (varname : str):
            for i, g in enumerate(self.glist):
                if varname == g[0].eval():
                   return i 
            return var_table[varname]
        
        def get_var_names (varname : str):
            var = get_key_idx(varname)
            if type(var) is str:
                return f'{var}[{val_var}]'
            else:
                return f'get<{var}>({key_var})'
        
        for ce in cexprs:
            ex = ce[1]
            materialize_builtin = {}
            if type(ex.udf_called) is udf:
                if '_builtin_len' in ex.udf_called.builtin_used:
                    define_len_var()
                    materialize_builtin['_builtin_len'] = len_var
                if '_builtin_ret' in ex.udf_called.builtin_used:
                    define_len_var()
                    gscanner.add(f'{ce[0]}.emplace_back({{{len_var}}});\n')
                    materialize_builtin['_builtin_ret'] = f'{ce[0]}.back()'
                    gscanner.add(f'{ex.eval(c_code = True, y=get_var_names, materialize_builtin = materialize_builtin)};\n')
                    continue
            gscanner.add(f'{ce[0]}.emplace_back({ex.eval(c_code = True, y=get_var_names, materialize_builtin = materialize_builtin)});\n')
        
        gscanner.finalize()
        
        self.datasource.groupinfo = None

            
class groupby(ast_node):
    name = 'group by'
    def produce(self, node):
        if type(self.parent) is not projection:
            raise ValueError('groupby can only be used in projection')
        
        node = enlist(node)
        o_list = []
        self.refs = set()
        self.dedicated_glist : List[Tuple[expr, Set[ColRef]]] = []
        self.use_sp_gb = False
        for g in node:
            self.datasource.rec = set()
            g_expr = expr(self, g['value'])
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
            
        if not self.use_sp_gb:
            self.dedicated_gb = None
            self.add(', '.join(o_list))
        else:
            for l in self.dedicated_glist:
                # l_exist = l[1].difference(self.parent.col_ext)
                # for l in l_exist:
                #     self.parent.var_table.
                self.parent.col_ext.update(l[1])    
                
    def finalize(self, cexprs : List[Tuple[str, expr]], var_table : Dict[str, Union[str, int]]):
        if self.use_sp_gb:
            self.dedicated_gb = groupby_c(self.parent, self.dedicated_glist)
            self.dedicated_gb.finalize(cexprs, var_table)
    
class join(ast_node):
    name = 'join'
    def init(self, _):
        self.joins:list = []
        self.tables : List[TableInfo] = []
        self.tables_dir = dict()
        self.rec = None
        # self.tmp_name = 'join_' + base62uuid(4)
        # self.datasource = TableInfo(self.tmp_name, [], self.context)
    def append(self, tbls, __alias = ''):
        alias = lambda t : '(' + t + ') ' + __alias if len(__alias) else t
        if type(tbls) is join:
            self.joins.append(alias(tbls.__str__()))
            self.tables += tbls.tables
            self.tables_dir = {**self.tables_dir, **tbls.tables_dir}
            
        elif type(tbls) is TableInfo:
            self.joins.append(alias(tbls.table_name))
            self.tables.append(tbls)
            self.tables_dir[tbls.table_name] = tbls
            for a in tbls.alias:
                self.tables_dir[a] = tbls
            
        elif type(tbls) is projection:
            self.joins.append(alias(tbls.finalize()))
            
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
                keys = node.keys()
                if keys[0].lower().endswith('join'):
                    j = join(self, node[keys[0]])
                    tablename = f' {keys[0]} {j}'
                    if keys[1].lower() == 'on':
                        tablename += f' on {expr(self, node[keys[1]])}' 
                    self.joins.append(tablename)
                    self.tables += j.tables
                    self.tables_dir = {**self.tables_dir, **j.tables_dir}
                
        elif type(node) is str:
            if node in self.context.tables_byname:
                self.append(self.context.tables_byname[node])
            else:
                print(f'Error: table {node} not found.')
    
    def get_cols(self, colExpr: str) -> ColRef:
        for t in self.tables:
            if colExpr in t.columns_byname:
                col = t.columns_byname[colExpr]
                if type(self.rec) is set:
                    self.rec.add(col)
                return col
            
    def parse_col_names(self, colExpr:str) -> ColRef:
        parsedColExpr = colExpr.split('.')
        if len(parsedColExpr) <= 1:
            return self.get_cols(colExpr)
        else:
            datasource = self.tables_dir[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                return datasource.parse_col_names(parsedColExpr[1])
    @property
    def all_cols(self):
        return set([c for t in self.tables for c in t.columns])
    def consume(self, _):
        self.sql = ', '.join(self.joins)
        return super().consume(_)
    def __str__(self):
        return ', '.join(self.joins)
    def __repr__(self):
        return self.__str__()
    
    
class filter(ast_node):
    name = 'where'
    def produce(self, node):
        self.add(expr(self, node).sql)
    
        
class create_table(ast_node):
    name = 'create_table'
    first_order = name
    def init(self, node):
        if self.parent is None:
            self.context.sql_begin()
        self.sql = 'CREATE TABLE '
    
    def produce(self, node):
        ct = node[self.name]
        tbl = self.context.add_table(ct['name'], ct['columns'])
        self.sql = f'CREATE TABLE {tbl.table_name}('
        columns = []
        for c in tbl.columns:
            columns.append(f'{c.name} {c.type.sqlname}')
        self.sql += ', '.join(columns)
        self.sql += ')'
        if self.context.use_columnstore:
            self.sql += ' engine=ColumnStore'
                    
class insert(ast_node):
    name = 'insert'
    first_order = name
    
    def produce(self, node):
        values = node['query']['select']
        tbl = node['insert']
        self.sql = f'INSERT INTO {tbl} VALUES('
        # if len(values) != table.n_cols:
        #     raise ValueError("Column Mismatch")
        list_values = []
        for i, s in enumerate(values):
            if 'value' in s:
                list_values.append(f"{s['value']}")
            else:
                # subquery, dispatch to select astnode
                pass
        self.sql += ', '.join(list_values) + ')'
        
        
class load(ast_node):
    name="load"
    first_order = name
    def init(self, _):
        if self.context.dialect == 'MonetDB':
            self.produce = self.produce_monetdb
        else:
            self.produce = self.produce_aq
        if self.parent is None:
            self.context.sql_begin()
            
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
                    
class outfile(ast_node):
    name="_outfile"
    def __init__(self, parent, node, context = None, *, sql = None):
        self.node = node
        super().__init__(parent, node, context)
        self.sql = sql if sql else ''
        
    def init(self, _):
        assert(type(self.parent) is projection)
        if not self.parent.use_postproc:
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
        p = os.path.abspath('.').replace('\\', '/') + '/' + filename
        self.sql = f'COPY {self.sql} INTO "{p}"'
        d = '\t'
        e = '\n'
        if 'term' in node:
            d = node['term']['literal']
        self.sql += f' delimiters \'{d}\', \'{e}\''

    def finalize(self):
        filename = self.node['loc']['literal'] if 'loc' in self.node else self.node['literal']
        sep = ',' if 'term' not in self.node else self.node['term']['literal']
        file_pointer = 'fp_' + base62uuid(6)
        self.addc(f'FILE* {file_pointer} = fopen("{filename}", "w");')
        self.addc(f'{self.parent.outtable_name}->printall("{sep}", "\\n", nullptr, {file_pointer});')
        self.addc(f'fclose({file_pointer});')  
        self.context.ccode += self.ccode

class udf(ast_node):
    name = 'udf'
    first_order = name
    @dataclass
    class builtin_var:
        enabled : bool = False
        _type : Types = AnyT  
        all = ('_builtin_len', '_builtin_ret')
        
    def decltypecall(self, c_code = False, *args):
        from engine.types import fn_behavior
        class dummy:
            def __init__(self, name):
                self.cname = name + '_gettype'
                self.sqlname = self.cname
        return fn_behavior(dummy(self.cname), c_code, *args)
    
    def __call__(self, c_code = False, *args):
        from engine.types import fn_behavior
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
        self.var_table = {}
        self.args = []
        if self.context.udf is None:
            self.context.udf = Context.udf_head
            self.context.headers.add('\"./udf.hpp\"')
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
        from engine.utils import get_legal_name, check_legal_name
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
        from engine.utils import get_legal_name, check_legal_name
        node = node[self.name]
                    
        if 'params' in node:
            for args in node['params']:
                cname = get_legal_name(args)
                self.var_table[args] = cname
                self.args.append(cname)
        front = [*self.code_list, ', '.join([f'auto {a}' for a in self.args])]
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
        self.context.udf += ccode + '\n'
        ccode = ''
        if self.return_pattern == udf.ReturnPattern.elemental_return:
            ccode += f'auto {self.cname}_gettype = []('
            process_recursion(front[1:], True)
            ccode += ') {\n\tuint32_t _builtin_len = 0;\n' 
            process_recursion(self.code_list, True)
            self.context.udf += ccode + '\n'
            
    class ReturnPattern(Enum):
        bulk_return = auto()
        elemental_return = auto()
        
    @property
    def return_pattern(self):
        if '_builtin_ret' in self.builtin_used:
            return udf.ReturnPattern.elemental_return
        else:
            return udf.ReturnPattern.bulk_return
            
        
def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node) and type(cls.first_order) is str:
            ast_node.types[cls.first_order] = cls
            
            
import sys
include(sys.modules[__name__])
