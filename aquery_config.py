# put environment specific configuration here

import os
# os.environ['CXX'] = 'C:/Program Files/LLVM/bin/clang.exe'
os.environ['THREADING'] = '0'

add_path_to_ldpath = True
rebuild_backend = True
run_backend = True
have_hge = False

os_platform = 'unkown'

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


# deal with msys dependencies:
if os_platform == 'win':
    os.add_dll_directory('c:/msys64/usr/bin')  
    os.add_dll_directory(os.path.abspath('./msc-plugin'))
    print("adding path")
else:
    import readline
    