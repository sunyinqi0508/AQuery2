import aquery_config
help_message = '''\
======================================================
                AQUERY COMMANDLINE HELP
======================================================

Run prompt.py without supplying with any arguments to run in interactive mode.

--help, -h
    print out this help message
    
--version, -v
    returns current version of AQuery

--mode, -m Optional[threaded|IPC]
    execution engine run mode:
        threaded or 1 (default): run the execution engine and compiler/prompt in separate threads
        IPC, standalong or 2: run the execution engine in a new process which uses shared memory to communicate with the compiler process

--script, -s [SCRIPT_FILE]
    script mode: run the aquery script file

--parse-only, -p
    parse only: parse the file and print out the AST
'''

prompt_help = '''\

******** AQuery Prompt Help *********

help:
    print this help message
help commandline:
    print help message for AQuery Commandline
<sql statement>: 
    parse sql statement
f <query file>: 
    parse all AQuery statements in file
script <AQuery Script file>:
    run AQuery Script in file
dbg: 
    start debugging session with current context
print: 
    printout parsed sql statements
exec: 
    execute last parsed statement(s) with AQuery Execution Engine (disabled)
xexec: 
    execute last parsed statement(s) with Hybrid Execution Engine
r: 
    run the last generated code snippet
save <OPTIONAL: filename>: 
    save current code snippet. will use timestamp as filename if not specified.
exit or Ctrl+C: 
    exit prompt mode
'''

if __name__ == '__main__':
    import mimetypes
    mimetypes._winreg = None
    state = None
    nextcmd = ''
    def check_param(param, args = False) -> bool:
        global nextcmd
        import sys
        for p in param:
            if p in sys.argv:
                if args:
                    return True
                pos = sys.argv.index(p)
                if len(sys.argv) > pos + 1:
                    nextcmd = sys.argv[pos + 1]
                    return True
        return False
    
    if check_param(['-v', '--version'], True):
        print(aquery_config.version_string)
        exit()

    if check_param(['-h', '--help'], True):
        print(help_message)
        exit()
    

    
import os
from dataclasses import dataclass
import enum
import time
# import dbconn
import re
from typing import Callable, List, Optional
from mo_parsing import ParseException
import aquery_parser as parser
import engine
import engine.projection
import engine.ddl
import reconstruct as xengine
import subprocess
import mmap
import sys
from engine.utils import base62uuid
import atexit
import threading
import ctypes
import numpy as np
from engine.utils import ws
from engine.utils import add_dll_dir
from engine.utils import nullstream
from build import build_manager

## CLASSES BEGIN
class RunType(enum.Enum):
    Threaded = 0
    IPC = 1

class Backend_Type(enum.Enum):
	BACKEND_AQuery = 0
	BACKEND_MonetDB = 1
	BACKEND_MariaDB = 2

@dataclass 
class QueryStats:
    last_time : int = time.time()
    parse_time : int = 0
    codegen_time : int = 0
    compile_time : int = 0
    exec_time : int = 0
    need_print : bool = False
    def clear(self):
        self.parse_time = 0
        self.codegen_time = 0
        self.compile_time = 0
        self.exec_time  = 0
        self.last_time = time.time()
        
    def stop(self):
        ret = time.time() - self.last_time
        self.last_time = time.time()
        return ret
    
    def cumulate(self, other : Optional['QueryStats']):
        if other:
            other.parse_time += self.parse_time
            other.codegen_time += self.codegen_time
            other.compile_time += self.compile_time
            other.exec_time += self.exec_time
        
    def print(self, cumulative = None, clear = True, need_print = True):
        if self.need_print:
            if cumulative:
                self.exec_time = self.stop()
                self.cumulate(cumulative)
            if need_print:
                print(f'Parse Time: {self.parse_time}, Codegen Time: {self.codegen_time}, Compile Time: {self.compile_time}, Execution Time: {self.exec_time}.')
                print(f'Total Time: {self.parse_time + self.codegen_time + self.compile_time + self.exec_time}')
                self.need_print = False
            if clear:
                self.clear()
class Config:
    __all_attrs__ = ['running', 'new_query', 'server_mode', 
                     'backend_type', 'has_dll', 
                     'postproc_time', 'sql_time', 
                     'n_buffers'
                     ]
    __init_attributes__ = False
    
    @staticmethod
    def __init_self__():
        if not Config.__init_attributes__:
            from functools import partial
            for _i, attr in enumerate(Config.__all_attrs__):
                if not hasattr(Config, attr):
                    setattr(Config, attr, property(partial(Config.getter, i = _i), partial(Config.setter, i = _i)))
            Config.__init_attributes__ = True
            
    def __init__(self, mode, nq = 0, n_bufs = 0, bf_szs = []) -> None:
        Config.__init_self__()
        self.int_size = 4
        self.n_attrib = len(Config.__all_attrs__)
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
        return ctypes.cast(\
            (ctypes.c_char * len(self.buf)).from_buffer(self.buf), ctypes.c_char_p)

@dataclass
class PromptState():
    cleanup = True
    th = None
    send = None
    test_parser = True
    server_mode: RunType = RunType.Threaded
    server_bin = 'server.bin' if server_mode == RunType.IPC else 'server.so'
    set_ready = lambda: None
    get_ready = lambda: None
    server_status = lambda: False
    cfg : Config = None
    shm : str = ''
    server : subprocess.Popen = None
    basecmd : List[str] = None
    mm : mmap = None
    th : threading.Thread
    send : Callable = lambda *_:None
    init : Callable[['PromptState'], None] = lambda _:None
    stmts = ['']
    payloads = {}
    need_print : bool = False
    stats : Optional[QueryStats] = None
    currstats : Optional[QueryStats] = None
    buildmgr : Optional[build_manager]= None
## CLASSES END

## FUNCTIONS BEGIN
def rm(state:PromptState):
    if state.cleanup:
        state.mm.seek(0,os.SEEK_SET)
        state.mm.write(b'\x00\x00')
        state.mm.flush()
    
        try:
            time.sleep(.001)
            state.server.kill()
            time.sleep(.001)
            state.server.terminate()
        except OSError:
            pass
    
        files = os.listdir('.')
        for f in files:
            if f.endswith('.shm'):
                os.remove(f)
        state.mm.close()
        state.cleanup = False
        nullstream.close()

def init_ipc(state: PromptState):
    state.shm = base62uuid()
    if sys.platform != 'win32':
        state.shm += '.shm'
        state.basecmd = ['bash', '-c', 'rlwrap k']
        state.mm = None
        if not os.path.isfile(shm):
            # create initial file
            with open(shm, "w+b") as handle:
                handle.write(b'\x01\x00') # [running, new job]
                handle.flush()
                state.mm = mmap.mmap(handle.fileno(), 2, access=mmap.ACCESS_WRITE, offset=0)
        if state.mm is None:
            exit(1)
    else:
        state.basecmd = ['bash.exe', '-c', 'rlwrap ./k']
        state.mm = mmap.mmap(0, 2, state.shm)
        state.mm.write(b'\x01\x00')
        state.mm.flush()
    state.server = subprocess.Popen(["./server.bin", state.shm])

def init_threaded(state : PromptState):
    state.cleanup = False
    if os.name == 'nt' and aquery_config.add_path_to_ldpath:
        t = os.environ['PATH'].lower().split(';')
        vars = re.compile('%.*%')
        add_dll_dir(os.path.abspath('.'))
        add_dll_dir(os.path.abspath('./lib'))
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
                    add_dll_dir(e)
                except Exception:
                    continue
    
    else:
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + os.path.abspath('.')
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + os.path.abspath('./lib')
    if aquery_config.run_backend:    
        server_so = ctypes.CDLL('./'+state.server_bin)
        state.send = server_so['receive_args']
        aquery_config.have_hge = server_so['have_hge']()
        if aquery_config.have_hge != 0:
            from engine.types import get_int128_support
            get_int128_support()
        state.th = threading.Thread(target=server_so['main'], args=(-1, ctypes.POINTER(ctypes.c_char_p)(state.cfg.c)), daemon=True)
        state.th.start()

def init_prompt() -> PromptState:
    aquery_config.init_config()
    
    state = PromptState()
    # if aquery_config.rebuild_backend:
    #     try:
    #         os.remove(state.server_bin) 
    #     except Exception as e:
    #         print(type(e), e)
    #     subprocess.call(['make', "info"])
    #     subprocess.call(['make', state.server_bin], stdout=nullstream)
    state.buildmgr = build_manager()  
    state.buildmgr.build_caches()  
    state.cfg = Config(state.server_mode)
    state.stats = QueryStats()
    state.currstats = QueryStats()
    
    if state.server_mode == RunType.IPC:
        atexit.register(lambda: rm(state))
        state.init = init_ipc
        state.set_ready = lambda : state.mm.seek(0,os.SEEK_SET) or state.mm.write(b'\x01\x01')
        def __get_ready():
            state.mm.seek(0,os.SEEK_SET)
            return  state.mm.read(2)[1]
        state.get_ready = __get_ready
        state.server_status = lambda : state.server.poll() is not None
    else:
        state.init = init_threaded
        rm = lambda: None
        def __set_ready():
            state.cfg.new_query = 1
        state.set_ready = __set_ready
        state.get_ready = lambda: aquery_config.run_backend and state.cfg.new_query
        if aquery_config.run_backend:
            state.server_status = lambda : not state.th.is_alive()
        else:
            state.server_status = lambda : True
    state.init(state)
    return state

def save(q:str, cxt: xengine.Context):
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

def prompt(running = lambda:True, next = lambda:input('> '), state = None):
    if state is None:
        state = init_prompt()
    q = ''
    payload = None
    keep = True
    cxt = engine.initialize()
    # state.currstats = QueryStats()
    # state.need_print = False
    while running():
        try:
            if state.server_status():
                state.init()
            while state.get_ready():
                time.sleep(.00001)
            state.currstats.print(state.stats, need_print=state.need_print)
            try:
                og_q : str = next()
                state.currstats.stop()
            except EOFError:
                print('stdin inreadable, Exiting...')
                exit(0)
            q = og_q.lower().strip()
            if (not re.sub(r'[ \r\n\t;]', '', q)):
                continue
            if False and q == 'exec': # generate build and run (AQuery Engine)
                state.cfg.backend_type = Backend_Type.BACKEND_AQuery.value
                cxt = engine.exec(state.stmts, cxt, keep)
                if state.buildmgr.build_dll() == 0:
                    state.set_ready()
                continue
            elif q.startswith('echo '):
                print(og_q[5:].lstrip())
                continue
            elif q.startswith('list '):
                qs = re.split(r'[ \t]', q)
                if len(qs) > 1 and qs[1].startswith('table'):
                    for t in cxt.tables:
                        lst_cols = []
                        for c in t.columns:
                            lst_cols.append(f'{c.name} : {c.type}')
                        print(f'{t.table_name} ({", ".join(lst_cols)})')
                continue
            elif q.startswith('help'):
                qs = re.split(r'[ \t]', q)
                if len(qs) > 1 and qs[1].startswith('c'):
                    print(help_message)
                else:
                    print(prompt_help)
                continue
            elif q.startswith('xexec') or q.startswith('exec'): # generate build and run (MonetDB Engine)
                state.cfg.backend_type = Backend_Type.BACKEND_MonetDB.value
                cxt = xengine.exec(state.stmts, cxt, keep)
                
                this_udf = cxt.finalize_udf()
                if this_udf:
                    with open('udf.hpp', 'wb') as outfile:
                        outfile.write(this_udf.encode('utf-8'))
                        
                if state.server_mode == RunType.Threaded:
                    # assignment to avoid auto gc
                    # sqls =  [s.strip() for s in cxt.sql.split(';')]
                    qs = [ctypes.c_char_p(bytes(q, 'utf-8')) for q in cxt.queries if len(q)]
                    sz = len(qs)
                    payload = (ctypes.c_char_p*sz)(*qs)
                    state.payload = payload
                    try:
                        state.send(sz, payload)
                    except TypeError as e:
                        print(e)
                state.currstats.codegen_time = state.currstats.stop()
                state.currstats.compile_time = 0
                state.currstats.exec_time = 0
                qs = re.split(r'[ \t]', q)
                build_this = not(len(qs) > 1 and qs[1].startswith('n'))
                if cxt.has_dll:
                    with open('out.cpp', 'wb') as outfile:
                        outfile.write((cxt.finalize()).encode('utf-8'))
                    state.currstats.codegen_time += state.currstats.stop()
                        
                    if build_this:
                        state.buildmgr.build_dll()
                        state.cfg.has_dll = 1
                else:
                    state.cfg.has_dll = 0
                state.currstats.compile_time = state.currstats.stop()
                if build_this:
                    state.set_ready()
                state.currstats.need_print = True
                continue
            
            elif q == 'dbg':
                import code
                from copy import deepcopy
                var = {**globals(), **locals()}
                sh = code.InteractiveConsole(var)
                __stdin = os.dup(0)
                try:
                    sh.interact(banner = 'debugging session began.', exitmsg = 'debugging session ended.')
                except BaseException as e:
                    # dont care about whatever happened in dbg session
                    print(e)
                finally:
                    import io
                    sys.stdin = io.TextIOWrapper(io.BufferedReader(io.FileIO(__stdin, mode='rb', closefd=False)), encoding='utf8')
                continue
            elif q.startswith('log'):
                qs = re.split(r'[ \t]', q)
                if len(qs) > 1:
                    cxt.log_level = qs[1]
                else:
                    cxt.print(cxt.log_level)
                continue
            elif q == 'k':
                subprocess.call(state.basecmd)
                continue
            elif q == 'print':
                cxt.print(state.stmts)
                continue
            elif q.startswith('save'):
                save(q, cxt)
                continue
            elif q == 'keep':
                keep = not keep
                continue
            elif q == 'format' or q == 'fmt':
                subprocess.call(['clang-format', 'out.cpp'])
            elif q == 'exit' or q == 'exit()' or q == 'quit' or q == 'quit()' or q == '\\q':
                rm(state)
                exit()
            elif q.startswith('sh'):
                from distutils.spawn import find_executable
                qs = re.split(r'[ \t]', q)
                shells = ('zsh', 'bash', 'sh', 'fish', 'cmd', 'pwsh', 'powershell', 'csh', 'tcsh', 'ksh')
                shell_path = ''
                if len(qs) > 1 and qs[1] in shells:
                    shell_path = find_executable(qs[1])
                    if shell_path:
                        os.system(shell_path)
                else:
                    for sh in shells:
                        shell_path = find_executable(sh)
                        if shell_path:
                            os.system(shell_path)
                            break
                
                continue
            elif q == 'r': # build and run
                if state.server_mode == RunType.Threaded:
                    qs = [ctypes.c_char_p(bytes(q, 'utf-8')) for q in cxt.queries if len(q)]
                    sz = len(qs)
                    payload = (ctypes.c_char_p*sz)(*qs)
                    state.payload = payload
                    try:
                        state.send(sz, payload)
                    except TypeError as e:
                        print(e)
                if subprocess.call(['make', 'snippet']) == 0:
                    state.set_ready()
                continue
            elif q == 'rr': # run
                state.set_ready()
                continue
            elif q.startswith('script'):
                qs = re.split(r'[ \t]', q)
                if len(qs) > 1:
                    qs = qs[1]
                with open(qs) as file:
                    qs = file.readline()
                    from engine.utils import _Counter
                    lst_tell = -1
                    while(qs):
                        while(not ws.sub('', qs) or qs.strip().startswith('#')):
                            qs = file.readline()
                            if lst_tell == file.tell():
                                break
                            else:
                                lst_tell = file.tell()
                        cnt = _Counter(1)
                        prompt(lambda : cnt.inc(-1) > 0, lambda:qs.strip(), state)
                        qs = file.readline()
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
            elif q.startswith('stats'):
                qs = re.split(r'[ \t]', q)
                if len(qs) > 1:
                    if qs[1].startswith('on'):
                        state.need_print = True
                    elif qs[1].startswith('off'):
                        state.need_print = False
                    elif qs[1].startswith('last'):
                        state.currstats.need_print = True
                        state.currstats.print()
                    elif qs[1].startswith('reset'):
                        state.currstats.clear()
                    continue
                state.stats.need_print = True
                state.stats.print(clear = False)
                continue
            trimed = ws.sub(' ', og_q).split(' ') 
            if trimed[0].lower().startswith('f'):
                fn = 'stock.a' if len(trimed) <= 1 or len(trimed[1]) == 0 \
                                else trimed[1]
                try:
                    with open(fn, 'r') as file:
                        contents = file.read()#.lower()
                except FileNotFoundError:
                    with open('tests/' + fn, 'r') as file:
                        contents = file.read()
                state.stmts = parser.parse(contents)
                state.currstats.parse_time = state.currstats.stop()
                continue
            state.stmts = parser.parse(og_q.strip())
            cxt.Info(state.stmts)
            state.currstats.parse_time = state.currstats.stop()
        except (ParseException) as e:
            print(e)
            continue
        except (ValueError, FileNotFoundError) as e:
            try:
                os.remove('./cached')
            except:
                pass
            print(e)
        except (KeyboardInterrupt):
            break
        except SystemExit:
            print("\nBye.")
            raise
        except ValueError as e:
            import code, traceback
            __stdin = os.dup(0)
            raise_exception = True
            sh = code.InteractiveConsole({**globals(), **locals()})
            try:
                sh.interact(banner = traceback.format_exc(), exitmsg = 'debugging session ended.')
            except:
                pass
            finally:
                sys.stdin = io.TextIOWrapper(io.BufferedReader(io.FileIO(__stdin, mode='rb', closefd=False)), encoding='utf8')
            save('', cxt)
            rm(state)
            # control whether to raise exception in interactive console
            if raise_exception:
                raise e
        
    rm(state)
## FUNCTIONS END

## MAIN
if __name__ == '__main__':

    if len(sys.argv) == 2:
        nextcmd = sys.argv[1]
        if nextcmd.startswith('-'):
            nextcmd = ''
    
    #nextcmd = 'test.aquery'
    if nextcmd or check_param(['-s', '--script']):
        with open(nextcmd) as file:
            nextcmd = file.readline()
            from engine.utils import _Counter
            if file.name.endswith('aquery') or nextcmd.strip() == '#!aquery':
                state = init_prompt()
                while(nextcmd):
                    while(not ws.sub('', nextcmd) or nextcmd.strip().startswith('#')):
                        nextcmd = file.readline()
                    cnt = _Counter(1)
                    prompt(lambda : cnt.inc(-1) > 0, lambda:nextcmd.strip(), state)
                    nextcmd = file.readline()
        
    if check_param(['-p', '--parse']):
        with open(nextcmd, 'r') as file:
            contents = file.read()
            print(parser.parse(contents))
    
    if check_param(['-m', '--mode', '--run-type']):
        nextcmd = nextcmd.lower()
        ipc_string = ['ipc', 'proc', '2', 'standalong']
        thread_string = ['thread', '1']
        if any([s in nextcmd for s in ipc_string]):
            server_mode = RunType.IPC
        elif any([s in nextcmd for s in thread_string]):
            server_mode = RunType.Threaded
    
    if check_param(['-r', '--rebuild']):
        try:
            os.remove('./.cached')
        except FileNotFoundError:
            pass
    
    prompt(state=state)
    
