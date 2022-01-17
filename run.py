import re
import aquery_parser as parser
import engine

test_parser = True

# code to test parser
ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)

print(res)

while test_parser:
    try:
        q = input()
        if q == 'exec':
            cxt = engine.initialize()
            for s in stmts['stmts']:
                engine.generate(s, cxt)
            print(cxt.k9code)
            with open('out.k', 'wb') as outfile:
                outfile.write(cxt.k9code)
            continue
        trimed = ws.sub(' ', q.lower()).split(' ') 
        if trimed[0] == 'file':
            fn = 'q.sql' if len(trimed) <= 1 or len(trimed[1]) == 0 \
                            else trimed[1]
                
            with open(fn, 'r') as file:
                contents = file.read()
            stmts = parser.parse(contents)
            continue
        stmts = parser.parse(q)
        print(stmts)
    except Exception as e:
        print(type(e), e)

