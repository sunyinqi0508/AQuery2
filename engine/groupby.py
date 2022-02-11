from engine.ast import ast_node
from engine.utils import base62uuid
from engine.expr import expr

class groupby(ast_node):
    name = '_groupby'
    def init(self, _):
        self.group = 'g' + base62uuid(7)
        self.datasource = self.parent.datasource
        self.datasource.rec = []
    def produce(self, node):
        if type(node) is not list:
            node = [node]
        g_contents = '('
        first_col = ''
        for i, g in enumerate(node):
            v = g['value']
            e = expr(self, v).k9expr
            # if v is compound expr, create tmp cols
            if type(v) is not str:
                tmpcol = 't' + base62uuid(7)
                self.emit(f'{tmpcol}:{e}')
                e = tmpcol
            if i == 0:
                first_col = e
            g_contents += e + (';'if i < len(node)-1 else '')
            
        self.emit(f'{self.group}:'+g_contents+')')
        self.n_grps = len(node)
        if self.n_grps <= 1:
            self.emit(f'{self.group}:={self.group}')
        else:
            self.emit(f'{self.group}:groupby[({self.group},(,!(#({first_col}))))]')
        
    def consume(self, _):
        self.referenced = self.datasource.rec
        self.datasource.rec = None
        return super().consume(_)

    def finalize(self, ret, out):
        self.groupby_function = 'fgrp'+base62uuid(4)
        grp = self.group
        if self.n_grps <= 1:
            k9fn = "{[range] start:*range;"+ ret + "}"
            self.emit(f'{out}:(({k9fn}\'{grp})[!{grp}])')
            self.parent.inv = False
        else:
            k9fn = "{[ids;grps;ll;dim;x] " + \
                    "start:$[x=ll;ll;grps[x+1][dim-1]];" + \
                    "end: grps[x][dim-1];" + \
                    "range:(end-start)#(((start-ll))#ids);" + \
                    ret + '}'
            self.emit(f'{self.groupby_function}:{k9fn}')
            self.emit(f'{out}:+({self.groupby_function}' + \
                f'[{grp}[1];{grp}[0];(#{grp}[0])-1;#({grp}[0][0])]\'!((#({grp}[0]))-1))')