import uuid

base62alp = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'

def base62uuid(crop=8):
    id = uuid.uuid4().int
    ret = ''
    
    while id:
        ret = base62alp[id % 62] + ret
        id //= 62
    
    return ret[:crop] if len(ret) else '0'

def enlist(l): 
    return l if type(l) is list else [l]

def seps(s, i, l):
    return s if i < len(l) - 1 else ''