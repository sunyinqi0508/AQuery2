# put environment specific configuration here
import os

## GLOBAL CONFIGURATION FLAGS

version_string = '0.7.7a'
add_path_to_ldpath = True
rebuild_backend = False
run_backend = True
have_hge = False
cygroot = 'c:/mingw64/usr/bin'
msbuildroot = ''
os_platform = 'unknown'
build_driver = 'Auto'
compilation_output = True
compile_use_gc = True
compile_use_threading = True

## END GLOBAL CONFIGURATION FLAGS

def init_config():
    global __config_initialized__, os_platform, msbuildroot, build_driver
## SETUP ENVIRONMENT VARIABLES
    # __config_initialized__ = False
    #os_platform = 'unkown'
    #msbuildroot = 'd:/gg/vs22/MSBuild/Current/Bin'
    from common.utils import add_dll_dir
    # os.environ['CXX'] = 'C:/Program Files/LLVM/bin/clang.exe'
    os.environ['THREADING'] = '1'
    os.environ['AQUERY_ITC_USE_SEMPH'] = '1'
    if 'AQ_DEBUG' not in os.environ:
        os.environ['AQ_DEBUG'] = '0'
    if  ('__config_initialized__' not in globals() or 
            not __config_initialized__):
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
            add_dll_dir(os.path.abspath('./msc-plugin'))
            add_dll_dir(os.path.abspath('./deps'))
            add_dll_dir(cygroot)  
            if build_driver == 'Auto':
                try:
                    import vswhere
                    vsloc = vswhere.find(prerelease = True, latest = True, prop = 'installationPath')
                    if vsloc:
                        msbuildroot = vsloc[0] + '/MSBuild/Current/Bin/MSBuild.exe'
                        build_driver = 'MSBuild'
                    else:
                        print('Warning: No Visual Studio installation found.')
                        build_driver = 'Makefile'
                except ModuleNotFoundError:
                    build_driver = 'Makefile'
            # print("adding path")
        else:
            try:
                import readline
            except ImportError:
                print("Warning: Readline module not present")
            if build_driver == 'Auto':
                build_driver = 'Makefile'
            if os_platform == 'linux':
                os.environ['PATH'] += os.pathsep + '/usr/lib'
            if os_platform == 'cygwin':
                add_dll_dir('./lib')
            key_ld_library_path = 'LD_LIBRARY_PATH'
            patch_ld_library_path = os.getcwd()+ os.sep + 'deps'
            if key_ld_library_path not in os.environ or patch_ld_library_path not in os.environ[key_ld_library_path]:
                if key_ld_library_path not in os.environ:
                    os.environ[key_ld_library_path] = patch_ld_library_path
                else:
                    os.environ[key_ld_library_path] += os.pathsep + patch_ld_library_path
                import subprocess, sys
                subprocess.run([sys.executable,  *sys.argv])    
                sys.exit(0)
        
        __config_initialized__ = True
        
