import mo_sql_parsing as parser
q = 'SELECT p.Name, v.Name FROM Production.Product p JOIN Purchasing.ProductVendor pv ON p.ProductID = pv.ProductID JOIN Purchasing.Vendor v ON pv.BusinessEntityID = v.BusinessEntityID WHERE ProductSubcategoryID = 15 ORDER BY v.Name;'

res = parser.parse(q)

print(res)

while True:
    try:
        q = input()
        stmts = parser.parse(q)
        for s in stmts:
            print(s)
    except Exception as e:
        print(e)
