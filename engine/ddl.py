# code-gen for data decl languages

from engine.ast import ColRef, TableInfo, ast_node, include
from engine.utils import base62uuid
class create_table(ast_node):
    name = 'create_table'
    def produce(self, node):
        ct = node[self.name]
        tbl = self.context.add_table(ct['name'], ct['columns'])
        # create tables in c
        self.emit(f"auto {tbl.table_name} = new TableInfo(\"{tbl.table_name}\", {tbl.n_cols});")
        self.emit("cxt->tables.insert({\"" + tbl.table_name + f"\", {tbl.table_name}"+"});")
        self.context.tables_in_context[tbl] = tbl.table_name
        tbl.cxt_name = tbl.table_name
        for i, c in enumerate(ct['columns']):
            # TODO: more self awareness
            self.emit(f"{tbl.table_name}->colrefs[{i}].ty = types::AINT;")
            
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
        node = node[self.name]
        table:TableInfo = self.context.tables_byname[node['table']]
        n_keys = len(table.columns)
        keys = ''
        for _ in range(n_keys):
            keys+='`tk'+base62uuid(6)
        tablename = 'l'+base62uuid(7)        

        self.emit(f"{tablename}:({keys}!(+(`csv ? 1:\"{node['file']['literal']}\")))[{keys}]")

        for i, c in enumerate(table.columns):
            self.emit(f'{c.cname}:{tablename}[{i}]')
            
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
                    l_cols=f'flatBOTH\'+(({ending(l_cols)});{c.cname})'
                else:
                    l_compound = True
                    if i >= 1:
                        l_cols = f'flatRO\'+(({ending(l_cols)});{c.cname})'
                    else:
                        l_cols = c.cname + ';'
            elif l_compound:
                l_cols = f'flatLO\'+(({ending(l_cols)});{c.cname})'
            else:
                l_cols += f"{c.cname};"
        if not l_compound:
            self.emit_no_ln(l_keys + '!(' + ending(l_cols) + ')')
        else:
            self.emit_no_ln(f'{l_keys}!+,/({ending(l_cols)})')
        self.emit('))')
            
import sys
include(sys.modules[__name__])