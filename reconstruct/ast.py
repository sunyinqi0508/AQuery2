from re import T
from typing import Set, Tuple
from engine.types import *
from engine.utils import enlist, base62uuid, base62alp, get_leagl_name
from reconstruct.storage import Context, TableInfo, ColRef
    
class ast_node:
    header = []
    types = dict()
    first_order = False
    
    def __init__(self, parent:"ast_node", node, context:Context = None):
        self.context = parent.context if context is None else context
        self.parent = parent
        self.sql = ''
        self.datasource = None
        self.init(node)
        self.produce(node)
        self.spawn(node)
        self.consume(node)
    
    def emit(self, code):
        self.context.emit(code)
    def add(self, code):
        self.sql += code + ' '
        
    name = 'null'
    
    def init(self, _):
        self.add(self.__class__.name.upper())
    def produce(self, _):
        pass
    def spawn(self, _):
        pass
    
    def consume(self, _):
        if self.parent is None:
            self.emit(self.sql+';\n')
        
        
from reconstruct.expr import expr


class projection(ast_node):
    name = 'projection'
    first_order = 'select'
    
    def init(self, _):
        pass
    def produce(self, node):
        p = node['select']
        self.projections = p if type(p) is list else [p]
        self.add('SELECT')
        
    def spawn(self, node):
        self.datasource = None # datasource is Join instead of TableInfo
        if 'from' in node:
            from_clause = node['from']
            self.datasource = join(self, from_clause)
            if 'assumptions' in from_clause:
                self.assumptions = enlist(from_clause['assumptions'])
                    
        if self.datasource is not None:
            self.datasource_changed = True
            self.prev_datasource = self.context.datasource
            self.context.datasource = self.datasource     
                   
        if 'where' in node:
            self.where = filter(self, node['where'])
        else:
            self.where = None    

        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
        else:
            self.group_node = None

    def consume(self, node):
        # deal with projections
        self.out_table = TableInfo('out_'+base62uuid(4), [], self.context)
        cols = []
        col_ext : Set[ColRef]= set()
        col_exprs : List[Tuple[str, Types]] = []
        
        proj_map = dict()
        var_table = dict()
        
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
                    if not proj_expr.is_special:
                        y = lambda x:x
                        name = eval('f\'' + name + '\'')
                        if name not in var_table:
                            var_table[name] = len(col_exprs)
                        proj_map[i] = [this_type, len(col_exprs)]
                        col_exprs.append((name, proj_expr.type))
                    else:
                        self.context.headers.add('"./server/aggregations.h"')
                        if self.datasource.rec is not None:
                            col_ext = col_ext.union(self.datasource.rec)
                        proj_map[i] = [this_type, proj_expr.sql]
                        
                    if 'name' in proj: # renaming column by AS keyword
                        name += ' AS ' +  proj['name']
                        if not proj_expr.is_special:
                            var_table[proj['name']] = len(col_exprs)
                    
                    disp_name = get_leagl_name(name)
                    
            elif type(proj) is str:
                col = self.datasource.get_col(proj)
                this_type = col.type
                name = col.name
            self.datasource.rec = None
            # TODO: Type deduction in Python
            cols.append(ColRef(this_type, self.out_table, None, disp_name, i, compound=compound))
        col_ext = [c for c in col_ext if c.name not in var_table] # remove duplicates in var_table
        col_ext_names = [c.name for c in col_ext]
        self.add(', '.join([c[0] for c in col_exprs] + col_ext_names))
    
        _base_offset = len(col_exprs)
        for i, col in enumerate(col_ext_names):
            if col not in var_table:
                var_table[col] = i + _base_offset
    
        def finialize(astnode:ast_node):
            if(astnode is not None):
                self.add(astnode.sql)
        self.add('FROM')        
        finialize(self.datasource)                
        finialize(self.where)
        finialize(self.group_node)
        if 'orderby' in node:
            self.add(orderby(self, node['orderby']).sql)
        if 'outfile' in node:
            self.sql = outfile(self, node['outfile'], sql = self.sql).sql
        if self.parent is None:
            self.emit(self.sql+';\n')
        else: 
            # TODO: subquery, name create tmp-table from subquery w/ alias as name 
            pass
        # cpp module codegen
        self.context.has_dll = True
        # extract typed-columns from result-set
        vid2cname = [0]*len(var_table)
        pyname2cname = dict()
        typenames = [c[1] for c in col_exprs] + [c.type for c in col_ext]
        length_name = 'len_' + base62uuid(6)
        self.context.emitc(f'auto {length_name} = server->cnt;')
        
        for v, idx in var_table.items():
            vname = get_leagl_name(v) + '_' + base62uuid(3)
            pyname2cname[v] = vname
            self.context.emitc(f'auto {vname} = ColRef<{typenames[idx].cname}>({length_name}, server->getCol({idx}));')
            vid2cname[idx] = vname
        # Create table into context
        outtable_name = 'out_' + base62uuid(6)
        out_typenames = [None] * len(proj_map)
        for key, val in proj_map.items():
            if type(val[1]) is str:
                x = True
                y = lambda t: pyname2cname[t]
                val[1] = eval('f\'' + val[1] + '\'')
            if val[0] == LazyT:
                out_typenames[key] = f'value_type<decays<decltype({val[1]})>>'
            else:
                out_typenames[key] = val[0].cname
            
        # out_typenames = [v[0].cname for v in proj_map.values()]
        self.context.emitc(f'auto {outtable_name} = new TableInfo<{",".join(out_typenames)}>("{outtable_name}");')
        
        for key, val in proj_map.items():
            if type(val[1]) is int:
                self.context.emitc(f'{outtable_name}->get_col<{key}>().initfrom({vid2cname[val[1]]});')
            else:
        # for funcs evaluate f_i(x, ...)
                self.context.emitc(f'{outtable_name}->get_col<{key}>() = {val[1]};')
        # print out col_is
        self.context.emitc(f'print(*{outtable_name});')
        
class orderby(ast_node):
    name = 'order by'
    def produce(self, node):
        if node is None:
            self.sql = ''
            return
        elif type(node) is not list:
            node = [node]
        
        o_list = []
        
        for o in node:
            o_str = expr(self, o['value']).sql
            if 'sort' in o and f'{o["sort"]}'.lower() == 'desc':
                o_str += ' ' + 'DESC'
            o_list.append(o_str)
        self.add(', '.join(o_list))
            
            
class groupby(orderby):
    name = 'group by'


class join(ast_node):
    name = 'join'
    def init(self, _):
        self.joins:list = []
        self.tables = []
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
            self.append(self.context.tables_byname[node])
    
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
        super().__init__(parent, node, context)
        self.sql = sql
        if self.context.dialect == 'MonetDB':
            self.produce = self.produce_monetdb
        else:
            self.produce = self.produce_aq
            
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

class udf(ast_node):
    name = 'udf'
    first_order = name
    def __call__(self, c_code = False, *args):
        from engine.types import fn_behavior
        return fn_behavior(self, c_code, *args)
    def return_type(self, *_ : Types):
        return LazyT
    def init(self, node):
        self.var_table = {}
        self.args = []
        if self.context.udf is None:
            self.context.udf = Context.udf_head
            self.context.headers.add('\"./udf.hpp\"')
            
    def produce(self, node):
        from engine.utils import get_leagl_name, check_leagl_name
        node = node[self.name]
        # register udf
        self.cname = get_leagl_name(node['fname'])
        self.sqlname = self.cname
        self.context.udf_map[self.cname] = self
        self.ccode = f'auto {self.cname} = []('
        
    def consume(self, node):
        from engine.utils import get_leagl_name, check_leagl_name
        node = node[self.name]
        
        if 'params' in node:
            for args in node['params']:
                cname = get_leagl_name(args)
                self.var_table[args] = cname
                self.args.append(cname)
        self.ccode += ', '.join([f'auto {a}' for a in self.args]) + ') {\n'
        
        if 'assignment' in node:
            for assign in node['assignment']:
                var = assign['var']
                ex = expr(self, assign['expr'], c_code=True)
                if var in self.var_table:
                    self.ccode += f'\t{var} = {ex.eval()};\n'
                else:
                    cvar = get_leagl_name(var)
                    self.var_table[var] = cvar
                    self.ccode += f'\tauto {cvar} = {ex.eval()};\n'
                    
        ret = node['ret']
        self.ccode += f'\treturn {expr(self, ret, c_code=True).eval()};'
        self.ccode += '\n};\n'
        print(self.ccode)
        
        self.context.udf += self.ccode + '\n'
    
def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node) and type(cls.first_order) is str:
            ast_node.types[cls.first_order] = cls
            
            
import sys
include(sys.modules[__name__])