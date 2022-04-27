from engine.ast import ColRef, TableInfo, ast_node, Context, include
from engine.groupby import groupby
from engine.join import join
from engine.expr import expr
from engine.orderby import orderby
from engine.scan import filter
from engine.utils import base62uuid, enlist, base62alp, has_other
from engine.ddl import create_table, outfile
import copy

class projection(ast_node):
    name='select'
    def __init__(self, parent:ast_node, node, context:Context = None, outname = None, disp = True):
        self.disp = disp
        self.outname = outname
        self.group_node = None
        self.assumption = None
        self.where = None
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
                    self.assumption = enlist(from_clause['assumptions'])
                    
            elif type(from_clause) is str:
                self.datasource = self.context.tables_byname[from_clause]
            
            if self.datasource is None:
                raise ValueError('spawn error: from clause')
           
        if self.datasource is not None:
            self.datasource_changed = True
            self.prev_datasource = self.context.datasource
            self.context.datasource = self.datasource            
        if 'where' in node:
            self.where = filter(self, node['where'], True)
            # self.datasource = filter(self, node['where'], True).output
            #self.context.datasource = self.datasource            

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
        
        new_names = []
        for i, proj in enumerate(self.projections):
            cname = ''
            compound = False
            self.datasource.rec = set()
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    sname = expr(self, e)._expr
                    fname = expr.toCExpr(sname) # fastest access method at innermost context
                    absname = expr(self, e, abs_col=True)._expr # absolute name at function scope
                    # TODO: Make it single pass here.
                    compound = True # compound column
                    cexprs.append(fname)
                    cname = e if type(e) is str else ''.join([a if a in base62alp else '' for a in expr.toCExpr(absname)()])
                if 'name' in proj: # renaming column by AS keyword
                    cname = proj['name']
                    new_names.append(cname)
            elif type(proj) is str:
                col = self.datasource.get_col_d(proj)
                if type(col) is ColRef:
                    col.reference()
                    
            compound = compound and has_groupby and has_other(self.datasource.rec, self.group_node.referenced)
            self.datasource.rec = None
            
            typename = f'decays<decltype({absname})>'
            if not compound:
                typename = f'value_type<{typename}>'
                
            cols.append(ColRef(cname, expr.toCExpr(typename)(), self.out_table, 0, None, cname, i, compound=compound))
            
        self.out_table.add_cols(cols, False)
        
        if has_groupby:
            create_table(self, self.out_table) # creates empty out_table.
            self.group_node.finalize(cexprs, self.out_table)
        else:
            create_table(self, self.out_table, cexprs = cexprs) # create and populate out_table.
            
            
        self.datasource.group_node = None
        
        if self.where is not None:
            self.where.finalize()
        
        has_orderby = 'orderby' in node
        if has_orderby:
            self.datasource = self.out_table
            self.context.datasource = self.out_table # discard current ds
            orderby_node = orderby(self, node['orderby'])
            self.emit(f'auto {disp_varname} = {self.out_table.reference()}->order_by_view<{",".join([f"{c}" for c in orderby_node.col_list])}>();')
        else:
            disp_varname = f'*{self.out_table.cxt_name}'
        if self.disp:
            self.emit(f'print({disp_varname});')
            

        if flatten:
            outfile(self, node['outfile'])

        if self.datasource_changed:
            self.context.datasource = self.prev_datasource


import sys
include(sys.modules[__name__])