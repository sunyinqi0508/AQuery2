from engine.ast import ColRef, TableInfo, ast_node, Context, include
from engine.groupby import groupby
from engine.join import join
from engine.expr import expr
from engine.orderby import orderby
from engine.scan import filter
from engine.utils import base62uuid, enlist, base62alp
from engine.ddl import create_table, outfile
import copy

class projection(ast_node):
    name='select'
    def __init__(self, parent:ast_node, node, context:Context = None, outname = None, disp = True):
        self.disp = disp
        self.outname = outname
        self.group_node = None
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
                            print(f'from func over table{node}')
                    elif type(value) is str:
                        self.datasource = self.context.tables_byname[value]
                if 'assumptions' in from_clause:
                    for assumption in enlist(from_clause['assumptions']):
                        orderby(self, assumption)

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

        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
            self.datasource = copy.copy(self.datasource) # shallow copy
            self.datasource.groupinfo = self.group_node
        else:
            self.group_node = None
            
    def consume(self, node):
        self.inv = True
        disp_varname = 'd'+base62uuid(7)
        has_groupby = False
        if self.group_node is not None:
            # There is group by;
            has_groupby = True
        cexprs = []
        flatten = False
        cols = []
        self.out_table = TableInfo('out_'+base62uuid(4), [], self.context)
        if 'outfile' in node:
            flatten = True
            
        for i, proj in enumerate(self.projections):
            cname = ''
            compound = False
            self.datasource.rec = []
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    sname = expr(self, e)._expr
                    fname = expr.toCExpr(sname)
                    absname = expr(self, e, abs_col=True)._expr
                    compound = True
                    cexprs.append(fname)
                    cname = ''.join([a if a in base62alp else '' for a in fname()])

            compound = compound and has_groupby and self.datasource.rec not in self.group_node.referenced
            
            cols.append(ColRef(cname, expr.toCExpr(f'decays<decltype({absname})>')(0), self.out_table, 0, None, cname, i, compound=compound))
        self.out_table.add_cols(cols, False)
        
        if has_groupby:
            create_table(self, self.out_table)
            self.group_node.finalize(cexprs, self.out_table)
        else:
            create_table(self, self.out_table, cexpr = cexprs)
        self.datasource.group_node = None

        has_orderby = 'orderby' in node

        if has_orderby:
            self.datasource = self.out_table
            self.context.datasource = self.out_table # discard current ds
            orderby_node = orderby(self, node['orderby'])
            self.context.datasource.materialize_orderbys()
            self.emit_no_ln(f"{f'{disp_varname}:+' if flatten else ''}(")
            
        if self.disp or has_orderby:
            self.emit(f'print(*{self.out_table.cxt_name});')
            
        if has_orderby:
            self.emit(f')[{orderby_node.view}]')
        else:
            self.context.emit_flush()
        if flatten:
            if len(self.projections) > 1 and not self.inv:
                self.emit(f"{disp_varname}:+{disp_varname}")
            outfile(self, node['outfile'])

        if self.datasource_changed:
            self.context.datasource = self.prev_datasource


import sys
include(sys.modules[__name__])