import uuid
from collections import OrderedDict
from collections.abc import Mapping, MutableMapping

lower_alp = 'abcdefghijklmnopqrstuvwxyz'
upper_alp = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
nums = '0123456789'
base62alp = nums + lower_alp + upper_alp

reserved_monet = ['month']
session_context = None

class CaseInsensitiveDict(MutableMapping):
    def __init__(self, data=None, **kwargs):
        self._store = OrderedDict()
        if data is None:
            data = {}
        self.update(data, **kwargs)

    def __setitem__(self, key, value):
        # Use the lowercased key for lookups, but store the actual
        # key alongside the value.
        self._store[key.lower()] = (key, value)

    def __getitem__(self, key):
        return self._store[key.lower()][1]

    def __delitem__(self, key):
        del self._store[key.lower()]

    def __iter__(self):
        return (casedkey for casedkey, mappedvalue in self._store.values())

    def __len__(self):
        return len(self._store)

    def lower_items(self):
        """Like iteritems(), but with all lowercase keys."""
        return ((lowerkey, keyval[1]) for (lowerkey, keyval) in self._store.items())

    def __eq__(self, other):
        if isinstance(other, Mapping):
            other = CaseInsensitiveDict(other)
        else:
            return NotImplemented
        # Compare insensitively
        return dict(self.lower_items()) == dict(other.lower_items())

    # Copy is required
    def copy(self):
        return CaseInsensitiveDict(self._store.values())

    def __repr__(self):
        return str(dict(self.items()))

def base62uuid(crop=8):
    _id = uuid.uuid4().int
    ret = ''
    
    while _id:
        ret = base62alp[_id % 62] + ret
        _id //= 62
    
    return ret[:crop] if len(ret) else '0'

def get_legal_name(name, lower = True):
    if name is not None:
        if lower:
            name = name.lower()
        name = ''.join([n for n in name if n in base62alp or n == '_'])
        
    if name is None or len(name) == 0 or set(name) == set('_'):
        name = base62uuid(8)
    if(name[0] in nums):
        name = '_' + name
        
    return name

def check_legal_name(name):
    all_underscores = True
    for c in name:
        if c not in base62alp and c != '_':
            return False
        if c != '_':
            all_underscores = False
    if all_underscores:
        return False
    if name[0] in nums:
        return False
    
    return True

def enlist(l): 
    return l if type(l) is list else [l]

def seps(s, i, l):
    return s if i < len(l) - 1 else ''

def has_other(a, b):
    for ai in a:
        if ai not in b:
            return True
    return False

def defval(val, default):
    return default if val is None else val

# escape must be readonly
from typing import Mapping, Set


def remove_last(pattern : str, string : str, escape : Set[str] = set()) -> str:
    idx = string.rfind(pattern)
    if idx == -1:
        return string
    else:
        if set(string[idx:]).difference(escape):
            return string
        else:
            return string[:idx] + string[idx+1:]
    
class _Counter:
    def __init__(self, cnt):
        self.cnt = cnt
    def inc(self, cnt = 1):
        self.cnt += cnt
        cnt = self.cnt - cnt
        return cnt

import re

ws = re.compile(r'\s+')
def encode_integral(val : int):
    return val.to_bytes(4, 'little').decode('latin-1')

import os

def add_dll_dir(dll: str):
    import sys
    try:
        if sys.version_info.major >= 3 and sys.version_info.minor >7 and os.name == 'nt':
            os.add_dll_directory(dll)
        else:
            os.environ['PATH'] = os.path.abspath(dll) + os.pathsep + os.environ['PATH']
    except FileNotFoundError:
        print(f"Error: path not found")

nullstream = open(os.devnull, 'w')


def clamp(val, minval, maxval):
    return min(max(val, minval), maxval)

def escape_qoutes(string : str):
    return re.sub(r'^\'', r'\'',re.sub(r'([^\\])\'', r'\1\'', string))

def get_innermost(sl):
    if sl and type(sl) is dict:
        if 'literal' in sl and type(sl['literal']) is str:
            return f"'{get_innermost(sl['literal'])}'"
        return get_innermost(next(iter(sl.values()), None))
    elif sl and type(sl) is list:
        return get_innermost(sl[0])
    else:
        return sl


def send_to_server(payload : str):
    from prompt import PromptState
    cxt : PromptState = session_context
    if cxt is None:
        raise RuntimeError("Error! no session specified.")
    else:
        from ctypes import c_char_p
        cxt.payload = (c_char_p*1)(c_char_p(bytes(payload, 'utf-8')))
        cxt.cfg.has_dll = 0
        cxt.send(1, cxt.payload)
        cxt.set_ready()

def get_storedproc(name : str):
    from prompt import PromptState, StoredProcedure
    cxt : PromptState = session_context
    if cxt is None:
        raise RuntimeError("Error! no session specified.")
    else:
        ret : StoredProcedure = cxt.get_storedproc(bytes(name, 'utf-8'))
        if (
            ret.name and 
            ret.name.decode('utf-8') != name
        ):
            print(f'Procedure {name} mismatch in server {ret.name.value}')
            return None
        else:
            return ret

def execute_procedure(proc):
    pass

import enum
class Backend_Type(enum.Enum):
    BACKEND_AQuery = 0
    BACKEND_MonetDB = 1
    BACKEND_MariaDB = 2
    BACKEND_DuckDB = 3
    BACKEND_SQLite = 4
    BACKEND_TOTAL = 5
backend_strings = {
    'aquery': Backend_Type.BACKEND_AQuery,
    'monetdb': Backend_Type.BACKEND_MonetDB,
    'mariadb': Backend_Type.BACKEND_MariaDB,
    'duckdb': Backend_Type.BACKEND_DuckDB,
    'sqlite': Backend_Type.BACKEND_SQLite,
}
