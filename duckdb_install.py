import urllib.request
import zipfile
from aquery_config import os_platform
from os import remove

version = '0.8.1'
duckdb_os = 'windows' if os_platform == 'windows' else 'osx' if os_platform == 'darwin' else 'linux'

duckdb_plat = 'i386'
if duckdb_os == 'darwin': 
    duckdb_plat = 'universal'
else:
    duckdb_plat = 'amd64'

duckdb_pkg = f'libduckdb-{duckdb_os}-{duckdb_plat}.zip'
# urllib.request.urlretrieve(f"https://github.com/duckdb/duckdb/releases/latest/download/{duckdb_pkg}", duckdb_pkg)
urllib.request.urlretrieve(f"https://github.com/duckdb/duckdb/releases/download/v{version}/{duckdb_pkg}", duckdb_pkg)
with zipfile.ZipFile(duckdb_pkg, 'r') as duck:
	duck.extractall('deps')

remove(duckdb_pkg)
