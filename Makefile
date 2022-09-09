OS_SUPPORT = 
MonetDB_LIB = 
Threading = 
CXXFLAGS = --std=c++1z
OPTFLAGS = -O3 -flto -march=native 
SHAREDFLAGS = -shared -fPIC
ifeq ($(PCH), 1)
	PCHFLAGS = -include server/aggregations.h
else
	PCHFLAGS = 
endif

ifeq ($(OS),Windows_NT)
	NULL_DEVICE = NUL
	OS_SUPPORT += server/winhelper.cpp
	MonetDB_LIB += -Imonetdb/msvc msc-plugin/monetdbe.dll 
else
	UNAME_S = $(shell uname -s)
	UNAME_M = $(shell uname -m)
	NULL_DEVICE = /dev/null
	MonetDB_LIB = 
	ifeq ($(UNAME_S),Darwin)
		MonetDB_LIB += -L`brew --prefix monetdb`/lib 
		ifneq ($(UNAME_M),arm64)
			OPTFLAGS += -march=native
		endif
	else
		OPTFLAGS += -march=native
	endif
	MonetDB_LIB += -I/usr/local/include/monetdb -I/usr/include/monetdb -lmonetdbe
endif

ifeq ($(THREADING),1)
	Threading += server/threading.cpp -DTHREADING
endif
pch:
	$(CXX) -x c++-header server/aggregations.h -o server/aggregations.h.pch
info:
	$(info $(OS_SUPPORT))
	$(info $(OS)) 
	$(info $(Threading))
	$(info "test")
	$(info $(CXX))
libaquery.a:
	$(CXX) -c server/server.cpp server/io.cpp server/table.cpp $(OS_SUPPORT) $(Threading) $(OPTFLAGS) $(CXXFLAGS) -o server.bin
server.bin:
	$(CXX) server/server.cpp server/io.cpp server/table.cpp $(OS_SUPPORT) $(Threading) $(OPTFLAGS) $(CXXFLAGS) -o server.bin
server.so:
#	$(CXX) server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
	$(CXX) $(SHAREDFLAGS) server/server.cpp server/io.cpp server/table.cpp $(OS_SUPPORT) server/monetdb_conn.cpp $(Threading) $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o server.so 
snippet:
	$(CXX) $(SHAREDFLAGS) $(PCHFLAGS) out.cpp server/server.cpp server/monetdb_conn.cpp server/table.cpp server/io.cpp $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o dll.so
docker:
	docker build -t aquery .

clean:
	rm *.shm *.o dll.so server.so server.bin -rf 2> $(NULL_DEVICE) || true


