from engine.utils import enlist, base62uuid, base62alp
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
        col_exprs = []
        for i, proj in enumerate(self.projections):
            compound = False
            self.datasource.rec = set()
            name = ''
            if type(proj) is dict:
                
                if 'value' in proj:
                    e = proj['value']
                    name = expr(self, e).sql
                    disp_name = ''.join([a if a in base62alp else '' for a in name])
                    compound = True # compound column
                    if 'name' in proj: # renaming column by AS keyword
                        name += ' ' +  proj['name']
                    col_exprs.append(name)
                
            elif type(proj) is str:
                col = self.datasource.get_col(proj)
                name = col.name
            self.datasource.rec = None
            # TODO: Type deduction in Python
            cols.append(ColRef('unknown', self.out_table, None, disp_name, i, compound=compound))
        self.add(', '.join(col_exprs))
        
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
            self.add(outfile(self, node['outfile']).sql)
        if self.parent is None:
            self.emit(self.sql+';\n')
        else: 
            # TODO: subquery, name create tmp-table from subquery w/ alias as name 
            pass
        
        
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
                self.append(join(self, d).__str__())
        
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
                return t.columns_byname[colExpr]                
            
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
            columns.append(f'{c.name} {c.type.upper()}')
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
    def produce(self, node):
        node = node['load']
        s1 = 'LOAD DATA INFILE '
        s2 = 'INTO TABLE '
        s3 = 'FIELDS TERMINATED BY '
        self.sql = f'{s1} \"{node["file"]["literal"]}\" {s2} {node["table"]}'
        if 'term' in node:
            self.sql += f' {s3} \"{node["term"]["literal"]}\"'
            
            
class outfile(ast_node):
    name="_outfile"
    def produce(self, node):
        filename = node['loc']['literal'] if 'loc' in node else node['literal']
        self.sql = f'INTO OUTFILE "{filename}"'
        if 'term' in node:
            self.sql += f' FIELDS TERMINATED BY \"{node["term"]["literal"]}\"'


def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node) and type(cls.first_order) is str:
            ast_node.types[cls.first_order] = cls
            
            
import sys
include(sys.modules[__name__])