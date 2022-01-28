from engine.ast import TableInfo, ast_node, Context, include
from engine.join import join
from engine.expr import expr
from engine.scan import filter
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
        self.projections = p if type(p) is list else [p]
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
           
        if self.datasource is not None:
            self.datasource_changed = True
            self.prev_datasource = self.context.datasource
            self.context.datasource = self.datasource            
        if 'where' in node:
            self.datasource = filter(self, node['where'], True).output
            self.context.datasource = self.datasource            


    def consume(self, _):
        disp_varname = 'd'+base62uuid(7)
        self.emit_no_ln(f'{disp_varname}:(')
        for i, proj in enumerate(self.projections):
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    if type(e) is str:
                        self.emit_no_ln(f"{self.datasource.parse_tablenames(proj['value'])}")
                    elif type(e) is dict:
                        self.emit_no_ln(f"{expr(self, e).k9expr}")
                    self.emit_no_ln(';'if i < len(self.projections)-1 else '')

        self.emit(')')
        if self.disp:
            if len(self.projections) > 1:
                self.emit(f'+{disp_varname}')
            else:
                self.emit(f'+,(,{disp_varname})')
        if self.datasource_changed:
            self.context.datasource = self.prev_datasource


import sys
include(sys.modules[__name__])