import uuid

def base62uuid(crop=8):
    alp = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
    id = uuid.uuid4().int
    ret = ''

    while id:
        ret = alp[id % 62] + ret
        id //= 62
    
    return ret[:crop] if len(ret) else '0'