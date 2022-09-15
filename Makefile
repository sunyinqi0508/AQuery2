OS_SUPPORT = 
MonetDB_LIB = 
MonetDB_INC = 
Threading = 
CXXFLAGS = --std=c++1z
OPTFLAGS = -g3 #-O3 -fno-semantic-interposition
LINKFLAGS = -flto
SHAREDFLAGS = -shared  
FPIC = -fPIC
COMPILER = $(shell $(CXX) --version | grep -q 'clang' && echo "clang"|| echo "gcc") 
LIBTOOL = 
USELIB_FLAG = -Wl,--whole-archive,libaquery.a -Wl,-no-whole-archive
LIBAQ_SRC = server/server.cpp server/monetdb_conn.cpp server/io.cpp 

ifeq ($(PCH), 1)
	PCHFLAGS = -include server/pch.hpp
else
	PCHFLAGS = 
endif

ifeq ($(OS),Windows_NT)
	NULL_DEVICE = NUL
	OS_SUPPORT += server/winhelper.cpp
	MonetDB_LIB += msc-plugin/monetdbe.dll 
	MonetDB_INC +=  -Imonetdb/msvc
	LIBTOOL = gcc-ar rcs
else
	UNAME_S = $(shell uname -s)
	UNAME_M = $(shell uname -m)
	NULL_DEVICE = /dev/null
	MonetDB_LIB = 
	LIBTOOL = ar rcs
	ifeq ($(UNAME_S),Darwin)
		USELIB_FLAG = -Wl,-force_load
		MonetDB_LIB += -L$(shell brew --prefix monetdb)/lib 
		MonetDB_INC += -I$(shell brew --prefix monetdb)/include/monetdb
		ifeq ($(COMPILER), clang)
			LIBTOOL = libtool -static -o
		endif
		ifneq ($(UNAME_M),arm64)
			OPTFLAGS += -march=native
		endif
	else
		OPTFLAGS += -march=native
		MonetDB_INC += -I/usr/local/include/monetdb -I/usr/include/monetdb 
	endif
	MonetDB_LIB += -lmonetdbe
endif

ifeq ($(THREADING),1)
	LIBAQ_SRC += server/threading.cpp
	Threading +=  -DTHREADING
endif

SHAREDFLAGS += $(FPIC)

info:
	$(info $(OS_SUPPORT))
	$(info $(OS)) 
	$(info $(Threading))
	$(info "test")
	$(info $(LIBTOOL))
	$(info $(MonetDB_INC))
	$(info $(CXX))
pch:
	$(CXX) -x c++-header server/pch.hpp $(FPIC) $(MonetDB_INC) $(OPTFLAGS) $(CXXFLAGS) $(Threading)
libaquery.a:
	$(CXX) -c $(FPIC) $(PCHFLAGS) $(LIBAQ_SRC) $(MonetDB_INC) $(MonetDB_LIB) $(OS_SUPPORT) $(Threading) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) &&\
	$(LIBTOOL) libaquery.a *.o &&\
	ranlib libaquery.a

server.bin:
	$(CXX) $(LIBAQ_SRC) $(LINKFLAGS) $(OS_SUPPORT) $(Threading)  $(MonetDB_INC) $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o server.bin
launcher:
	$(CXX) -D__AQ_BUILD_LAUNCHER__ $(LIBAQ_SRC) $(LINKFLAGS) $(OS_SUPPORT) $(Threading)  $(MonetDB_INC) $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o aq
server.so:
#	$(CXX) server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
	$(CXX) $(SHAREDFLAGS) $(PCHFLAGS) $(LIBAQ_SRC) $(OS_SUPPORT) $(Threading) $(MonetDB_INC) $(MonetDB_LIB) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) -o server.so 
server_uselib:
	$(CXX) $(SHAREDFLAGS) $(USELIB_FLAG),libaquery.a $(MonetDB_LIB) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) -o server.so

snippet:
	$(CXX) $(SHAREDFLAGS) $(PCHFLAGS) out.cpp $(LIBAQ_SRC) $(MonetDB_INC) $(MonetDB_LIB) $(Threading) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) -o dll.so
snippet_uselib:
	$(CXX) $(SHAREDFLAGS) $(PCHFLAGS) out.cpp libaquery.a $(MonetDB_INC) $(Threading) $(MonetDB_LIB) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) -o dll.so

docker:
	docker build -t aquery .

clean:
	rm *.shm *.o dll.so server.so server.bin libaquery.a .cached -rf 2> $(NULL_DEVICE) || true


