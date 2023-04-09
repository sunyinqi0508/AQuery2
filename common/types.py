from copy import deepcopy
from typing import Dict, List

from aquery_config import have_hge
from common.utils import base62uuid, defval

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
            self.ctype_name = defval(ctype_name, f'types::A{name.upper()}') 
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
 
    def __call__(self, args):
        arg_str = ', '.join([a.__str__() for a in args])
        ret = deepcopy(self)
        ret.sqlname = self.sqlname + f'({arg_str})'
        return ret

    def __repr__(self) -> str:
        return self.sqlname
    def __str__(self) -> str:
        return self.sqlname
    
    @staticmethod
    def decode(aquery_type : str, vector_type:str = 'vector_type') -> "Types":
        if (aquery_type.lower().startswith('vec')):
            return VectorT(Types.decode(aquery_type[3:]), vector_type)
        return type_table[aquery_type.lower()]
    
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
AnyT = Types(-1)
LazyT = Types(240, name = 'Lazy', cname = '', sqlname = '', ctype_name = '')
DateT = Types(200, name = 'DATE', cname = 'types::date_t', sqlname = 'DATE', ctype_name = 'types::ADATE')
TimeT = Types(201, name = 'TIME', cname = 'types::time_t', sqlname = 'TIME', ctype_name = 'types::ATIME')
TimeStampT = Types(202, name = 'TIMESTAMP', cname = 'types::timestamp_t', sqlname = 'TIMESTAMP', ctype_name = 'ATIMESTAMP')
DoubleT = Types(17, name = 'double', cname='double', sqlname = 'DOUBLE', is_fp = True)
LDoubleT = Types(18, name = 'long double', cname='long double', sqlname = 'LDOUBLE', is_fp = True)
FloatT = Types(16, name = 'float', cname = 'float', sqlname = 'REAL', 
                long_type = DoubleT, is_fp = True)
HgeT = Types(9, name = 'int128',cname='__int128_t', sqlname = 'HUGEINT', fp_type = DoubleT)
UHgeT = Types(10, name = 'uint128', cname='__uint128_t', sqlname = 'HUGEINT', fp_type = DoubleT)
LongT = Types(4, name = 'int64', sqlname = 'BIGINT', fp_type = DoubleT)
BoolT = Types(0, name = 'bool', cname='bool', sqlname = 'BOOL', long_type=LongT, fp_type=FloatT)
ByteT = Types(1, name = 'int8', sqlname = 'TINYINT', long_type=LongT, fp_type=FloatT)
ShortT = Types(2, name = 'int16', sqlname='SMALLINT', long_type=LongT, fp_type=FloatT)
IntT = Types(3, name = 'int', cname = 'int', long_type=LongT, ctype_name = 'types::AINT32', fp_type=FloatT)
ULongT = Types(8, name = 'uint64', sqlname = 'UINT64', fp_type=DoubleT)
UIntT = Types(7, name = 'uint32', sqlname = 'UINT32', long_type=ULongT, fp_type=FloatT)
UShortT = Types(6, name = 'uint16', sqlname = 'UINT16', long_type=ULongT, fp_type=FloatT)
UByteT = Types(5, name = 'uint8', sqlname = 'UINT8', long_type=ULongT, fp_type=FloatT)
StrT = Types(200, name = 'str', cname = 'string_view', sqlname='TEXT', ctype_name = 'types::ASTR')
TextT = Types(200, name = 'text', cname = 'string_view', sqlname='TEXT', ctype_name = 'types::ASTR')
VarcharT = Types(200, name = 'varchar', cname = 'string_view', sqlname='VARCHAR', ctype_name = 'types::ASTR')
VoidT = Types(200, name = 'void', cname = 'void', sqlname='Null', ctype_name = 'types::None')

class VectorT(Types):
    def __init__(self, inner_type : Types, vector_type:str = 'vector_type'):
        self.inner_type = inner_type
        self.vector_type = vector_type
        
    @property
    def name(self) -> str:
        return f'{self.vector_type}<{self.inner_type.name}>'
    @property
    def sqlname(self) -> str:
        return 'HUGEINT' # Store vector_type into 16 bit integers
    @property
    def cname(self) -> str:
        return f'{self.vector_type}<{self.inner_type.cname}>'
    @property
    def fp_type(self) -> Types:
        return VectorT(self.inner_type.fp_type, self.vector_type)
    @property
    def long_type(self):
        return VectorT(self.inner_type.long_type, self.vector_type)


def _ty_make_dict(fn : str, *ty : Types): 
    return {eval(fn):t for t in ty}

int_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', LongT, ByteT, ShortT, IntT)
uint_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', ULongT, UByteT, UShortT, UIntT)
fp_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', FloatT, DoubleT)
temporal_types : Dict[str, Types] = _ty_make_dict('t.sqlname.lower()', DateT, TimeT, TimeStampT)
builtin_types : Dict[str, Types] = {
    'string' : StrT,
    **_ty_make_dict('t.sqlname.lower()', AnyT, TextT, VarcharT, HgeT),
    **int_types, **fp_types, **temporal_types}

def get_int128_support():
    for t in int_types.values():
        t.long_type = HgeT
    for t in uint_types.values():
        t.long_type = UHgeT
    int_types['int128'] = HgeT
    uint_types['uint128'] = UHgeT
    
def revert_int128_support():
    for t in int_types.values():
        t.long_type = LongT
    for t in uint_types.values():
        t.long_type = ULongT
    int_types.pop('int128', None)
    uint_types.pop('uint128', None)
    

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
        
    def get_return_type(self, *inputs):
        return self.return_type(*inputs)
    
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
def long_return(*_ : Types) -> Types:
    return LongT.long_type
def lfp_return(*_ : Types) -> Types:
    return LongT.fp_type
def pack_return(*args : Types) -> Types:
    if len(args) == 0:
        raise ValueError('0 arguments in pack expression')
    inner_ty = args[0]
    for a in args:
        if a != inner_ty:
            raise ValueError('pack expression with different types')
    if isinstance(inner_ty, VectorT):
        print('warning: packing vector types')
        inner_ty = inner_ty.inner_type
    return VectorT(args[0])
def as_is (t: Types) -> Types:
    return t

def fp (fx):
    return lambda *args : fx(*args).fp_type
def ext (fx):
    return lambda *args : fx(*args).long_type


# operator call behavior
def binary_op_behavior(op:OperatorBase, c_code, *xs):
    name = op.cname if c_code else op.sqlname
    return f'({f" {name} ".join(xs)})'

def unary_op_behavior(op:OperatorBase, c_code, x):
    name = op.cname if c_code else op.sqlname
    return f'({name} {x})'

def fn_behavior(op:OperatorBase, c_code, *x):
    name = op.cname if c_code else op.sqlname
    return f'{name}({", ".join([f"{xx}" for xx in x])})'

def count_behavior(op:OperatorBase, c_code, x, distinct = False):
    if not c_code:
        return f'{op.sqlname}({"distinct " if distinct else ""}{x})'
    elif distinct:
        return f'({x}).distinct_size()'
    else:
        return '{count()}'
    
def distinct_behavior(op:OperatorBase, c_code, x):
    if not c_code:
        return f'{op.sqlname}({x})'
    else:
        return f'({x}).distinct()'
    
def windowed_fn_behavor(op: OperatorBase, c_code, *x):
    if not c_code:
        return f'{op.sqlname}({", ".join([f"{xx}" for xx in x])})'
    else: 
        name = op.cname if len(x) == 1 else op.cname[:-1] + 'w'
        return f'{name}({", ".join([f"{xx}" for xx in x])})'

def pack_behavior(op: OperatorBase, c_code, *x):
    if len(x) == 0:
        raise ValueError('0 arguments in pack expression')
        
    if not c_code:
        return f'{op.sqlname}({", ".join([f"{xx}" for xx in x])})'
    else:
        return f'decltype({x[0]})::pack({len(x)}, {", ".join([f"{xx}.s()" for xx in x])})'
    
def subvec_behavior(op: OperatorBase, c_code, *x):
    if len(x) < 1 :
        raise ValueError('At least 1 parameters in subvec')
    if len(x) == 1:
        return f'{x[0]}'
    if not c_code:
        return f'{op.sqlname}({", ".join([f"{xx}" for xx in x])})'
    else:
        return f'{x[0]}.subvec({x[1]}{f", {x[2]}" if len(x) == 3 else ""})'
    
# arithmetic 
opadd = OperatorBase('add', 2, auto_extension, cname = '+', sqlname = '+', call = binary_op_behavior)
# monetdb wont extend int division to fp type
# opdiv = OperatorBase('div', 2, fp(auto_extension), cname = '/', sqlname = '/', call = binary_op_behavior)
opdiv = OperatorBase('div', 2, auto_extension, cname = '/', sqlname = '/', call = binary_op_behavior)
opmul = OperatorBase('mul', 2, auto_extension, cname = '*', sqlname = '*', call = binary_op_behavior)
opsub = OperatorBase('sub', 2, auto_extension, cname = '-', sqlname = '-', call = binary_op_behavior)
opmod = OperatorBase('mod', 2, auto_extension_int, cname = '%', sqlname = '%', call = binary_op_behavior)
opneg = OperatorBase('neg', 1, as_is, cname = '-', sqlname = '-', call = unary_op_behavior)
# logical
opand = OperatorBase('and', 2, logical, cname = '&&', sqlname = ' AND ', call = binary_op_behavior)
opor = OperatorBase('or', 2, logical, cname = '||', sqlname = ' OR ', call = binary_op_behavior)
opxor = OperatorBase('xor', 2, logical, cname = '^', sqlname = ' XOR ', call = binary_op_behavior)
opgt = OperatorBase('gt', 2, logical, cname = '>', sqlname = '>', call = binary_op_behavior)
oplt = OperatorBase('lt', 2, logical, cname = '<', sqlname = '<', call = binary_op_behavior)
opgte = OperatorBase('gte', 2, logical, cname = '>=', sqlname = '>=', call = binary_op_behavior)
oplte = OperatorBase('lte', 2, logical, cname = '<=', sqlname = '<=', call = binary_op_behavior)
opneq = OperatorBase('neq', 2, logical, cname = '!=', sqlname = '!=', call = binary_op_behavior)
opeq = OperatorBase('eq', 2, logical, cname = '==', sqlname = '=', call = binary_op_behavior)
opnot = OperatorBase('not', 1, logical, cname = '!', sqlname = 'NOT', call = unary_op_behavior)
opdistinct = OperatorBase('distinct', 1, as_is, cname = '.distinct()', sqlname = 'distinct', call = distinct_behavior)
# functional
fnprev = OperatorBase('prev', 1, as_is, cname = 'prev', sqlname = 'PREV', call = fn_behavior)
fnnext = OperatorBase('next', 1, as_is, cname = 'aggnext', sqlname = 'NEXT', call = fn_behavior)
fnmax = OperatorBase('max', 1, as_is, cname = 'max', sqlname = 'MAX', call = fn_behavior)
fnmin = OperatorBase('min', 1, as_is, cname = 'min', sqlname = 'MIN', call = fn_behavior)
fndeltas = OperatorBase('deltas', 1, as_is, cname = 'deltas', sqlname = 'DELTAS', call = fn_behavior)
fnratios = OperatorBase('ratios', [1, 2], fp(ty_clamp(as_is, -1)), cname = 'ratios', sqlname = 'RATIOS', call = windowed_fn_behavor)
fnlast = OperatorBase('last', 1, as_is, cname = 'last', sqlname = 'LAST', call = fn_behavior)
fnfirst = OperatorBase('first', 1, as_is, cname = 'frist', sqlname = 'FRIST', call = fn_behavior)
#fnsum = OperatorBase('sum', 1, ext(auto_extension), cname = 'sum', sqlname = 'SUM', call = fn_behavior)
#fnavg = OperatorBase('avg', 1, fp(ext(auto_extension)), cname = 'avg', sqlname = 'AVG', call = fn_behavior)
fnmedian = OperatorBase('median', 1, as_is, cname = 'median', sqlname = 'MEDIAN', call = fn_behavior)
fnsum = OperatorBase('sum', 1, long_return, cname = 'sum', sqlname = 'SUM', call = fn_behavior)
fnavg = OperatorBase('avg', 1, lfp_return, cname = 'avg', sqlname = 'AVG', call = fn_behavior)
fnvar = OperatorBase('var', 1, lfp_return, cname = 'var', sqlname = 'VAR', call = fn_behavior)
fnstd = OperatorBase('stddev', 1, lfp_return, cname = 'stddev', sqlname = 'STDDEV', call = fn_behavior)
fncorr = OperatorBase('corr', 1, lfp_return, cname = 'corr', sqlname = 'CORR', call = fn_behavior)
fnmaxs = OperatorBase('maxs', [1, 2], ty_clamp(as_is, -1), cname = 'maxs', sqlname = 'MAXS', call = windowed_fn_behavor)
fnmins = OperatorBase('mins', [1, 2], ty_clamp(as_is, -1), cname = 'mins', sqlname = 'MINS', call = windowed_fn_behavor)
fnsums = OperatorBase('sums', [1, 2], ext(ty_clamp(auto_extension, -1)), cname = 'sums', sqlname = 'SUMS', call = windowed_fn_behavor)
fnavgs = OperatorBase('avgs', [1, 2], fp(ext(ty_clamp(auto_extension, -1))), cname = 'avgs', sqlname = 'AVGS', call = windowed_fn_behavor)
fnvars = OperatorBase('vars', [1, 2], fp(ext(ty_clamp(auto_extension, -1))), cname = 'vars', sqlname = 'VARS', call = windowed_fn_behavor)
fnstds = OperatorBase('stddevs', [1, 2], fp(ext(ty_clamp(auto_extension, -1))), cname = 'stddevs', sqlname = 'STDDEVS', call = windowed_fn_behavor)
fncnt = OperatorBase('count', 1, int_return, cname = 'count', sqlname = 'COUNT', call = count_behavior)
fnpack = OperatorBase('pack', -1, pack_return, cname = 'pack', sqlname = 'PACK', call = pack_behavior)
fnsubvec = OperatorBase('subvec', [1, 2, 3], ty_clamp(as_is, 0, 1), cname = 'subvec', sqlname = 'SUBVEC', call = subvec_behavior)
# special
def is_null_call_behavior(op:OperatorBase, c_code : bool, x : str):
    if c_code : 
        return f'{x} == nullval<decays<decltype({x})>>'
    else :
        return f'{x} IS NULL'
spnull = OperatorBase('missing', 1, logical, cname = "", sqlname = "", call = is_null_call_behavior)

# cstdlib
# If in aggregation functions, using monetdb builtins. If in nested agg, inside udfs, using cstdlib.
fntrunc = OperatorBase('truncate', 2, ty_clamp(as_is, 0, 1), cname = 'truncate', sqlname = 'TRUNCATE', call = fn_behavior)
fnsqrt = OperatorBase('sqrt', 1, lambda *_ : DoubleT, cname = 'sqrt', sqlname = 'SQRT', call = fn_behavior)
fnlog = OperatorBase('log', 2, lambda *_ : DoubleT, cname = 'log', sqlname = 'LOG', call = fn_behavior)
fnsin = OperatorBase('sin', 1, lambda *_ : DoubleT, cname = 'sin', sqlname = 'SIN', call = fn_behavior)
fncos = OperatorBase('cos', 1, lambda *_ : DoubleT, cname = 'cos', sqlname = 'COS', call = fn_behavior)
fntan = OperatorBase('tan', 1, lambda *_ : DoubleT, cname = 'tan', sqlname = 'TAN', call = fn_behavior)
fnpow = OperatorBase('pow', 2, lambda *_ : DoubleT, cname = 'pow', sqlname = 'POW', call = fn_behavior)

# type collections
def _op_make_dict(*items : OperatorBase):
    return { i.name: i for i in items}
#binary op
builtin_binary_arith = _op_make_dict(opadd, opdiv, opmul, opsub, opmod)
builtin_binary_logical = _op_make_dict(opand, opor, opxor, opgt, oplt, 
                                       opgte, oplte, opneq, opeq)
builtin_binary_ops = {**builtin_binary_arith, **builtin_binary_logical}
#unary op
builtin_unary_logical = _op_make_dict(opnot)
builtin_unary_arith = _op_make_dict(opneg)
builtin_unary_special = _op_make_dict(spnull, opdistinct)
# functions 
builtin_cstdlib = _op_make_dict(fnsqrt, fnlog, fnsin, fncos, fntan, fnpow)
builtin_aggfunc = _op_make_dict(fnmedian, fnsubvec, fnmax, fnmin, fnsum, fnavg, 
                                fnlast, fnfirst, fncnt, fnvar, fnstd, fncorr)
builtin_vecfunc = _op_make_dict(fnmaxs, 
                             fnmins, fndeltas, fnratios, fnsums, fnavgs, 
                             fnpack, fntrunc, fnprev, fnnext, fnvars, fnstds)
builtin_vecfunc = {**builtin_vecfunc, **builtin_cstdlib}
builtin_func = {**builtin_vecfunc, **builtin_aggfunc}

user_module_func = {}

builtin_operators : Dict[str, OperatorBase] = {**builtin_binary_arith, **builtin_binary_logical, 
    **builtin_unary_arith, **builtin_unary_logical, **builtin_unary_special, **builtin_func, **builtin_cstdlib, 
    **user_module_func}

type_table = {**builtin_types, **type_table}

# Additional Aliases for type names
type_table['boolean'] = BoolT
