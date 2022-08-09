import enum
import re
import time
# import dbconn
from mo_parsing import ParseException
import aquery_parser as parser
import engine
import engine.projection
import engine.ddl

import reconstruct as xengine
import subprocess
import mmap
import sys
import os
from engine.utils import base62uuid
import atexit

import threading
import ctypes

import aquery_config

class RunType(enum.Enum):
    Threaded = 0
    IPC = 1

class Backend_Type(enum.Enum):
	BACKEND_AQuery = 0
	BACKEND_MonetDB = 1
	BACKEND_MariaDB = 2

server_mode = RunType.Threaded

server_bin = 'server.bin' if server_mode == RunType.IPC else 'server.so'


    
nullstream = open(os.devnull, 'w')

if aquery_config.rebuild_backend:
    try:
        os.remove(server_bin) 
    except Exception as e:
        print(type(e), e)
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
        self.n_attrib = 6
        self.buf = bytearray((self.n_attrib + n_bufs) * self.int_size)
        self.np_buf = np.ndarray(shape=(self.n_attrib), buffer=self.buf, dtype=np.int32)
        self.new_query = nq
        self.server_mode = mode.value 
        self.running = 1
        self.backend_type = Backend_Type.BACKEND_AQuery.value
        self.has_dll = 0
        self.n_buffers = n_bufs
        
    def getter (self, *, i):
        return self.np_buf[i]
    def setter(self, v, *, i):
        self.np_buf[i] = v

    def set_bufszs(self, buf_szs):
        for i in range(min(len(buf_szs), self.n_buffers)):
            self.np_buf[i+self.n_attrib] = buf_szs[i]

    @property
    def c(self):
        return c(self.buf)

def binder(cls, attr, _i):
    from functools import partial
    setattr(cls, attr, property(partial(cls.getter, i = _i), partial(cls.setter, i = _i)))

binder(Config, 'running', 0)
binder(Config, 'new_query', 1)
binder(Config, 'server_mode', 2)
binder(Config, 'backend_type', 3)
binder(Config, 'has_dll', 4)
binder(Config, 'n_buffers', 5)

cfg = Config()
th = None
send = None
    
def init_threaded():
    if os.name == 'nt' and aquery_config.add_path_to_ldpath:
        t = os.environ['PATH'].lower().split(';')
        vars = re.compile('%.*%')
        os.add_dll_directory(os.path.abspath('.'))
        os.add_dll_directory(os.path.abspath('./lib'))
        for e in t:
            if(len(e) != 0):
                if '%' in e:
                    try:
                        m_e = vars.findall(e)
                        for m in m_e:
                            e = e.replace(m, os.environ[m[1:-1]])
                    except Exception:
                        continue
                try:
                    os.add_dll_directory(e)
                except Exception:
                    continue
    
    else:
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + os.path.abspath('.')
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + os.path.abspath('./lib')
    if aquery_config.run_backend:    
        server_so = ctypes.CDLL('./'+server_bin)
        global cfg, th, send
        send = server_so['receive_args']
        aquery_config.have_hge = server_so['have_hge']()
        if aquery_config.have_hge:
            from engine.types import get_int128_support
            get_int128_support()
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
    get_ready = lambda: aquery_config.run_backend and cfg.new_query
    if aquery_config.run_backend:
        server_status = lambda : not th.is_alive()
    else:
        server_status = lambda : True
init()

test_parser = True

# code to test parser
ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)

payload = None
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
        if q == 'exec': # generate build and run (AQuery Engine)
            cfg.backend_type = Backend_Type.BACKEND_AQuery.value
            cxt = engine.exec(stmts, cxt, keep)
            if subprocess.call(['make', 'snippet'], stdout = nullstream) == 0:
                set_ready()
            continue
        
        elif q == 'xexec': # generate build and run (MonetDB Engine)
            cfg.backend_type = Backend_Type.BACKEND_MonetDB.value
            cxt = xengine.exec(stmts, cxt, keep)
            if server_mode == RunType.Threaded:
                # assignment to avoid auto gc
                # sqls =  [s.strip() for s in cxt.sql.split(';')]
                qs = [ctypes.c_char_p(bytes(q, 'utf-8')) for q in cxt.queries if len(q)]
                sz = len(qs)
                payload = (ctypes.c_char_p*sz)(*qs)
                try:
                    send(sz, payload)
                except TypeError as e:
                    print(e)
            if cxt.udf is not None:
                with open('udf.hpp', 'wb') as outfile:
                    outfile.write(cxt.udf.encode('utf-8'))
                
            if cxt.has_dll:
                with open('out.cpp', 'wb') as outfile:
                    outfile.write((cxt.finalize()).encode('utf-8'))
                subprocess.call(['make', 'snippet'], stdout = nullstream)
                cfg.has_dll = 1
            else:
                cfg.has_dll = 0
            set_ready()
            
            continue
        
        elif q == 'dbg':
            import code
            from copy import deepcopy
            var = {**globals(), **locals()}
            sh = code.InteractiveConsole(var)
            try:
                sh.interact(banner = 'debugging session began.', exitmsg = 'debugging session ended.')
            except BaseException as e: 
            # don't care about anything happened in interactive console
                print(e)
            continue
        elif q.startswith('log'):
            qs = re.split(r'[ \t]', q)
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
        elif q.startswith('save'):
            savecmd = re.split(r'[ \t]', q)
            if len(savecmd) > 1:
                fname = savecmd[1]
            else:
                tm = time.gmtime()
                fname = f'{tm.tm_year}{tm.tm_mon}_{tm.tm_mday}_{tm.tm_hour}:{tm.tm_min}:{tm.tm_sec}'
            if cxt:
                from typing import Optional
                def savefile(attr:str, desc:str, ext:Optional[str] = None):
                    if hasattr(cxt, attr):
                        attr : str = getattr(cxt, attr)
                        if attr:
                            ext = ext if ext else '.' + desc
                            name = fname if fname.endswith(ext) else fname + ext
                            with open('saves/' + name, 'wb') as cfile:
                                cfile.write(attr.encode('utf-8'))
                                print(f'saved {desc} code as {name}')
                savefile('ccode', 'cpp')
                savefile('udf', 'udf', '.hpp')
                savefile('sql', 'sql')
                
            continue
        elif q == 'keep':
            keep = not keep
            continue
        elif q == 'format' or q == 'fmt':
            subprocess.call(['clang-format', 'out.cpp'])
        elif q == 'exit':
            break
        elif q == 'r': # build and run
            if subprocess.call(['make', 'snippet']) == 0:
                set_ready()
            continue
        elif q == 'rr': # run
            set_ready()
            continue
        elif q.startswith('save2'):
            filename = re.split(r'[ \t]', q)
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
        print(e)
    except (KeyboardInterrupt):
        break
    except:
        rm()
        raise
rm()
