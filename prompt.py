import re
import time
import dbconn

from mo_parsing import ParseException
import aquery_parser as parser
import engine
import reconstruct as xengine
import subprocess
import mmap
import sys
import os
from engine.utils import base62uuid
import atexit
try:
    os.remove('server.bin') 
except Exception as e:
    print(type(e), e)
    
nullstream = open(os.devnull, 'w')

subprocess.call(['make', 'server.bin'], stdout=nullstream)
cleanup = True

def rm():
    global cleanup
    if cleanup:
        mm.seek(0,os.SEEK_SET)
        mm.write(b'\x00\x00')
        mm.flush()
    
        try:
            time.sleep(.001)
            server.kill()
            time.sleep(.001)
            server.terminate()
        except OSError:
            pass
    
        files = os.listdir('.')
        for f in files:
            if f.endswith('.shm'):
                os.remove(f)
        mm.close()
        cleanup = False
        nullstream.close()
        
atexit.register(rm)

def init():
    global shm, server, basecmd, mm
    shm = base62uuid()
    if sys.platform != 'win32':
        import readline
        shm += '.shm'
        basecmd = ['bash', '-c', 'rlwrap k']
        mm = None
        if not os.path.isfile(shm):
            # create initial file
            with open(shm, "w+b") as handle:
                handle.write(b'\x01\x00') # [running, new job]
                handle.flush()
                mm = mmap.mmap(handle.fileno(), 2, access=mmap.ACCESS_WRITE, offset=0)
        if mm is None:
            exit(1)
    else:
        basecmd = ['bash.exe', '-c', 'rlwrap ./k']
        mm = mmap.mmap(0, 2, shm)
        mm.write(b'\x01\x00')
        mm.flush()
    server = subprocess.Popen(["./server.bin", shm])

init()

test_parser = True

# code to test parser
ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)


# else:f
#     if subprocess.call(['make', 'snippet']) == 0:
#         mm.seek(0)
#         mm.write(b'\x01\x01')
#         time.sleep(.1)
#         mm.seek(0)
#         print(mm.read(2))
        
#         mm.close()
#         handle.close()
#         os.remove(shm)
#     exit()
keep = True
cxt = engine.initialize()
cxt.Info(res)
while test_parser:
    try:
        if server.poll() is not None:
            init()
        print("> ", end="")
        ready = 1
        while ready == 1:
            mm.seek(0,os.SEEK_SET)
            ready = mm.read(2)[1]
            time.sleep(.00001)
        q = input().lower()
        if q == 'exec':
            if not keep or cxt is None:
                cxt = engine.initialize()
            else:
                cxt.new()
            stmts_stmts = stmts['stmts']
            if type(stmts_stmts) is list:
                for s in stmts_stmts:
                    engine.generate(s, cxt)
            else:
                engine.generate(stmts_stmts, cxt)
            cxt.Info(cxt.ccode)
            with open('out.cpp', 'wb') as outfile:
                outfile.write((cxt.finalize()).encode('utf-8'))
            if subprocess.call(['make', 'snippet'], stdout = nullstream) == 0:
                mm.seek(0,os.SEEK_SET)
                mm.write(b'\x01\x01')
            continue
        if q == 'xexec':
            cxt = xengine.initialize()
            stmts_stmts = stmts['stmts']
            if type(stmts_stmts) is list:
                for s in stmts_stmts:
                    xengine.generate(s, cxt)
            else:
                xengine.generate(stmts_stmts, cxt)
            print(cxt.sql)
            continue
        
        elif q.startswith('log'):
            qs = re.split(' |\t', q)
            if len(qs) > 1:
                cxt.log_level = qs[1]
            else:
                cxt.print(cxt.log_level)
            continue
        elif q == 'k':
            subprocess.call(basecmd)
            continue
        elif q == 'print':
            cxt.print(stmts)
            continue
        elif q == 'keep':
            keep = not keep
            continue
        elif q == 'format' or q == 'fmt':
            subprocess.call(['clang-format', 'out.cpp'])
        elif q == 'exit':
            break
        elif q == 'r':
            if subprocess.call(['make', 'snippet']) == 0:
                mm.seek(0,os.SEEK_SET)  
                mm.write(b'\x01\x01')
            continue
        elif q.startswith('save'):
            filename = re.split(' |\t', q)
            if (len(filename) > 1):
                filename = filename[1]
            else:
                filename = f'out_{base62uuid(4)}.cpp'
            with open(filename, 'wb') as outfile:
                outfile.write((cxt.finalize()).encode('utf-8'))
            continue
        trimed = ws.sub(' ', q.lower()).split(' ') 
        if trimed[0].startswith('f'):
            fn = 'stock.a' if len(trimed) <= 1 or len(trimed[1]) == 0 \
                            else trimed[1]
                
            with open(fn, 'r') as file:
                contents = file.read()#.lower()
            stmts = parser.parse(contents)
            continue
        stmts = parser.parse(q)
        cxt.Info(stmts)
    except ParseException as e:
        print(e)
        continue
    except (ValueError, FileNotFoundError) as e:
        # rm()
        # init()
        print(e)
    except (KeyboardInterrupt):
        break
    except (Exception) as e:
        rm()
        raise e

rm()
