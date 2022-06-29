from engine.utils import defval
from typing import Dict, List

type_table: Dict[str, "Types"] = {}

class Types:
    def init_any(self):
        self.name : str = 'Any'
        self.sqlname : str = 'Int'
        self.cname : str = 'void*'
        self.ctype_name : str = "types::NONE"
        self.null_value = 0
        self.priority : int= 0
        self.cast_to_dict = dict()
        self.cast_from_dict = dict()
    def __init__(self, priority = 0, *, 
                 name = None, cname = None, sqlname = None,
                 ctype_name = None, null_value = None,
                 fp_type = None, long_type = None, is_fp = False, 
                 cast_to = None, cast_from = None
                 ):
        
        self.is_fp = is_fp
        if name is None:
            self.init_any()
        else:
            self.name = name
            self.cname = defval(cname, name.lower() + '_t')
            self.sqlname = defval(sqlname, name.upper())
            self.ctype_name = defval(ctype_name, f'types::{name.upper()}') 
            self.null_value = defval(null_value, 0)
            self.cast_to_dict = defval(cast_to, dict())
            self.cast_from_dict = defval(cast_from, dict())
            self.priority = priority

        self.long_type = defval(long_type, self)
        self.fp_type = defval(fp_type, self)

        global type_table
        type_table[name] = self
        
    def cast_to(self, ty : "Types"):
        if ty in self.cast_to_dict:
            return self.cast_to_dict[ty.name](ty)
        else:
            raise Exception(f'Illeagal cast: from {self.name} to {ty.name}.')
    def cast_from(self, ty : "Types"):
        if ty in self.cast_from_dict:
            return self.cast_from_dict[ty.name](ty)
        else:
            raise Exception(f'Illeagal cast: from {ty.name} to {self.name}.')

    def __repr__(self) -> str:
        return self.sqlname
    def __str__(self) -> str:
        return self.sqlname
class TypeCollection:
    def __init__(self, sz, deftype, fptype = None, utype = None, *, collection = None) -> None:
        self.size = sz
        self.type = deftype
        self.fptype = fptype
        self.utype = utype
        self.all_types = [deftype]
        if fptype is not None:
            self.all_types.append(fptype)
        if utype is not None:
            self.all_types.append(utype)
        if collection is not None:
            for ty in collection:
                self.all_types.append(ty)
                
type_table = dict()
AnyT = Types(0)
LazyT = Types(240, name = 'Lazy', cname = '', sqlname = '', ctype_name = '')
DoubleT = Types(17, name = 'double', cname='double', sqlname = 'DOUBLE', is_fp = True)
FloatT = Types(16, name = 'float', cname = 'float', sqlname = 'REAL', 
                long_type = DoubleT, is_fp = True)
LongT = Types(4, name = 'int64', sqlname = 'BIGINT', fp_type = DoubleT)
ByteT = Types(1, name = 'int8', sqlname = 'TINYINT', long_type=LongT, fp_type=FloatT)
ShortT = Types(2, name = 'int16', sqlname='SMALLINT', long_type=LongT, fp_type=FloatT)
IntT = Types(3, name = 'int', cname = 'int', long_type=LongT, fp_type=FloatT)
StrT = Types(200, name = 'str', cname = 'const char*', sqlname='VARCHAR', ctype_name = 'types::STRING')
def _ty_make_dict(fn : str, *ty : Types): 
    return {eval(fn):t for t in ty}

int_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', LongT, ByteT, ShortT, IntT)
fp_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', FloatT, DoubleT)
builtin_types : Dict[str, Types] = {**_ty_make_dict('t.sqlname.lower()', AnyT, StrT), **int_types, **fp_types}

type_bylength : Dict[int, TypeCollection] = {}
type_bylength[1] = TypeCollection(1, ByteT)
type_bylength[2] = TypeCollection(2, ShortT)
type_bylength[4] = TypeCollection(4, IntT, FloatT)
type_bylength[8] = TypeCollection(8, LongT, DoubleT, collection=[AnyT])

class OperatorBase:
    def extending_type(ops:Types):
        return ops.long_type
    def fraction_type (ops:Types):
        return ops.fp_type
    def __init__(self, opname, n_ops, return_fx, * , 
                 optypes = None, cname = None, sqlname = None,
                 call = None):
        self.name = opname
        self.cname = defval(cname, opname)
        self.sqlname = defval(sqlname, opname.upper())
        self.n_ops = n_ops
        self.optypes = optypes
        self.return_type = defval(return_fx, lambda: self.optypes[0])
        self.call = defval(call, lambda _, c_code = False, *args: 
                               f'{self.cname if c_code else self.sqlname}({", ". join(args)})') 
    def __call__(self, c_code = False, *args) -> str:
        return self.call(self, c_code, *args)
        
    def get_return_type(self, inputs):
        return self.return_type(inputs)
    
    def __repr__(self) -> str:
        return self.name
    def __str__(self) -> str:
        return self.name
    
    # TODO: Type checks, Type catagories, e.g.: value type, etc.


# return type deduction
def auto_extension(*args : Types) -> Types:
    final_type = AnyT
    is_fp = False
    for a in args:
        if not is_fp and a.is_fp:
            is_fp = True
            final_type = final_type.fp_type
        elif is_fp:
            a = a.fp_type
        final_type = a if a.priority > final_type.priority else final_type
    return final_type

def auto_extension_int(*args : Types) -> Types:
    final_type = AnyT
    for a in args:
        final_type = a if a.priority > final_type.priority else final_type
    return final_type
def ty_clamp(fn, l:int = None, r:int = None):
    return lambda *args : fn(*args[l: r])
def logical(*_ : Types) -> Types:
    return ByteT
def int_return(*_ : Types) -> Types:
    return IntT
def as_is (t: Types) -> Types:
    return t

def fp (fx):
    return lambda *args : fx(*args).fp_type
def ext (fx):
    return lambda *args : fx(*args).long_type


# operator call behavior
def binary_op_behavior(op:OperatorBase, c_code, x, y):
    name = op.cname if c_code else op.sqlname
    return f'({x} {name} {y})'

def unary_op_behavior(op:OperatorBase, c_code, x):
    name = op.cname if c_code else op.sqlname
    return f'({x} {name})'

def fn_behavior(op:OperatorBase, c_code, *x):
    name = op.cname if c_code else op.sqlname
    return f'{name}({", ".join([f"{xx}" for xx in x])})'

def windowed_fn_behavor(op: OperatorBase, c_code, *x):
    if not c_code:
        return f'{op.sqlname}({", ".join([f"{xx}" for xx in x])})'
    else: 
        name = op.cname if len(x) == 1 else op.cname[:-1] + 'w'
        return f'{name}({", ".join([f"{xx}" for xx in x])})'
    
# arithmetic 
opadd = OperatorBase('add', 2, auto_extension, cname = '+', sqlname = '+', call = binary_op_behavior)
opdiv = OperatorBase('div', 2, fp(auto_extension), cname = '/', sqlname = '/', call = binary_op_behavior)
opmul = OperatorBase('mul', 2, fp(auto_extension), cname = '*', sqlname = '*', call = binary_op_behavior)
opsub = OperatorBase('sub', 2, auto_extension, cname = '-', sqlname = '-', call = binary_op_behavior)
opmod = OperatorBase('mod', 2, auto_extension_int, cname = '%', sqlname = '%', call = binary_op_behavior)
opneg = OperatorBase('neg', 1, as_is, cname = '-', sqlname = '-', call = unary_op_behavior)
# logical
opand = OperatorBase('and', 2, logical, cname = '&&', sqlname = ' AND ', call = binary_op_behavior)
opor = OperatorBase('or', 2, logical, cname = '||', sqlname = ' OR ', call = binary_op_behavior)
opxor = OperatorBase('xor', 2, logical, cname = '^', sqlname = ' XOR ', call = binary_op_behavior)
opgt = OperatorBase('gt', 2, logical, cname = '>', sqlname = '>', call = binary_op_behavior)
oplt = OperatorBase('lt', 2, logical, cname = '<', sqlname = '<', call = binary_op_behavior)
opge = OperatorBase('gte', 2, logical, cname = '>=', sqlname = '>=', call = binary_op_behavior)
oplte = OperatorBase('lte', 2, logical, cname = '<=', sqlname = '<=', call = binary_op_behavior)
opneq = OperatorBase('neq', 2, logical, cname = '!=', sqlname = '!=', call = binary_op_behavior)
opeq = OperatorBase('eq', 2, logical, cname = '==', sqlname = '=', call = binary_op_behavior)
opnot = OperatorBase('not', 1, logical, cname = '!', sqlname = 'NOT', call = unary_op_behavior)
# functional
fnmax = OperatorBase('max', 1, as_is, cname = 'max', sqlname = 'MAX', call = fn_behavior)
fnmin = OperatorBase('min', 1, as_is, cname = 'min', sqlname = 'MIN', call = fn_behavior)
fnsum = OperatorBase('sum', 1, ext(auto_extension), cname = 'sum', sqlname = 'SUM', call = fn_behavior)
fnavg = OperatorBase('avg', 1, fp(ext(auto_extension)), cname = 'avg', sqlname = 'AVG', call = fn_behavior)
fnmaxs = OperatorBase('maxs', [1, 2], ty_clamp(as_is, -1), cname = 'maxs', sqlname = 'MAXS', call = windowed_fn_behavor)
fnmins = OperatorBase('mins', [1, 2], ty_clamp(as_is, -1), cname = 'mins', sqlname = 'MINS', call = windowed_fn_behavor)
fnsums = OperatorBase('sums', [1, 2], ext(ty_clamp(auto_extension, -1)), cname = 'sums', sqlname = 'SUMS', call = windowed_fn_behavor)
fnavgs = OperatorBase('avgs', [1, 2], fp(ext(ty_clamp(auto_extension, -1))), cname = 'avgs', sqlname = 'AVGS', call = windowed_fn_behavor)
fncnt = OperatorBase('count', 1, int_return, cname = 'count', sqlname = 'COUNT', call = fn_behavior)
# special
def is_null_call_behavior(op:OperatorBase, c_code : bool, x : str):
    if c_code : 
        return f'{x} == nullval<decays<decltype({x})>>'
    else :
        return f'{x} IS NULL'
spnull = OperatorBase('missing', 1, logical, cname = "", sqlname = "", call = is_null_call_behavior)

# cstdlib
fnsqrt = OperatorBase('sqrt', 1, lambda *_ : DoubleT, cname = 'sqrt', sqlname = 'SQRT', call = fn_behavior)

# type collections
def _op_make_dict(*items : OperatorBase):
    return { i.name: i for i in items}
builtin_binary_arith = _op_make_dict(opadd, opdiv, opmul, opsub, opmod)
builtin_binary_logical = _op_make_dict(opand, opor, opxor, opgt, oplt, opge, oplte, opneq, opeq)
builtin_unary_logical = _op_make_dict(opnot)
builtin_unary_arith = _op_make_dict(opneg)
builtin_unary_special = _op_make_dict(spnull)
builtin_cstdlib = _op_make_dict(fnsqrt)
builtin_func = _op_make_dict(fnmax, fnmin, fnsum, fnavg, fnmaxs, fnmins, fnsums, fnavgs, fncnt)
builtin_operators : dict[str, OperatorBase] = {**builtin_binary_arith, **builtin_binary_logical, 
    **builtin_unary_arith, **builtin_unary_logical, **builtin_unary_special, **builtin_func, **builtin_cstdlib}
    