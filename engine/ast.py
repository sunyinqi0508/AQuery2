from typing import List
import uuid


class TableInfo:
    def __init__(self, table_name, cols, cxt:'Context'):
        # statics
        self.table_name = table_name
        self.columns_byname = dict() # column_name, type
        self.columns = []

        for c in cols:
            k9name = self.table_name + c['name']
            if k9name in cxt.k9cols_byname: # duplicate names?
                root = cxt.k9cols_byname[k9name] 
                k9name = k9name + root[3]
                root[3] += 1

            # column: (k9name, type, original col_object, dup_count)
            col_object =  (k9name, (list(c['type'].keys()))[0], c, 1)

            cxt.k9cols_byname[k9name] = col_object
            self.columns_byname[c['name']] = col_object
            self.columns.append(col_object)

        # runtime
        self.n_rows = 0 # number of cols
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map

    @property
    def n_cols(self):
        return len(self.columns)

    def get_k9colname(self, col_name):
        return self.columns_byname[col_name][0]

    def parse_tablenames(self, str):
        # TODO: deal with alias
        return self.get_k9colname(str)

class Context:
    def __init__(self): 
        self.tables:List[TableInfo] = []
        self.tables_byname = dict()
        self.k9cols_byname = dict()

        self.udf_map = dict()

        self.k9code = ''

    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl

    def gen_tmptable(self):
        from engine.utils import base62uuid
        return f'tmp{base62uuid()}'

    def emit(self, codelet):
        self.k9code += codelet + '\n'
    def emit_nonewline(self, codelet):
        self.k9code += codelet
    def __str__(self):
        return self.k9code

class ast_node:
    types = dict()
    def __init__(self, parent:"ast_node", node, context:Context = None):
        self.context = parent.context if context is None else context
        self.init(node)
        self.produce(node)
        self.spawn(node)
        self.consume(node)
    
    def emit(self, code):
        self.context.emit(code)
    def emit_no_ln(self, code):
        self.context.emit_nonewline(code)

    name = 'null'

    # each ast node has 3 stages. 
    # `produce' generates info for child nodes
    # `spawn' populates child nodes
    # `consume' consumes info from child nodes and finalizes codegen
    # For simple operators, there may not be need for some of these stages
    def init(self, _):
        pass
    def produce(self, _):
        pass
    def spawn(self, _):
        pass
    def consume(self, _):
        pass

# include classes in module as first order operators
def include(objs):
    import inspect
    for _, cls in inspect.getmembers(objs):
        if inspect.isclass(cls) and issubclass(cls, ast_node):
            ast_node.types[cls.name] = cls