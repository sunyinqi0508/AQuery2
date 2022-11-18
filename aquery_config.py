# put environment specific configuration here

## GLOBAL CONFIGURATION FLAGS

version_string = '0.5.4a'
add_path_to_ldpath = True
rebuild_backend = False
run_backend = True
have_hge = False
cygroot = 'c:/msys64/usr/bin'
msbuildroot = ''
os_platform = 'unknown'
build_driver = 'Auto'
compilation_output = True

## END GLOBAL CONFIGURATION FLAGS

def init_config():
    global __config_initialized__, os_platform, msbuildroot, build_driver
## SETUP ENVIRONMENT VARIABLES
    # __config_initialized__ = False
    #os_platform = 'unkown'
    #msbuildroot = 'd:/gg/vs22/MSBuild/Current/Bin'
    import os
    from engine.utils import add_dll_dir
    # os.environ['CXX'] = 'C:/Program Files/LLVM/bin/clang.exe'
    os.environ['THREADING'] = '1'
    os.environ['AQUERY_ITC_USE_SEMPH'] = '1'

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
            add_dll_dir(cygroot)  
            add_dll_dir(os.path.abspath('./msc-plugin'))
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
            import readline
            if build_driver == 'Auto':
                build_driver = 'Makefile'
            if os_platform == 'cygwin':
                add_dll_dir('./lib')
        __config_initialized__ = True
        
