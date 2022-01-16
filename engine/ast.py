from typing import List


class TableInfo:
    def __init__(self, table_name, cols, cxt:'Context'):
        # statics
        self.table_name = table_name
        self.columns = dict() # column_name, type
        for c in cols:
            self.columns[c['name']] = ((list(c['type'].keys()))[0], c)
            k9name = self.table_name + c['name']
            if k9name in cxt.k9cols_byname: # duplicate names?
                root = cxt.k9cols_byname[k9name] 
                k9name = k9name + root[1]
                root[1] += 1
            cxt.k9cols[c] = k9name
            cxt.k9cols_byname[k9name] = (c, 1)
        # runtime
        self.n_cols = 0 # number of cols
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map

    def get_k9colname(self, cxt:'Context', col_name):
        return cxt.k9cols[self.columns[col_name][1]] # well, this is gnarly.. will change later

class Context:
    def __init__(self): 
        self.tables:List[TableInfo] = []
        self.tables_byname = dict()
        self.k9cols = dict()
        self.k9cols_byname = dict()

        self.k9code = ''

    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl

    def emit(self, codelet):
        self.k9code += codelet + '\n'

    def __str__(self):
        return self.k9code

class ast_node:
    types = dict()
    def __init__(self, parent:"ast_node", node, context:Context = None):
        self.context = parent.context if context is None else context
        self.produce(node)
        self.enumerate(node)
        self.consume(node)
    
    def emit(self, code):
        self.context.emit(code)

    name = 'null'

    def produce(self, _):
        pass
    def enumerate(self, _):
        pass
    def consume(self, _):
        pass
