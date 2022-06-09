import enum
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

import threading
import ctypes

class RunType(enum.Enum):
    Threaded = 0
    IPC = 1

server_mode = RunType.Threaded

server_bin = 'server.bin' if server_mode == RunType.IPC else 'server.so'

try:
    os.remove(server_bin) 
except Exception as e:
    print(type(e), e)
    
nullstream = open(os.devnull, 'w')

subprocess.call(['make', server_bin], stdout=nullstream)
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

def init_ipc():
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

import numpy as np

c = lambda _ba: ctypes.cast((ctypes.c_char * len(_ba)).from_buffer(_ba), ctypes.c_char_p)

class Config:
    def __init__(self, nq = 0, mode = server_mode, n_bufs = 0, bf_szs = []) -> None:
        self.int_size = 4
        self.n_attrib = 4
        self.buf = bytearray((self.n_attrib + n_bufs) * self.int_size)
        self.np_buf = np.ndarray(shape=(self.n_attrib), buffer=self.buf, dtype=np.int32)
        self.new_query = nq
        self.server_mode = mode.value 
        self.running = 1
        self.n_buffers = n_bufs
        
    def __getter (self, *, i):
        return self.np_buf[i]
    def __setter(self, v, *, i):
        self.np_buf[i] = v
    def binder(_i):
        from functools import partial
        return property(partial(Config.__getter, i = _i), partial(Config.__setter, i = _i))
    
    running = binder(0)
    new_query = binder(1)
    server_mode = binder(2)
    backend_type = binder(3)
    n_buffers = binder(4)

    def set_bufszs(self, buf_szs):
        for i in range(min(len(buf_szs), self.n_buffers)):
            self.np_buf[i+self.n_attrib] = buf_szs[i]

    @property
    def c(self):
        return c(self.buf)

cfg = Config()
th = None
    
def init_threaded():
    
    if os.name == 'nt':
        t = os.environ['PATH'].lower().split(';')
        vars = re.compile('%.*%')
        for e in t:
            if(len(e) != 0):
                if '%' in e:
                    try:
                        m_e = vars.findall(e)
                        for m in m_e:
                            e = e.replace(m, os.environ[m[1:-1]])
                        # print(m, e)
                    except Exception:
                        continue
                os.add_dll_directory(e)
    
    server_so = ctypes.CDLL('./'+server_bin)
    global cfg, th
    th = threading.Thread(target=server_so['main'], args=(-1, ctypes.POINTER(ctypes.c_char_p)(cfg.c)), daemon=True)
    th.start()
        
if server_mode == RunType.IPC:
    atexit.register(rm)
    init = init_ipc
    set_ready = lambda : mm.seek(0,os.SEEK_SET) or mm.write(b'\x01\x01')
    def __get_ready():
        mm.seek(0,os.SEEK_SET)
        return  mm.read(2)[1]
    get_ready = __get_ready
    server_status = lambda : server.poll() is not None
else:
    init = init_threaded
    rm = lambda: None
    def __set_ready():
        global cfg
        cfg.new_query = 1
    set_ready = __set_ready
    get_ready = lambda:cfg.new_query
    server_status = lambda : not th.is_alive()
init()

test_parser = True

# code to test parser
ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)


keep = True
cxt = engine.initialize()
cxt.Info(res)
while test_parser:
    try:
        if server_status():
            init()
        while get_ready():
            time.sleep(.00001)
        print("> ", end="")
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
                set_ready()
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
                set_ready()
            continue
        elif q == 'rr':
            set_ready()
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
