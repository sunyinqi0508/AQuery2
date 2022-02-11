from engine.ast import ColRef, TableInfo, ast_node, Context, include
from engine.groupby import groupby
from engine.join import join
from engine.expr import expr
from engine.scan import filter
from engine.utils import base62uuid, enlist, base62alp
from engine.ddl import outfile
import copy

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
                            print(f'from func over table{node}')
                    elif type(value) is str:
                        self.datasource = self.context.tables_byname[value]
                if 'assumptions' in from_clause:
                    for assumption in enlist(from_clause['assumptions']):
                        ord = assumption['ord'] == 'asc'
                        attrib = assumption['attrib']
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

        if 'groupby' in node:
            self.group_node = groupby(self, node['groupby'])
            self.datasource = copy(self.datasource) # shallow copy
            self.datasource.groupinfo = self.group_node
        else:
            self.group_node = None
            
    def consume(self, node):
        disp_varname = 'd'+base62uuid(7)
        pcolrefs = []
        if type(self.group_node) is groupby:
            grp_table = self.group_node.group
            grp_refs = self.group_node.referenced
            for i, proj in enumerate(self.projections):
                self.datasource.rec = []
                cname = ''
                if type(proj) is dict:
                    if 'value' in proj:
                        e = proj['value']
                        if type(e) is str:
                            cname = self.datasource.parse_tablenames(proj['value'])
                        elif type(e) is dict:
                            cname = expr(self, e).k9expr
                            cname = ''.join([a if a in base62alp else '' for a in cname])
                pcolrefs.append(self.datasource.rec)
                self.datasource.rec = None
            keys = 'k'+base62uuid(7)
            self.emit(f'{keys}:!{grp_table}')
            fn = 'fn' + base62uuid(6)
            # self.emit
        
        self.emit_no_ln(f'{disp_varname}:(')
        flatten = False
        cols = []
        self.out_table = TableInfo('out_'+base62uuid(4), [], self.context)
        if 'outfile' in node:
            flatten = True
            
        for i, proj in enumerate(self.projections):
            cname = ''
            if type(proj) is dict:
                if 'value' in proj:
                    e = proj['value']
                    if type(e) is str:
                        cname = self.datasource.parse_tablenames(proj['value'])
                        self.emit_no_ln(f"{cname}")
                    elif type(e) is dict:
                        cname = expr(self, e).k9expr
                        self.emit_no_ln(f"{cname}")
                        cname = ''.join([a if a in base62alp else '' for a in cname])
                    self.emit_no_ln(';'if i < len(self.projections)-1 else '')
            cols.append(ColRef(f'(+{disp_varname})[{i}]', 'generic', self.out_table, 0, None, cname, i))
        self.emit(')')
        if flatten:
            self.emit_no_ln(f'{disp_varname}:' if flatten else '')
            
        if flatten or self.disp:
            if len(self.projections) > 1:
                self.emit(f"+{disp_varname}")
            else:
                self.emit(f'+,(,{disp_varname})')
            if flatten:
                self.emit(f'{disp_varname}')
        if flatten:
            self.out_table.columns = cols
            outfile(self, node['outfile'])
        if self.datasource_changed:
            self.context.datasource = self.prev_datasource


import sys
include(sys.modules[__name__])