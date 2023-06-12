import os

payload = ('''\
(C) Bill Sun 2022 - 2023
All rights reserved. (or some other license stuff)
''' ).strip().split('\n')

comment_factory = lambda mark, enclosure = '': (f'''\
{enclosure}{mark}
{mark} {f'{chr(10)}{mark} '.join(payload)}
{mark}{enclosure}\n
''' ).encode()

py_payload = comment_factory('#')
c_payload = comment_factory('*', '/')

curr = ['.']
while curr:
    next = []
    for dir in curr:
        items = os.listdir(dir)
        for file in items:
            fullpath = f'{dir}{os.sep}{file}'
            if os.path.isdir(fullpath):
                next.append(fullpath)
            else:
                def write_to_file(payload: str):
                    with open(fullpath, 'rb+') as f:
                        content = f.read()
                        if not content.startswith(payload):
                            f.seek(0)
                            f.write(payload + content)
                            print('processed', fullpath)
                        else:
                            print('not processed', fullpath)
                            
                if (
                        file.lower() == 'makefile' or 
                        file.lower() == 'dockerfile' or 
                        '.' in file and 
                        file[file.rfind('.') + 1:].lower() 
                            in 
                        ['py', 'sh']
                    ):
                    write_to_file(py_payload)
                elif (
                        '.' in file and 
                        file[file.rfind('.') + 1:].lower() 
                            in 
                        ['cc', 'c', 'cpp', 'cxx', 'hpp', 'h']
                    ):
                    write_to_file(c_payload)
    curr = next
