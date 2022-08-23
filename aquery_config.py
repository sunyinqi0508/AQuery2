# put environment specific configuration here

## GLOBAL CONFIGURATION FLAGS
add_path_to_ldpath = True
rebuild_backend = True
run_backend = True
have_hge = False
os_platform = 'unkown'
cygroot = 'c:/msys64/usr/bin'
msbuildroot = 'd:/gg/vs22/MSBuild/Current/Bin'
__config_initialized__ = False

class Build_Config:
    def __init__(self) -> None:
        self.OptimizationLv = '4' # [O0, O1, O2, O3, Ofast]
        self.Platform = 'x64'
        self.fLTO = True
        self.fmarchnative = True
        
    def configure(self):
        pass


def init_config():
    global __config_initialized__, os_platform
## SETUP ENVIRONMENT VARIABLES
    import os
    from engine.utils import add_dll_dir
    # os.environ['CXX'] = 'C:/Program Files/LLVM/bin/clang.exe'
    os.environ['THREADING'] = '1'

    if not __config_initialized__:
        import sys
        if os.name == 'nt':
            if sys.platform == 'win32':
                os_platform = 'win'
            elif sys.platform == 'cygwin' or sys.platform == 'msys':
                os_platform = 'cygwin'
        elif os.name == 'posix':
            if sys.platform == 'darwin':
                os_platform = 'mac'
            elif 'linux' in sys.platform:
                os_platform = 'linux'
            elif 'bsd' in sys.platform:
                os_platform = 'bsd'
            elif sys.platform == 'cygwin' or sys.platform == 'msys':
                os_platform = 'cygwin'
        # deal with msys dependencies:
        if os_platform == 'win':

            add_dll_dir(cygroot)  
            add_dll_dir(os.path.abspath('./msc-plugin'))
            # print("adding path")
        else:
            import readline
            if os_platform == 'cygwin':
                add_dll_dir('./lib')
        __config_initialized__ = True
        