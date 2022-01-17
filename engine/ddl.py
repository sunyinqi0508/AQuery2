# code-gen for data decl languages

from engine.ast import TableInfo, ast_node, include

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
        
import sys
include(sys.modules[__name__])