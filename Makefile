OS_SUPPORT = 
MonetDB_LIB = 

ifeq ($(OS),Windows_NT)
	OS_SUPPORT += server/winhelper.cpp
	MonetDB_LIB += -Imonetdb/msvc msc-plugin/monetdbe.dll 
endif
$(info $(OS_SUPPORT))

server.bin:
	$(CXX) server/server.cpp $(OS_SUPPORT) --std=c++1z -O3 -march=native -o server.bin
server.so:
#	$(CXX) server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
	$(CXX) -shared -fPIC server/server.cpp $(OS_SUPPORT) server/monetdb_conn.cpp $(MonetDB_LIB) --std=c++1z -o server.so -O3
snippet:
	$(CXX) -shared -fPIC --std=c++1z out.cpp server/monetdb_conn.cpp $(MonetDB_LIB) -O3 -march=native -o dll.so
clean:
	rm *.shm -rf
