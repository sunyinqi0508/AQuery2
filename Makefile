OS_SUPPORT = 
MonetDB_LIB = 
Threading = 

ifeq ($(OS),Windows_NT)
	OS_SUPPORT += server/winhelper.cpp
	MonetDB_LIB += -Imonetdb/msvc msc-plugin/monetdbe.dll 
else
	MonetDB_LIB += -I/usr/local/include/monetdb -I/usr/include/monetdb -lmonetdbe
endif

ifeq ($(THREADING),1)
	Threading += server/threading.cpp -DTHREADING
endif

info:
	$(info $(OS_SUPPORT))
	$(info $(OS)) 
	$(info $(Threading))
	$(info "test")
server.bin:
	$(CXX) server/server.cpp server/io.cpp server/table.cpp $(OS_SUPPORT) $(Threading) -flto --std=c++1z -O3 -march=native -o server.bin
server.so:
#	$(CXX) server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
	$(CXX) -shared -fPIC -flto server/server.cpp server/io.cpp server/table.cpp $(OS_SUPPORT) server/monetdb_conn.cpp $(Threading) $(MonetDB_LIB)  --std=c++1z -o server.so -O3
snippet:
	$(CXX) -shared -fPIC -flto --std=c++1z -include server/aggregations.h out.cpp server/monetdb_conn.cpp server/table.cpp server/io.cpp $(MonetDB_LIB) -O3 -march=native -o dll.so
clean:
	rm *.shm -rf
