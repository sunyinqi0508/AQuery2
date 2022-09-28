OS_SUPPORT = 
MonetDB_LIB = 
MonetDB_INC = 
Threading = 
CXXFLAGS = --std=c++1z
OPTFLAGS = -O3 -DNDEBUG 
LINKFLAGS = -flto # + $(AQ_LINK_FLAG)
SHAREDFLAGS = -shared  
FPIC = -fPIC
COMPILER = $(shell $(CXX) --version | grep -q clang && echo clang|| echo gcc) 
LIBTOOL = ar rcs
USELIB_FLAG = -Wl,--whole-archive,libaquery.a -Wl,-no-whole-archive
LIBAQ_SRC = server/server.cpp server/monetdb_conn.cpp server/io.cpp 
LIBAQ_OBJ = server.o monetdb_conn.o io.o 
SEMANTIC_INTERPOSITION = -fno-semantic-interposition
RANLIB = ranlib

ifeq ($(COMPILER), clang )
	CLANG_GE_10 = $(shell expr `$(CXX) -dumpversion | cut -f1 -d.` \>= 10)
	ifneq ($(CLANG_GE_10), 1)
		SEMANTIC_INTERPOSITION = 
	endif
	ifneq (, $(shell which llvm-ranlib))
		RANLIB = llvm-ranlib
	endif
else 
	RANLIB = echo
	LIBTOOL = ar rcs
	ifneq (, $(shell which gcc-ar))
		LIBTOOL = gcc-ar rcs
	endif
endif
OPTFLAGS += $(SEMANTIC_INTERPOSITION)

ifeq ($(PCH), 1)
	PCHFLAGS = -include server/pch.hpp
else
	PCHFLAGS = 
endif

ifeq ($(OS),Windows_NT)
	NULL_DEVICE = NUL
	OS_SUPPORT += server/winhelper.cpp
	LIBAQ_OBJ += winhelper.o
	MonetDB_LIB += msc-plugin/monetdbe.dll 
	MonetDB_INC +=  -Imonetdb/msvc
	LIBTOOL = gcc-ar rcs
	ifeq ($(COMPILER), clang )
		FPIC =
	endif
else
	UNAME_S = $(shell uname -s)
	UNAME_M = $(shell uname -m)
	NULL_DEVICE = /dev/null
	MonetDB_LIB = 
	ifeq ($(UNAME_S),Darwin)
		USELIB_FLAG = -Wl,-force_load
		MonetDB_LIB += -L$(shell brew --prefix monetdb)/lib 
		MonetDB_INC += -I$(shell brew --prefix monetdb)/include/monetdb
		ifeq ($(COMPILER), clang )
			LIBTOOL = libtool -static -o
		endif
		ifneq ($(UNAME_M),arm64)
			OPTFLAGS += -march=native
		endif
	else
		OPTFLAGS += -march=native
		MonetDB_LIB += $(AQ_MONETDB_LIB)
		MonetDB_INC += $(AQ_MONETDB_INC)
		MonetDB_INC += -I/usr/local/include/monetdb -I/usr/include/monetdb 
	endif
	MonetDB_LIB += -lmonetdbe
endif

ifeq ($(THREADING),1)
	LIBAQ_SRC += server/threading.cpp
	LIBAQ_OBJ += threading.o
	Threading +=  -DTHREADING
endif

SHAREDFLAGS += $(FPIC)

info:
	$(info $(OPTFLAGS))
	$(info $(OS_SUPPORT))
	$(info $(OS)) 
	$(info $(Threading))
	$(info "test")
	$(info $(LIBTOOL))
	$(info $(MonetDB_INC))
	$(info $(COMPILER))
	$(info $(CXX))
	$(info $(FPIC))
pch:
	$(CXX) -x c++-header server/pch.hpp $(FPIC) $(MonetDB_INC) $(OPTFLAGS) $(CXXFLAGS) $(Threading)
libaquery.a:
	$(CXX) -c $(FPIC) $(PCHFLAGS) $(LIBAQ_SRC) $(MonetDB_INC) $(MonetDB_LIB) $(OS_SUPPORT) $(Threading) $(OPTFLAGS) $(LINKFLAGS) $(CXXFLAGS) &&\
	$(LIBTOOL) libaquery.a $(LIBAQ_OBJ) &&\
	$(RANLIB) libaquery.a

server.bin:
	$(CXX) $(LIBAQ_SRC) $(LINKFLAGS) $(OS_SUPPORT) $(Threading)  $(MonetDB_INC) $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o server.bin
launcher:
	$(CXX) -D__AQ_BUILD_LAUNCHER__ $(LIBAQ_SRC) $(LINKFLAGS) $(OS_SUPPORT) $(Threading)  $(MonetDB_INC) $(MonetDB_LIB) $(OPTFLAGS) $(CXXFLAGS) -o aq
server.so:
#	$(CXX) -z muldefs server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
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
	rm .cached *.shm *.o dll.so server.so server.bin libaquery.a libaquery.lib -rf 2> $(NULL_DEVICE) || true


