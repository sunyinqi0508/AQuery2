# code-gen for data decl languages

from engine.ast import ColRef, TableInfo, ast_node, Context, include
from engine.scan import scan
from engine.utils import base62uuid

class create_table(ast_node):
    name = 'create_table'
    def __init__(self, parent: "ast_node", node, context: Context = None, cexprs = None):
        self.cexprs = cexprs
        super().__init__(parent, node, context)
    def produce(self, node):
        if type(node) is not TableInfo:
            ct = node[self.name]
            tbl = self.context.add_table(ct['name'], ct['columns'])
        else:
            tbl = node
            
        col_type_str = ','.join([c.type for c in tbl.columns])
        # create tables in c
        self.emit(f"auto {tbl.table_name} = new TableInfo<{col_type_str}>(\"{tbl.table_name}\", {tbl.n_cols});")
        self.emit("cxt->tables.insert({\"" + tbl.table_name + f"\", {tbl.table_name}"+"});")
        self.context.tables_in_context[tbl] = tbl.table_name
        tbl.cxt_name = tbl.table_name
        tbl.refer_all()
        # create an empty new table
        if self.cexprs is None:
            for c in tbl.columns:
                self.emit(f"{c.cxt_name}.init();")
        # create an output table
        else:
            if len(self.context.scans) == 0:
                for i, c in enumerate(tbl.columns):
                    self.emit(f"{c.cxt_name}.init();")
                    self.emit(f"{c.cxt_name} = {self.cexprs[i]()};")
            else:
                scanner:scan = self.context.scans[-1]
                for i, c in enumerate(tbl.columns):
                    scanner.add(f"{c.cxt_name}.init();", "init")
                    scanner.add(f"{c.cxt_name} = {self.cexprs[i](scanner.it_ver)};")

class insert(ast_node):
    name = 'insert'
    def produce(self, node):
        ct = node[self.name]
        table:TableInfo = self.context.tables_byname[ct]

        values = node['query']['select']
        if len(values) != table.n_cols:
            raise ValueError("Column Mismatch")
        table.refer_all()
        for i, s in enumerate(values):
            if 'value' in s:
                cname = table.columns[i].cxt_name
                self.emit(f"{cname}.emplace_back({s['value']});")
            else:
                # subquery, dispatch to select astnode
                pass
            
class c(ast_node):
    name='c'
    def produce(self, node):
        self.emit(node[self.name])
        
class load(ast_node):
    name="load"
    def produce(self, node):
        self.context.headers.add('"csv.h"')
        node = node[self.name]
        table:TableInfo = self.context.tables_byname[node['table']]
        table.refer_all()
        csv_reader_name = 'csv_reader_' + base62uuid(6)
        col_types = [c.type for c in table.columns]
        col_tmp_names = ['tmp_'+base62uuid(8) for _ in range(len(table.columns))]
        # col_type_str = ",".join(col_types)
        col_names = ','.join([f'"{c.name}"' for c in table.columns])
        
        self.emit(f'io::CSVReader<{len(col_types)}> {csv_reader_name}("{node["file"]["literal"]}");')
        self.emit(f'{csv_reader_name}.read_header(io::ignore_extra_column, {col_names});')
        for t, n in zip(col_types, col_tmp_names):
            self.emit(f'{t} {n};')
        self.emit(f'while({csv_reader_name}.read_row({",".join(col_tmp_names)})) {{ \n')
        for i, c in enumerate(table.columns):
            self.emit(f'{c.cxt_name}.emplace_back({col_tmp_names[i]});')
        self.emit('}')
        
            
class outfile(ast_node):
    name="_outfile"
    def produce(self, node):
        out_table:TableInfo = self.parent.out_table
        filename = node['loc']['literal'] if 'loc' in node else node['literal']
        sep = ',' if 'term' not in node else node['term']['literal']
        file_pointer = 'fp_' + base62uuid(6)
        self.emit(f'FILE* {file_pointer} = fopen("{filename}", "w");')
        self.emit(f'{out_table.cxt_name}->printall("{sep}", "\\n", nullptr, {file_pointer});')
        self.emit(f'fclose({file_pointer});')
        # self.context.headers.add('fstream')
        # cout_backup_buffer = 'stdout_' + base62uuid(4)
        # ofstream = 'ofstream_' + base62uuid(6)
        # self.emit(f'auto {cout_backup_buffer} = cout.rdbuf();')
        # self.emit(f'auto {ofstream} = ofstream("{filename}");')
        # self.emit(f'cout.rdbuf({ofstream}.rdbuf());')
        # TODO: ADD STMTS.
        # self.emit(f'cout.rdbuf({cout_backup_buffer});')
        # self.emit(f'{ofstream}.close();')
        
        
import sys
include(sys.modules[__name__])