import struct
import readline
from typing import List

name : str = input('Filename (in path ./procedures/<filename>.aqp):')

def write():
    s : str = input('Enter queries: empty line to stop. \n')
    qs : List[str] = []

    while(len(s) and not s.startswith('S')):
        qs.append(s)
        s = input()

    ms : int = int(input('number of modules:'))

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
        

def read(cmd : str):
    rc = len(cmd) > 1 and cmd[1] == 'c'
    clip = ''
    with open(f'./procedures/{name}.aqp', 'rb') as fp:
        nq = struct.unpack("I", fp.read(4))[0]
        ms = struct.unpack("I", fp.read(4))[0]
        qs = fp.read().split(b'\x00')
        print(f'Procedure {name}, {nq} queries, {ms} modules:')
        for q in qs:
            q = q.decode('utf-8').strip()
            if q:
                q = f'"{q}",' if rc else f'\t{q}'
                print(q)
                clip += q + '\n'
    if rc and not input('copy to clipboard?').lower().startswith('n'):
        import pyperclip
        pyperclip.copy(clip)

if __name__ == '__main__':
    import os
    files = os.listdir('./procedures/')
    while True:
        cmd = input("r for read, rc to read c_str, w for write: ")
        if cmd.lower().startswith('ra'):
            for f in files:
                if f.endswith('.aqp'):
                    name = f[:-4]
                    read('r')
                    input('press any key to continue')
        elif cmd.lower().startswith('r'):
            read(cmd.lower())
            break
        elif cmd.lower().startswith('w'):
            write()
            break
        elif cmd.lower().startswith('q'):
            break
        