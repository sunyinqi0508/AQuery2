from dataclasses import dataclass
from enum import Enum, auto
import aquery_config
import sys
import os
import subprocess
import hashlib
import pickle
from common.utils import nullstream
from typing import Dict, Optional, Set, Union

@dataclass
class checksums:
    libaquery_a : Optional[Union[bytes, bool]] = None
    pch_hpp_gch : Optional[Union[bytes, bool]] = None
    server : Optional[Union[bytes, bool]] = None
    sources : Optional[Union[Dict[str, bytes], bool]] = None
    env : str = ''
    
    def calc(self, compiler_name, libaquery_a = 'libaquery.a' , 
                pch_hpp_gch = 'server/pch.hpp.gch', 
                server = 'server.so'
        ):
        from platform import machine
        self.env = (aquery_config.os_platform +
                    machine() + 
                    aquery_config.build_driver + 
                    compiler_name + 
                    aquery_config.version_string
                    + str(os.environ['AQ_DEBUG'] == '1')
                )
        for key in self.__dict__.keys():
            try:
                with open(eval(key), 'rb') as file:
                    self.__dict__[key] = hashlib.md5(file.read()).digest()
            except (FileNotFoundError, NameError):
                pass
        sources = [*build_manager.headerfiles, *build_manager.sourcefiles]
        self.sources = dict()
        for key in sources:
            try:
                with open(key, 'rb') as file:
                    self.sources[key] = hashlib.md5(file.read()).digest()
            except FileNotFoundError:
                print('missing component: ' + key)
                self.sources[key] = None
                
    def __ne__(self, __o: 'checksums') -> 'checksums':
        ret = checksums()
        for key in self.__dict__.keys():
            try:
                ret.__dict__[key] = (
                    not (self.__dict__[key] and __o.__dict__[key]) or
                    self.__dict__[key] != __o.__dict__[key]
                )
            except KeyError:
                ret.__dict__[key] = True
        return ret
    
    def __eq__(self, __o: 'checksums') -> 'checksums':
        ret = checksums()
        for key in self.__dict__.keys():
            try:
                ret.__dict__[key] = (
                    self.__dict__[key] and __o.__dict__[key] and
                    self.__dict__[key] == __o.__dict__[key]
                )
            except KeyError:
                ret.__dict__[key] = False
        return ret

        
        
class build_manager:
    sourcefiles = [
                   'build.py', 'Makefile', 
                   'server/server.cpp', 'server/libaquery.cpp',  
                   'server/monetdb_conn.cpp', 'server/duckdb_conn.cpp',
                   'server/threading.cpp', 'server/winhelper.cpp', 
                   'server/monetdb_ext.c'
                   ]
    headerfiles = ['server/aggregations.h', 'server/hasher.h', 'server/io.h', 
                   'server/libaquery.h', 'server/monetdb_conn.h', 'server/duckdb_conn.h', 
                   'server/pch.hpp', 'server/table.h', 'server/threading.h', 
                   'server/types.h', 'server/utils.h', 'server/winhelper.h', 
                   'server/gc.h', 'server/vector_type.hpp', 'server/table_ext_monetdb.hpp' 
                   ]
   
    class DriverBase:
        def __init__(self, mgr : 'build_manager') -> None:
            self.mgr = mgr
            self.build_cmd = []
        def libaquery_a(self) :
            return False
        def pch(self):
            return False
        def build(self, stdout = sys.stdout, stderr = sys.stderr):
            ret = True
            if not aquery_config.compilation_output:
                stdout = nullstream
                stderr = nullstream
            for c in self.build_cmd:
                if c:
                    try: # only last success matters
                        ret = not subprocess.call(c, stdout = stdout, stderr = stderr) # and ret
                    except (FileNotFoundError):
                        ret = False
                        pass
            return ret
        def warmup(self):
            return True
                
    class MakefileDriver(DriverBase):
        def __init__(self, mgr : 'build_manager') -> None:
            super().__init__(mgr)
            os.environ['PCH'] = f'{mgr.PCH}'
            if 'CXX' not in os.environ:
                os.environ['CXX'] = mgr.cxx if mgr.cxx else 'c++'
            else:
                mgr.cxx = os.environ['CXX']
            if 'AQ_DEBUG' not in os.environ:
                os.environ['AQ_DEBUG'] = ('0' if mgr.OptimizationLv != '0' else '1')

        def libaquery_a(self):
            self.build_cmd = [['rm', 'libaquery.a'],['make', 'libaquery', '-j']]
            return self.build()
        def pch(self):
            self.build_cmd = [['rm', 'server/pch.hpp.gch'], ['make', 'pch', '-j']]
            return self.build()
        def server(self):
            if self.mgr.StaticLib:
                self.build_cmd = [['rm', '*.o'],['rm', 'server.so'], ['make', 'server_uselib', '-j']]
            else:
                self.build_cmd = [['rm', 'server.so'], ['make', 'server.so', '-j']]
            return self.build()
            
        def snippet(self):
            if self.mgr.StaticLib:
                self.build_cmd = [['make', 'snippet_uselib', '-j']]
            else:
                self.build_cmd = [['rm', 'dll.so'], ['make', 'snippet', '-j']]
            return self.build()

    class MSBuildDriver(DriverBase):
        platform_map = {'amd64':'x64', 'arm64':'arm64', 'x86':'win32'}
        opt_map = {'0':'Debug', '1':'RelWithDebugInfo', '2':'Release', '3':'Release', '4':'Release'}
        def __init__(self, mgr : 'build_manager') -> None:
            super().__init__(mgr)
            mgr.cxx = aquery_config.msbuildroot
            
        def get_flags(self):
            self.platform = self.platform_map[self.mgr.Platform]
            self.platform = f'/p:platform={self.platform}'
            self.opt = self.opt_map[self.mgr.OptimizationLv]
            self.opt = f'/p:configuration={self.opt}'

        def libaquery_a(self):
            loc = os.path.abspath('./msc-plugin/libaquery.vcxproj')
            self.get_flags()
            self.build_cmd = [['del', 'libaquery.lib'], [aquery_config.msbuildroot, loc, self.opt, self.platform]]
            return self.build()

        def pch(self):
            return True

        def server(self):
            print(self.opt)
            loc = os.path.abspath('./msc-plugin/server.vcxproj')
            self.get_flags()
            self.build_cmd = [['del', 'server.so'], [aquery_config.msbuildroot, loc, self.opt, self.platform]]
            return self.build()

        def snippet(self):
            loc = os.path.abspath('./msc-plugin/msc-plugin.vcxproj')
            self.get_flags()
            self.build_cmd = [[aquery_config.msbuildroot, loc, self.opt, self.platform]]
            return self.build()

        def warmup(self):
            self.build_cmd = [['make', 'warmup']]
            return self.build()
            
    #class PythonDriver(DriverBase):
    #    def __init__(self, mgr : 'build_manager') -> None:
    #        super().__init__(mgr)           

    def __init__(self) -> None:
        self.method = 'make'
        self.cxx = ''
        self.OptimizationLv = '4' # [O0, O1, O2, O3, Ofast]
        self.Platform = 'amd64'
        self.PCH = os.environ['PCH'] if 'PCH' in os.environ else 1
        self.StaticLib = 1
        self.fLTO = 1
        self.fmarchnative = 1
        if aquery_config.build_driver == 'MSBuild':
            self.driver = build_manager.MSBuildDriver(self) 
        elif aquery_config.build_driver == 'Makefile':
            self.driver = build_manager.MakefileDriver(self)
        self.cache_status = checksums()
        
    def build_dll(self):
        return self.driver.snippet()
        
    def build_caches(self, force = False):
        cached = checksums()
        current = checksums()
        libaquery_a = 'libaquery.a' 
        if aquery_config.os_platform == 'win':
            libaquery_a = 'libaquery.lib'
        current.calc(self.cxx, libaquery_a)
        try:
            with open('.cached', 'rb') as cache_sig:
                cached = pickle.loads(cache_sig.read())
        except FileNotFoundError:
            pass
        self.cache_status = current != cached
        
        success = True
        if  (force or 
             self.cache_status.sources or 
             self.cache_status.env
        ):
            success &= self.driver.pch()
            success &= self.driver.libaquery_a()
            success &= self.driver.server()
        else:
            if self.cache_status.libaquery_a:
                success &= self.driver.libaquery_a() 
            if self.cache_status.pch_hpp_gch:
                success &= self.driver.pch() 
            if self.cache_status.server:
                success &= self.driver.server() 
        if success:
            current.calc(self.cxx, libaquery_a)
            with open('.cached', 'wb') as cache_sig:
                cache_sig.write(pickle.dumps(current))
            self.driver.warmup()
            
            
        else:
            if aquery_config.os_platform == 'mac':
                os.system('./arch-check.sh')
            try:
                os.remove('./.cached')
            except:
                pass
            
