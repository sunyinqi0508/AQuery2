import re
import mo_sql_parsing as parser

ws = re.compile(r'\s+')

q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)

print(res)

while True:
    try:
        q = input()
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
