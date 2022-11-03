from typing import Optional, Set

from engine.types import *
from reconstruct.ast import ast_node
from reconstruct.storage import ColRef, Context

# TODO: Decouple expr and upgrade architecture
# C_CODE : get ccode/sql code?
# projections : C/SQL/decltype string
# orderby/joins/where : SQL only
# assumption/groupby : C/sql
# is_udfexpr: C only

class expr(ast_node):
    name='expr'
    valid_joincond = {
        0 : ('and', 'eq', 'not'),
        1 : ('or', 'neq', 'not'),
        2 : ('', '', '')
    }
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

        # gen2 expr have multi-passes
        # first pass parse json into expr tree
        # generate target code in later passes upon need
        self.children = []
        self.opname = ''
        self.curr_code = ''
        self.counts = {}
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
        self.node = node
        self.supress_undefined = supress_undefined
        if(type(parent) is expr):
            self.next_valid = parent.next_valid
            self.inside_agg = parent.inside_agg
            self.is_udfexpr = parent.is_udfexpr
            self.is_agg_func = parent.is_agg_func
            self.root : expr = parent.root
            self.c_code = parent.c_code
            self.builtin_vars = parent.builtin_vars
        else:
            self.join_conditions = []
            self.next_valid = 0
            self.is_agg_func = False
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
        
        self.udf_called = None
        self.cols_mentioned : Optional[set[ColRef]] = None
        ast_node.__init__(self, parent, node, None)

    def init(self, _):
        from reconstruct.ast import _tmp_join_union, projection
        parent = self.parent
        self.is_compound = parent.is_compound if type(parent) is expr else False
        if type(parent) in [projection, expr, _tmp_join_union]:
            self.datasource = parent.datasource
        else:
            self.datasource = self.context.datasource
        self.udf_map = parent.context.udf_map
        self.func_maps = {**builtin_func, **self.udf_map, **user_module_func}
        self.operators = {**builtin_operators, **self.udf_map, **user_module_func}
        self.ext_aggfuncs = ['sum', 'avg', 'count', 'min', 'max', 'last', 'first', 'prev', 'next']
        
    def produce(self, node):
        from engine.utils import enlist
        from reconstruct.ast import udf, projection
        
        if type(node) is dict:
            if 'literal' in node:
                node = node['literal']
            else:
                if len(node) > 1:
                    print(f'Parser Error: {node} has more than 1 dict entry.')

                is_joincond = False
                for key, val in node.items():
                    key = key.lower()
                    if key not in self.valid_joincond[self.next_valid]:
                        self.next_valid = 2
                    else:
                        if key == self.valid_joincond[self.next_valid][2]:
                            self.next_valid = not self.next_valid
                        elif key == self.valid_joincond[self.next_valid][1]:
                            self.next_valid = 2
                            is_joincond = True
                    if key in self.operators:
                        if key in builtin_func:
                            if self.is_agg_func:
                                self.root.is_special = True # Nested Aggregation
                            else:
                                self.is_agg_func = True
                        
                        op = self.operators[key]
                        count_distinct = False
                        if key == 'count' and type(val) is dict and 'distinct' in val:
                            count_distinct = True
                            val = val['distinct']
                            
                        val = enlist(val)
                        exp_vals = []
                        for v in val:
                            if (
                                    type(v) is str and  
                                    '*' in v and 
                                    key != 'count'
                                ):
                                cols = self.datasource.get_cols(v)
                                if cols:
                                    for c in cols:
                                        exp_vals.append(expr(self, c.name, c_code=self.c_code))
                            else:
                                exp_vals.append(expr(self, v, c_code=self.c_code))
                                
                        self.children = exp_vals
                        self.opname = key
                        
                        str_vals = [e.sql for e in exp_vals]
                        type_vals = [e.type for e in exp_vals]
                        is_compound = max([e.is_compound for e in exp_vals])
                        if key in self.ext_aggfuncs:
                            self.is_compound = max(0, is_compound - 1)
                        else:
                            self.is_compound = is_compound
                        try:
                            self.type = op.return_type(*type_vals)
                        except AttributeError as e:
                            if type(self.root.parent) is not udf:
                                # TODO: do something when this is not an error
                                print(f'alert: {e}')
                                pass
                            self.type = AnyT
                            
                        if count_distinct: # inject distinct col later
                            self.sql = f'{{{op(self.c_code, *str_vals, True)}}}'
                        else:
                            self.sql = op(self.c_code, *str_vals)
                            
                        special_func = [*self.context.udf_map.keys(), *self.context.module_map.keys(), 
                                        "maxs", "mins", "avgs", "sums", "deltas", "last", "first", 
                                        "stddevs", "vars", "ratios", "pack", "truncate"]
                        
                        if (
                                self.context.special_gb 
                                    or 
                                (
                                    type(self.root.parent) is projection 
                                        and
                                    self.root.parent.force_use_spgb
                                )
                           ):
                            special_func = [*special_func, *self.ext_aggfuncs]
                            
                        if key in special_func and not self.is_special:
                            self.is_special = True
                            if key in self.context.udf_map:
                                self.root.udf_called = self.context.udf_map[key]
                                if self.is_udfexpr and key == self.root.udf.name:
                                    self.root.is_recursive_call_inudf = True
                            elif key in user_module_func.keys():
                                udf.try_init_udf(self.context)                           
                            # TODO: make udf_called a set!
                            p = self.parent
                            while type(p) is expr and not p.udf_called:
                                p.udf_called = self.udf_called
                                p = p.parent
                            p = self.parent
                            while type(p) is expr and not p.is_special:
                                p.is_special = True
                                p = p.parent

                        need_decltypestr = any([e.need_decltypestr for e in exp_vals])        
                        if need_decltypestr or (self.udf_called and type(op) is udf):
                            decltypestr_vals = [e.udf_decltypecall for e in exp_vals]
                            self.udf_decltypecall = op(self.c_code, *decltypestr_vals)

                            if self.udf_called and type(op) is udf:
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
                            # print(f'Use of undefined variable {vec[0]}')
                            # TODO: do something when this is not an error
                            pass
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
                
                if (is_joincond and len(self.children) == 2
                    and all([c.is_ColExpr for c in self.children])) :
                    self.root.join_conditions.append(
                            (self.children[0].raw_col, self.children[1].raw_col)
                        )
                    
        if type(node) is str:
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
                if self.datasource is not None:
                    if (node == '*' and 
                        not (type(self.parent) is expr 
                             and 'count' in self.parent.node)):
                        self.datasource.all_cols(ordered = True)
                    else:
                        self.raw_col = self.datasource.parse_col_names(node)
                        self.raw_col = self.raw_col if type(self.raw_col) is ColRef else None
                if self.raw_col is not None:
                    self.is_ColExpr = True
                    table_name = ''
                    if '.' in node:
                        table_name = self.raw_col.table.table_name
                        if self.raw_col.table.alias:
                            alias = iter(self.raw_col.table.alias)
                            try:
                                a = next(alias)
                                while(not a or a == table_name):
                                    a = next(alias)
                                if (a and a != table_name):
                                    table_name = a
                            except StopIteration:
                                pass
                    if table_name:
                        table_name = table_name + '.'
                    self.sql = table_name + self.raw_col.name
                    self.type = self.raw_col.type
                    self.is_compound = True
                    self.is_compound += self.raw_col.compound
                    self.opname = self.raw_col
                else:
                    self.sql = '\'' + node + '\'' if node != '*' else '*'
                    self.type = StrT
                    self.opname = self.sql
                if self.c_code and self.datasource is not None:
                    if (type(self.parent) is expr and 
                        'distinct' in self.parent.node and 
                        not self.is_special):
                        # this node is executed by monetdb
                        # gb condition, not special
                        self.sql  = f'distinct({self.sql})'
                    self.sql = f'{{y(\"{self.sql}\")}}'
        elif type(node) is bool:
            self.type = BoolT
            self.opname = node
            if self.c_code:
                self.sql = '1' if node else '0'
            else:
                self.sql = 'TRUE' if node else 'FALSE'
        elif type(node) is not dict:
            self.sql = f'{node}'
            self.opname = node
            if type(node) is int:
                if (node >= 2**63 - 1 or node <= -2**63):
                    self.type = HgeT
                elif (node >= 2**31 - 1 or node <= -2**31):
                    self.type = LongT
                elif node >= 2**15 - 1 or node <= -2**15:
                    self.type = IntT
                elif node >= 2**7 - 1 or node <= -2**7:
                    self.type = ShortT
                else:
                    self.type = ByteT
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

    def codegen(self, delegate):
        self.curr_code = ''
        for c in self.children:
            self.curr_code += c.codegen(delegate)
        return self.curr_code
    
    def __str__(self):
        return self.sql
    def __repr__(self):
        return self.__str__()
    
    # builtins is readonly, so it's okay to set default value as an object
    # eval is only called at root expr.
    def eval(self, c_code = None, y = lambda t: t, 
             materialize_builtin = False, _decltypestr = False, 
             count = lambda : 'count', var_inject = None, 
             *, 
             gettype = False):
        assert(self.is_root)
        def call(decltypestr = False) -> str:
            nonlocal c_code, y, materialize_builtin, count, var_inject
            if var_inject:
                for k, v in var_inject.items():
                    locals()[k] = v
            if self.udf_called is not None:
                loc = locals()
                builtin_vars = self.udf_called.builtin_used
                for b in self.udf_called.builtin_var.all:
                        exec(f'loc["{b}"] = lambda: "{{{b}()}}"')
                if builtin_vars:
                    if type(materialize_builtin) is dict:
                        for b in builtin_vars:
                            exec(f'loc["{b}"] = lambda: "{materialize_builtin[b]}"')
                    elif self.is_recursive_call_inudf:
                        for b in builtin_vars:
                            exec(f'loc["{b}"] = lambda : "{b}"')
                
            x = self.c_code if c_code is None else c_code
            from engine.utils import escape_qoutes
            if decltypestr:
                return eval('f\'' + escape_qoutes(self.udf_decltypecall) + '\'')
            self.sql.replace("'", "\\'")
            return eval('f\'' + escape_qoutes(self.sql) + '\'')
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
