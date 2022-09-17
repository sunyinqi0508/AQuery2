from engine.ast import ColRef, TableInfo, ast_node, Context, include
from engine.groupby import groupby
from engine.join import join
from engine.expr import expr
from engine.orderby import assumption, orderby
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
        self.assumptions = None
        self.where = None
        ast_node.__init__(self, parent, node, context)
    def init(self, _):
        if self.outname is None:
            self.outname = self.context.gen_tmptable()

    def produce(self, node):
        p = node['select']
        self.projections = p if type(p) is list else [p]
        self.context.Info(node)

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
                    if 'name' in value:
                        self.datasource.add_alias(value['name'])
                if 'assumptions' in from_clause:
                    self.assumptions = enlist(from_clause['assumptions'])
                    
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
            # self.context.datasource = self.datasource            

        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
            self.datasource = copy.copy(self.datasource) # shallow copy
            self.datasource.groupinfo = self.group_node
        else:
            self.group_node = None
            
    def consume(self, node):
        self.inv = True
        disp_varname = 'd'+base62uuid(7)
        has_groupby = self.group_node is not None
        cexprs = []
        flatten = False
        cols = []
        self.out_table = TableInfo('out_'+base62uuid(4), [], self.context)
        if 'outfile' in node:
            flatten = True
        
        new_names = []
        proj_raw_cols = []
        for i, proj in enumerate(self.projections):
            cname = ''
            compound = False
            self.datasource.rec = set()
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    sname = expr(self, e)
                    if type(sname.raw_col) is ColRef:
                        proj_raw_cols.append(sname.raw_col)
                    sname = sname._expr
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
        
        lineage = None
        
        if has_groupby:
            create_table(self, self.out_table) # creates empty out_table.
            if self.assumptions is not None:
                self.assumptions = assumption(self, self.assumptions, exclude=self.group_node.raw_groups)
                if not self.assumptions.empty():
                    self.group_node.deal_with_assumptions(self.assumptions, self.out_table)
                self.assumptions = None
            self.group_node.finalize(cexprs, self.out_table)
        else:
            # if all assumptions in projections, treat as orderby
            lineage = self.assumptions is not None and has_other(self.assumptions, proj_raw_cols) 
            spawn = create_table(self, self.out_table, cexprs = cexprs, lineage = lineage) # create and populate out_table.
            if lineage and type(spawn.lineage) is str:
                lineage = spawn.lineage
                self.assumptions = orderby(self, self.assumptions) # do not exclude proj_raw_cols
            else:
                lineage = None
        if self.where is not None:
            self.where.finalize()
        
        if type(lineage) is str:
            order = 'order_' + base62uuid(6)
            self.emit(f'auto {order} = {self.datasource.cxt_name}->order_by<{self.assumptions.result()}>({lineage});')
            self.emit(f'{self.out_table.cxt_name}->materialize(*{order});')
            self.assumptions = None
        
        if self.assumptions is not None:
            orderby_node = orderby(self, self.assumptions)
        else:
            orderby_node = None
            
        if 'orderby' in node:
            self.datasource = self.out_table
            self.context.datasource = self.out_table # discard current ds
            orderbys = node['orderby']
            orderby_node = orderby(self, orderbys) if orderby_node is None else orderby_node.merge(orderbys)
        
        if orderby_node is not None:
            self.emit(f'auto {disp_varname} = {self.out_table.reference()}->order_by_view<{orderby_node.result()}>();')
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