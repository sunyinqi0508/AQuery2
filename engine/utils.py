import uuid

base62alp = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
nums = '0123456789'
reserved_monet = ['month']
def base62uuid(crop=8):
    id = uuid.uuid4().int
    ret = ''
    
    while id:
        ret = base62alp[id % 62] + ret
        id //= 62
    
    return ret[:crop] if len(ret) else '0'

def get_leagl_name(name, lower = True):
    if name is not None:
        if lower:
            name = name.lower()
        name = ''.join([n for n in name if n in base62alp or n == '_'])
        
    if name is None or len(name) == 0 or set(name) == set('_'):
        name = base62uuid(8)
    if(name[0] in nums):
        name = '_' + name
        
    return name

def check_leagl_name(name):
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