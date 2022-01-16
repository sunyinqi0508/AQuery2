from engine.ast import TableInfo, ast_node
class create_table(ast_node):
    name = 'create_table'
    def produce(self, node):
        ct = node[self.name]
        tbl = self.context.add_table(ct['name'], ct['columns'])
        # create tables in k9
        for c in ct['columns']:
            self.emit(f"{tbl.get_k9colname((list(c['name'].keys())))[0]}:()")

class insert_into(ast_node):
    name = 'insert'
    def produce(self, node):
        ct = node[self.name]
        table:TableInfo = self.context.tables_byname[ct]
        

import sys, inspect

for name, cls in inspect.getmembers(sys.modules[__name__]):
    if inspect.isclass(cls) and issubclass(cls, ast_node):
        ast_node.types[name] = cls