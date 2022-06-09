class ColRef:
    def __init__(self, _ty, cobj, table:'TableInfo', name, id, compound = False):
        self.type = _ty
        self.cobj = cobj
        self.table = table
        self.name = name
        self.alias = set()
        self.id = id # position in table
        self.compound = compound # compound field (list as a field) 
        # e.g. order by, group by, filter by expressions
        
        self.__arr__ = (_ty, cobj, table, name, id)
    def __getitem__(self, key):
        if type(key) is str:
            return getattr(self, key)
        else:
            return self.__arr__[key]

    def __setitem__(self, key, value):
        self.__arr__[key] = value


class TableInfo:
    def __init__(self, table_name, cols, cxt:'Context'):
        # statics
        self.table_name = table_name
        self.alias = set([table_name])
        self.columns_byname = dict() # column_name, type
        self.columns = []
        self.cxt = cxt
        # keep track of temp vars
        self.rec = None 
        self.add_cols(cols)
        # runtime
        self.order = [] # assumptions

        cxt.tables_byname[self.table_name] = self # construct reverse map

    def add_cols(self, cols, new = True):
        for i, c in enumerate(cols):
            self.add_col(c, new, i)
    def add_col(self, c, new = True, i = 0):
        _ty = c['type']
        if new:
            _ty = _ty if type(c) is ColRef else list(_ty.keys())[0]
            col_object =  ColRef(_ty, c, self, c['name'], len(self.columns))
        else:
            col_object = c
            c.table = self
        self.columns_byname[c['name']] = col_object
        self.columns.append(col_object)
        
    def add_alias(self, alias):
        if alias in self.cxt.tables_byname.keys():
            print("Error: table alias already exists")
            return
        self.cxt.tables_byname[alias] = self
        self.alias.add(alias)
    def parse_col_names(self, colExpr) -> ColRef:
        parsedColExpr = colExpr.split('.')
        if len(parsedColExpr) <= 1:
            return self.columns_byname[colExpr]
        else:
            datasource = self.cxt.tables_byname[parsedColExpr[0]]
            if datasource is None:
                raise ValueError(f'Table name/alias not defined{parsedColExpr[0]}')
            else:
                return datasource.parse_col_names(parsedColExpr[1])
            
            
class Context:
    def __init__(self):
        self.sql = ''
        self.tables_byname = dict()
        self.col_byname = dict()
        self.tables = []
        self.cols = []
        self.datasource = None
        self.udf_map = dict()
        self.use_columnstore = False
        
    def emit(self, sql:str):
        self.sql += sql + ' '
        
    def add_table(self, table_name, cols):
        tbl = TableInfo(table_name, cols, self)
        self.tables.append(tbl)
        return tbl
    
    