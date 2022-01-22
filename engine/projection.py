from engine.ast import TableInfo, ast_node, Context, include
from engine.join import join
from engine.expr import expr
from engine.utils import base62uuid

class projection(ast_node):
    name='select'
    def __init__(self, parent:ast_node, node, context:Context = None, outname = None, disp = True):
        self.disp = disp
        self.outname = outname
        ast_node.__init__(self, parent, node, context)
    def init(self, _):
        if self.outname is None:
            self.outname = self.context.gen_tmptable()

    def produce(self, node):
        p = node['select']
        self.projections = p if type(projection) == list else [p]
        print(node)

    def spawn(self, node):
        self.datasource = None
        if 'from' in node:
            from_clause = node['from']
            if type(from_clause) is list:
                # from joins
                join(self, from_clause)
            elif type(from_clause) is dict:
                if 'value' in from_clause:
                    value = from_clause['value']
                    if type(value) is dict:
                        if 'select' in value:
                            # from subquery
                            projection(self, from_clause, disp = False)
                        else:
                            # TODO: from func over table
                            print(f"from func over table{node}")
                    elif type(value) is str:
                        self.datasource = self.context.tables_byname[value]
                if 'assumptions' in from_clause:
                    ord = from_clause['assumptions']['ord'] == 'asc'
                    ord = '^' if ord else '|^'
                    # TODO: generate view of table by order

            elif type(from_clause) is str:
                self.datasource = self.context.tables_byname[from_clause]
            
            if self.datasource is None:
                raise ValueError('spawn error: from clause')
        if 'where' in node:
            # apply filter
            pass


    def consume(self, node):
        disp_varname = 'disptmp' + base62uuid()
        self.emit_no_ln(f'{disp_varname}:(')
        for proj in self.projections:
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    
                    if type(e) is str:
                        self.emit_no_ln(f"{self.datasource.parse_tablenames(proj['value'])};")
                    elif type(e) is dict:
                        self.emit_no_ln(f"{expr(self, e).k9expr};")

        self.emit(')')
        if self.disp:
            self.emit(disp_varname)


import sys
include(sys.modules[__name__])