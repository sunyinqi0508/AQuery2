import re
import aquery_parser as parser
import engine
import subprocess

import sys
if sys.platform != 'win32':
    import readline
    basecmd = ['bash', '-c', 'rlwrap k']
else:
    basecmd = ['bash.exe', '-c', 'rlwrap ./k']

test_parser = True

# code to test parser
ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)

print(res)

keep = False
cxt = None
while test_parser:
    try:
        q = input()
        if q == 'exec':
            if not keep or cxt is None:
                cxt = engine.initialize()
            stmts_stmts = stmts['stmts']
            if type(stmts_stmts) is list:
                for s in stmts_stmts:
                    engine.generate(s, cxt)
            else:
                engine.generate(stmts_stmts, cxt)
            print(cxt.k9code)
            with open('out.k', 'wb') as outfile:
                outfile.write((cxt.k9code+'\n\\\\').encode('utf-8'))
            subprocess.call(basecmd[:-1] + [basecmd[-1] + ' out.k'])
            continue
        elif q == 'k':
            subprocess.call(basecmd)
            continue
        elif q == 'print':
            print(stmts)
            continue
        elif q == 'keep':
            keep = not keep
            continue
        trimed = ws.sub(' ', q.lower()).split(' ') 
        if trimed[0].startswith('f'):
            fn = 'stock.a' if len(trimed) <= 1 or len(trimed[1]) == 0 \
                            else trimed[1]
                
            with open(fn, 'r') as file:
                contents = file.read()
            stmts = parser.parse(contents)
            continue
        stmts = parser.parse(q)
        print(stmts)
    except (ValueError) as e:
        print(type(e), e)

