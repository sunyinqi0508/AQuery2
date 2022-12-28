import struct
import readline
from typing import List

name : str = input()

def write():
    s : str = input()
    qs : List[str] = []

    while(len(s) and not s.startswith('S')):
        qs.append(s)
        s = input()

    ms : int = int(input())

    with open(f'./procedures/{name}.aqp', 'wb') as fp:
        fp.write(struct.pack("I", len(qs) + (ms > 0)))
        fp.write(struct.pack("I", ms))
        if (ms > 0):
            fp.write(b'N\x00')
            
        for q in qs:
            fp.write(q.encode('utf-8'))
            if q.startswith('Q'):
                fp.write(b'\n ')
            fp.write(b'\x00')
        

def read():
    with open(f'./procedures/{name}.aqp', 'rb') as fp:
        nq = struct.unpack("I", fp.read(4))[0]
        ms = struct.unpack("I", fp.read(4))[0]
        qs = fp.read().split(b'\x00')
        print(f'Procedure {name}, {nq} queries, {ms} modules:')
        for q in qs:
            print('    ' + q.decode('utf-8'))
            
            
if __name__ == '__main__':
    while True:
        cmd = input("r for read, w for write: ")
        if cmd.lower().startswith('r'):
            read()
            break
        elif cmd.lower().startswith('w'):
            write()    
            break
        elif cmd.lower().startswith('q'):
            break
        