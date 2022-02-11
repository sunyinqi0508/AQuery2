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
        table:TableInfo = self.context.tables_byname[node['table']]
        n_keys = len(table.columns)
        keys = ''
        for _ in range(n_keys):
            keys+='`tk'+base62uuid(6)
        tablename = 'l'+base62uuid(7)        

        self.emit(f"{tablename}:({keys}!(+(`csv ? 1:\"{node['file']['literal']}\")))[{keys}]")

        for i, c in enumerate(table.columns):
            self.emit(f'{c.k9name}:{tablename}[{i}]')
            
class outfile(ast_node):
    name="_outfile"
    def produce(self, node):
        out_table:TableInfo = self.parent.out_table
        filename = node['loc']['literal'] if 'loc' in node else node['literal']
        self.emit_no_ln(f"\"{filename}\"1:`csv@(+(")
        l_compound = False
        l_cols = ''
        l_keys = ''
        ending = lambda x: x[:-1] if len(x) > 0 and x[-1]==';' else x
        for i, c in enumerate(out_table.columns):
            c:ColRef
            l_keys += '`' + c.name
            if c.compound:
                if l_compound:
                    l_cols=f'flatBOTH\'+(({ending(l_cols)});{c.k9name})'
                else:
                    l_compound = True
                    if i >= 1:
                        l_cols = f'flatRO\'+(({ending(l_cols)});{c.k9name})'
                    else:
                        l_cols = c.k9name + ';'
            elif l_compound:
                l_cols = f'flatLO\'+(({ending(l_cols)});{c.k9name})'
            else:
                l_cols += f"{c.k9name};"
        if not l_compound:
            self.emit_no_ln(l_keys + '!(' + ending(l_cols) + ')')
        else:
            self.emit_no_ln(f'{l_keys}!+,/({ending(l_cols)})')
        self.emit('))')
            
import sys
include(sys.modules[__name__])