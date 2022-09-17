from engine.ast import ColRef, TableInfo, ast_node
from engine.orderby import assumption
from engine.scan import scan
from engine.utils import base62uuid
from engine.expr import expr

class groupby(ast_node):
    name = '_groupby'
    def init(self, _):
        self.context.headers.add('"./server/hasher.h"')
        self.context.headers.add('unordered_map')
        self.group = 'g' + base62uuid(7)
        self.group_type = 'record_type' + base62uuid(7)
        self.datasource = self.parent.datasource
        self.scanner = None
        self.datasource.rec = set()
        self.raw_groups = []
    def produce(self, node):
        
        if type(node) is not list:
            node = [node]
        g_contents = ''
        g_contents_list = []
        first_col = ''
        for i, g in enumerate(node):
            v = g['value']
            e = expr(self, v)
            if type(e.raw_col) is ColRef:
                self.raw_groups.append(e.raw_col)
            e = e._expr
            # if v is compound expr, create tmp cols
            if type(v) is not str:
                tmpcol = 't' + base62uuid(7)
                self.emit(f'auto {tmpcol} = {e};')
                e = tmpcol
            if i == 0:
                first_col = e
            g_contents_list.append(e)
        g_contents_decltype = [f'decltype({c})' for c in g_contents_list]
        g_contents = expr.toCExpr(','.join(g_contents_list))
        self.emit(f'typedef record<{expr.toCExpr(",".join(g_contents_decltype))(0)}> {self.group_type};')
        self.emit(f'unordered_map<{self.group_type}, vector_type<uint32_t>, '
            f'transTypes<{self.group_type}, hasher>> {self.group};')
        self.n_grps = len(node)
        self.scanner = scan(self, self.datasource, expr.toCExpr(first_col)()+'.size')
        self.scanner.add(f'{self.group}[forward_as_tuple({g_contents(self.scanner.it_ver)})].emplace_back({self.scanner.it_ver});')

        
    def consume(self, _):
        self.referenced = self.datasource.rec
        self.datasource.rec = None
        self.scanner.finalize()
        
    def deal_with_assumptions(self, assumption:assumption, out:TableInfo):
        gscanner = scan(self, self.group)
        val_var = 'val_'+base62uuid(7)
        gscanner.add(f'auto &{val_var} = {gscanner.it_ver}.second;')
        gscanner.add(f'{self.datasource.cxt_name}->order_by<{assumption.result()}>(&{val_var});')
        gscanner.finalize()
        
    def finalize(self, cexprs, out:TableInfo):
        gscanner = scan(self, self.group)
        key_var = 'key_'+base62uuid(7)
        val_var = 'val_'+base62uuid(7)
        
        gscanner.add(f'auto &{key_var} = {gscanner.it_ver}.first;')
        gscanner.add(f'auto &{val_var} = {gscanner.it_ver}.second;')
        gscanner.add(';\n'.join([f'{out.columns[i].reference()}.emplace_back({ce(x=val_var, y=key_var)})' for i, ce in enumerate(cexprs)])+';')
        
        gscanner.finalize()
        
        self.datasource.groupinfo = None