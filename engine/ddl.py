# code-gen for data decl languages

from engine.ast import ColRef, TableInfo, ast_node, include
from engine.utils import base62uuid
class create_table(ast_node):
    name = 'create_table'
    def produce(self, node):
        ct = node[self.name]
        tbl = self.context.add_table(ct['name'], ct['columns'])
        # create tables in k9
        for c in ct['columns']:
            self.emit(f"{tbl.get_k9colname(c['name'])}:()")

class insert(ast_node):
    name = 'insert'
    def produce(self, node):
        ct = node[self.name]
        table:TableInfo = self.context.tables_byname[ct]

        values = node['query']['select']
        if len(values) != table.n_cols:
            raise ValueError("Column Mismatch")
        for i, s in enumerate(values):
            if 'value' in s:
                k9name = table.columns[i][0]
                self.emit(f"{k9name}:{k9name},{s['value']}")
            else:
                # subquery, dispatch to select astnode
                pass
            
class k9(ast_node):
    name='k9'
    def produce(self, node):
        self.emit(node[self.name])
        
class load(ast_node):
    name="load"
    def produce(self, node):
        node = node[self.name]
        tablename = 'l'+base62uuid(7)
        keys = 'k'+base62uuid(7)
        self.emit(f"{tablename}:`csv ? 1:\"{node['file']['literal']}\"")
        self.emit(f"{keys}:!{tablename}")
        table:TableInfo = self.context.tables_byname[node['table']]
        
        for i, c in enumerate(table.columns):
            c:ColRef
            self.emit(f'{c.k9name}:{tablename}[({keys})[{i}]]')
            
class outfile(ast_node):
    name="_outfile"
    def produce(self, node):
        out_table:TableInfo = self.parent.out_table
        self.emit_no_ln(f"\"{node['loc']['literal']}\"1:`csv@[[]")
        for i, c in enumerate(out_table.columns):
            self.emit_no_ln(f"{c.name}:{c.k9name}{';' if i < len(out_table.columns) - 1 else ''}")
        self.emit(']')
            
import sys
include(sys.modules[__name__])