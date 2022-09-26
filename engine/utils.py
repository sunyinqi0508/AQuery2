from collections import OrderedDict
from collections.abc import MutableMapping, Mapping
import uuid

lower_alp = 'abcdefghijklmnopqrstuvwxyz'
upper_alp = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
nums = '0123456789'
base62alp = nums + lower_alp + upper_alp

reserved_monet = ['month']


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
import os

def add_dll_dir(dll: str):
    import sys
    if sys.version_info.major >= 3 and sys.version_info.minor >7 and os.name == 'nt':
        os.add_dll_directory(dll)
    else:
        os.environ['PATH'] = os.path.abspath(dll) + os.pathsep + os.environ['PATH']
        
nullstream = open(os.devnull, 'w')


def clamp(val, minval, maxval):
    return min(max(val, minval), maxval)

def escape_qoutes(string : str):
    return re.sub(r'^\'', r'\'',re.sub(r'([^\\])\'', r'\1\'', string))
