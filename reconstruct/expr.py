from typing import Optional
from reconstruct.ast import ast_node
from reconstruct.storage import ColRef, Context
from engine.types import *

# TODO: Decouple expr and upgrade architecture
# C_CODE : get ccode/sql code?
# projections : C/SQL/decltype string
# orderby/joins/where : SQL only
# assumption/groupby : C/sql
# is_udfexpr: C only

class expr(ast_node):
    name='expr'
    @property
    def udf_decltypecall(self):
        return self._udf_decltypecall if self._udf_decltypecall else self.sql
    
    @udf_decltypecall.setter
    def udf_decltypecall(self, val):
        self._udf_decltypecall = val
    
    @property
    def need_decltypestr(self):
        return self._udf_decltypecall is not None
    
    def __init__(self, parent, node, *, c_code = None, supress_undefined = False):
        from reconstruct.ast import projection, udf
        
        self.type = None
        self.raw_col = None
        self.udf : Optional[udf] = None
        self.inside_agg = False
        self.is_special = False
        self.is_ColExpr = False
        self.is_recursive_call_inudf = False
        self.codlets : list = []
        self.codebuf : Optional[str] = None
        self._udf_decltypecall = None
        
        self.supress_undefined = supress_undefined
        if(type(parent) is expr):
            self.inside_agg = parent.inside_agg
            self.is_udfexpr = parent.is_udfexpr
            self.root : expr = parent.root
            self.c_code = parent.c_code
            self.builtin_vars = parent.builtin_vars
        else:
            self.is_udfexpr = type(parent) is udf
            self.root : expr = self
            self.c_code = self.is_udfexpr or type(parent) is projection
            if self.is_udfexpr:
                self.udf : udf = parent
                self.builtin_vars = self.udf.builtin.keys()
            else:
                self.builtin_vars = []
        if type(c_code) is bool:
            self.c_code = c_code
        
        ast_node.__init__(self, parent, node, None)

    def init(self, _):
        from reconstruct.ast import projection
        parent = self.parent
        self.isvector = parent.isvector if type(parent) is expr else False
        self.is_compound = parent.is_compound if type(parent) is expr else False
        if type(parent) in [projection, expr]:
            self.datasource = parent.datasource
        else:
            self.datasource = self.context.datasource
        self.udf_map = parent.context.udf_map
        self.func_maps = {**builtin_func, **self.udf_map}
        self.operators = {**builtin_operators, **self.udf_map}
        
    def produce(self, node):
        from engine.utils import enlist
        from reconstruct.ast import udf
        
        if type(node) is dict:
            for key, val in node.items():
                if key in self.operators:
                    op = self.operators[key]

                    val = enlist(val)
                    exp_vals = [expr(self, v, c_code = self.c_code) for v in val]
                    str_vals = [e.sql for e in exp_vals]
                    type_vals = [e.type for e in exp_vals]
                    try:
                        self.type = op.return_type(*type_vals)
                    except AttributeError as e:
                        if type(self.root) is not udf:
                            print(f'alert: {e}')
                        self.type = AnyT
                        
                    self.sql = op(self.c_code, *str_vals)
                    special_func = [*self.context.udf_map.keys(), "maxs", "mins", "avgs", "sums"]
                    if key in special_func and not self.is_special:
                        self.is_special = True
                        if key in self.context.udf_map:
                            self.root.udf = self.context.udf_map[key]
                            if key == self.root.udf.name:
                                self.root.is_recursive_call_inudf = True
                        p = self.parent
                        while type(p) is expr and not p.is_special:
                            p.is_special = True
                            p = p.parent
                    
                    need_decltypestr = any([e.need_decltypestr for e in exp_vals])        
                    if need_decltypestr or (self.udf and type(op) is udf):
                        decltypestr_vals = [e.udf_decltypecall for e in exp_vals]
                        self.udf_decltypecall = op(self.c_code, *decltypestr_vals)

                        if self.udf and type(op) is udf:
                            self.udf_decltypecall = op.decltypecall(self.c_code, *decltypestr_vals)
            
                elif self.is_udfexpr:
                    var_table = self.root.udf.var_table
                    vec = key.split('.')
                    _vars = [*var_table, *self.builtin_vars]
                    def get_vname (node):
                        if node in self.builtin_vars:
                            self.root.udf.builtin[node].enabled = True
                            self.builtin_var = node
                            return node
                        else:
                            return var_table[node]
                    if vec[0] not in _vars:
                        print(f'Use of undefined variable {vec[0]}')
                    else:
                        vname = get_vname(vec[0])
                        val = enlist(val)
                        if(len(val) > 2):
                            print('Warning: more than 2 indexes found for subvec operator.')
                        ex = [expr(self, v, c_code = self.c_code) for v in val]
                        idxs = ', '.join([e.sql for e in ex])
                        self.sql = f'{vname}.subvec({idxs})'
                        if any([e.need_decltypestr for e in ex]):
                            self.udf_decltypecall = f'{vname}.subvec({[", ".join([e.udf_decltypecall for e in ex])]})'
                    if key == 'get' and len(val) > 1:
                        ex_vname = expr(self, val[0], c_code=self.c_code)
                        self.sql = f'{ex_vname.sql}[{expr(self, val[1], c_code=self.c_code).sql}]'
                        if hasattr(ex_vname, 'builtin_var'):
                            if not hasattr(self, 'builtin_var'):
                                self.builtin_var = []
                            self.builtin_var = [*self.builtin_var, *ex_vname.builtin_var]
                            self.udf_decltypecall = ex_vname.sql
                else:
                    print(f'Undefined expr: {key}{val}')

        elif type(node) is str:
            if self.is_udfexpr:
                curr_udf : udf = self.root.udf
                var_table = curr_udf.var_table
                split = node.split('.')
                if split[0] in var_table:
                    varname = var_table[split[0]]
                    if curr_udf.agg and varname in curr_udf.vecs:
                        if len(split) > 1:
                            if split[1] == 'vec':
                                self.sql += varname
                            elif split[1] == 'len':
                                self.sql += f'{varname}.size'
                            else:
                                print(f'no member {split[1]} in object {varname}')
                        else:
                            self.sql += f'{varname}[{curr_udf.idx_var}]'
                    else:
                        self.sql += varname
                elif self.supress_undefined or split[0] in self.builtin_vars:
                    self.sql += node
                    if split[0] in self.builtin_vars:
                        curr_udf.builtin[split[0]].enabled = True
                        self.builtin_var = split[0]
                else:
                    print(f'Undefined varname: {split[0]}')
                
    
            # get the column from the datasource in SQL context
            else:
                p = self.parent
                while type(p) is expr and not p.isvector:
                    p.isvector = True
                    p = p.parent
                if self.datasource is not None:
                    self.raw_col = self.datasource.parse_col_names(node)
                    self.raw_col = self.raw_col if type(self.raw_col) is ColRef else None
                if self.raw_col is not None:
                    self.is_ColExpr = True
                    self.sql = self.raw_col.name
                    self.type = self.raw_col.type
                else:
                    self.sql = node
                    self.type = StrT
                if self.c_code and self.datasource is not None:
                    self.sql = f'{{y(\"{self.sql}\")}}'
        elif type(node) is bool:
            self.type = IntT
            if self.c_code:
                self.sql = '1' if node else '0'
            else:
                self.sql = 'TRUE' if node else 'FALSE'
        else:
            self.sql = f'{node}'
            if type(node) is int:
                self.type = LongT
            elif type(node) is float:
                self.type = DoubleT

    def finalize(self, override = False):
        from reconstruct.ast import udf
        if self.codebuf is None or override:
            self.codebuf = ''
            for c in self.codlets:
                if type(c) is str:
                    self.codebuf += c
                elif type(c) is udf:
                    self.codebuf += c()
                elif type(c) is expr:
                    self.codebuf += c.finalize(override=override)
        return self.codebuf

    def __str__(self):
        return self.sql
    def __repr__(self):
        return self.__str__()
    
    # builtins is readonly, so it's okay to set default value as an object
    # eval is only called at root expr.
    def eval(self, c_code = None, y = lambda t: t, materialize_builtin = False, _decltypestr = False, *, gettype = False):
        assert(self.is_root)
        def call(decltypestr = False) -> str:
            nonlocal c_code, y, materialize_builtin
            if self.udf is not None:
                loc = locals()
                builtin_vars = self.udf.builtin_used
                for b in self.udf.builtin_var.all:
                        exec(f'loc["{b}"] = lambda: "{{{b}()}}"')
                if builtin_vars:
                    if type(materialize_builtin) is dict:
                        for b in builtin_vars:
                            exec(f'loc["{b}"] = lambda: "{materialize_builtin[b]}"')
                    elif self.is_recursive_call_inudf:
                        for b in builtin_vars:
                            exec(f'loc["{b}"] = lambda : "{b}"')
                    
            x = self.c_code if c_code is None else c_code
            if decltypestr:
                return eval('f\'' + self.udf_decltypecall + '\'')
            return eval('f\'' + self.sql + '\'')
        if self.is_recursive_call_inudf or (self.need_decltypestr and self.is_udfexpr) or gettype:
            return call
        else:
            return call(_decltypestr)
        
    @property
    def is_root(self):
        return self.root == self


# For UDFs: first check if agg variable is used as vector 
# if not, then check if its length is used
class fastscan(expr):
    name = 'fastscan'
    
    def init(self, _):
        self.vec_vars = set()
        self.requested_lens = set()
        super().init(self, _)
        
    def process(self, key : str):
        segs = key.split('.')
        var_table = self.root.udf.var_table
        if segs[0] in var_table and len(segs) > 1:
            if segs[1] == 'vec':
                self.vec_vars.add(segs[0])
            elif segs[1] == 'len':
                self.requested_lens.add(segs[0])
        
    def produce(self, node):
        from engine.utils import enlist
        if type(node) is dict:
            for key, val in node.items():
                if key in self.operators:
                    val = enlist(val)
                elif self.is_udfexpr:
                    self.process(key)
                [fastscan(self, v, c_code = self.c_code) for v in val]

        elif type(node) is str:
            self.process(node)


class getrefs(expr):
    name = 'getrefs'
    
    def init(self, _):
        self.datasource.rec = set()
        self.rec = None
        
    def produce(self, node):
        from engine.utils import enlist
        if type(node) is dict:
            for key, val in node.items():
                if key in self.operators:
                    val = enlist(val)
                [getrefs(self, v, c_code = self.c_code) for v in val]

        elif type(node) is str:
             self.datasource.parse_col_names(node)
    
    def consume(self, _):
        if self.root == self:
            self.rec = self.datasource.rec
            self.datasource.rec = None