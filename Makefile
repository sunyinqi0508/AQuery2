OS_SUPPORT = 
MonetDB_LIB = 
MonetDB_INC = 
Defines = 
CC = $(CXX) -xc 
CXXFLAGS = --std=c++2a

ifdef AQ_LINKER
	CXX += -fuse-ld=$(AQ_LINKER)
endif

SHAREDFLAGS = -shared  
FPIC = -fPIC
_COMPILER = $(shell $(CXX) --version | grep -q clang && echo clang|| echo gcc) 
COMPILER = $(strip $(_COMPILER))
LIBTOOL = ar rcs
USELIB_FLAG = -Wl,--whole-archive,libaquery.a -Wl,-no-whole-archive
LIBAQ_SRC = server/monetdb_conn.cpp server/duckdb_conn.cpp server/libaquery.cpp 
LIBAQ_OBJ = monetdb_conn.o duckdb_conn.o libaquery.o monetdb_ext.o
SEMANTIC_INTERPOSITION = -fno-semantic-interposition
RANLIB = ranlib
_LINKER_BINARY = $(shell `$(CXX) -print-prog-name=ld` -v 2>&1 | grep -q LLVM && echo lld || echo ld)
LINKER_BINARY = $(strip $(_LINKER_BINARY))
DuckDB_LIB = -Ldeps -lduckdb
DuckDB_INC = -Ideps

ifeq ($(LINKER_BINARY), ld)
	LINKER_FLAGS = -Wl,--allow-multiple-definition
else
	LINKER_FLAGS =
endif

ifeq ($(COMPILER), clang)
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
LINKFLAGS = $(SEMANTIC_INTERPOSITION)

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
	ifeq ($(COMPILER), clang)
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
		ifeq ($(COMPILER), clang)
			LIBTOOL = libtool -static -o
		endif
		ifneq ($(UNAME_M),arm64)
			OPTFLAGS = -march=native
		endif
	else
		OPTFLAGS = -march=native
		MonetDB_LIB += $(AQ_MONETDB_LIB)
		MonetDB_INC += $(AQ_MONETDB_INC)
		MonetDB_INC += -I/usr/local/include/monetdb -I/usr/include/monetdb 
	endif
	MonetDB_LIB += -lmonetdbe -lmonetdbsql -lbat
endif


ifeq ($(AQ_DEBUG), 1)
	OPTFLAGS = -g3 #-static-libsan -fsanitize=address 
#	LINKFLAGS = 
else
	OPTFLAGS += -Ofast -DNDEBUG -fno-stack-protector 
	LINKFLAGS += -flto -s
endif

ifeq ($(THREADING),1)
	LIBAQ_SRC += server/threading.cpp
	LIBAQ_OBJ += threading.o
	Defines +=  -DTHREADING
endif

ifeq ($(AQUERY_ITC_USE_SEMPH), 1)
	Defines += -D__AQUERY_ITC_USE_SEMPH__
endif

CXXFLAGS += $(OPTFLAGS) $(Defines) $(MonetDB_INC) $(DuckDB_INC) 
BINARYFLAGS = $(CXXFLAGS) $(LINKFLAGS) $(MonetDB_LIB) $(DuckDB_LIB)
SHAREDFLAGS += $(FPIC) $(BINARYFLAGS)

info:
	$(info This makefile script is used in AQuery to automatically build required libraries and executables.)
	$(info Run it manually only for debugging purposes.)
	$(info Targets (built by `make <target>`):)
	$(info $"	pch: generate precompiled header)
	$(info $"	libaquery.a: build static library)
	$(info $"	server.so: build execution engine)
	$(info $"	snippet: build generated query snippet)
	$(info $"	server_uselib: build execution engine using shared library and pch)
	$(info $"	snippet_uselib: build generated query snippet using shared library and pch)
	$(info $"	docker: build docker image with name aquery)
	$(info $"	launcher: build launcher for aquery ./aq)
	$(info $"	clean: remove all generated binaraies and caches)
	$(info )
	$(info Variables:)
	$(info $"	OPTFLAGS: $(OPTFLAGS))
	$(info $"	OS_SUPPORT: $(OS_SUPPORT))
	$(info $"	OS: $(OS)) 
	$(info $"	Defines: $(Defines))
	$(info $"	LIBTOOL: $(LIBTOOL))
	$(info $"	MonetDB_INC: $(MonetDB_INC))
	$(info $"	COMPILER: $(COMPILER))
	$(info $"	CXX: $(CXX))
	$(info $"	LINKER_BINARY: $(LINKER_BINARY))
	$(info $"	LINKER_FLAGS: $(LINKER_FLAGS))
pch:
	$(CXX) -x c++-header server/pch.hpp $(FPIC) $(CXXFLAGS)
libaquery:
	$(CXX) -c $(FPIC) $(PCHFLAGS) $(LIBAQ_SRC) $(OS_SUPPORT) $(CXXFLAGS) &&\
	$(CC) -c $(FPIC) server/monetdb_ext.c $(OPTFLAGS) $(MonetDB_INC) &&\
	$(LIBTOOL) libaquery.a $(LIBAQ_OBJ) &&\
	$(RANLIB) libaquery.a

warmup:
	$(CXX)  msc-plugin/dummy.cpp libaquery.a $(SHAREDFLAGS) -o dll.so
server.bin:
	$(CXX) $(LIBAQ_SRC) $(OS_SUPPORT) $(BINARYFLAGS) -o server.bin
launcher:
	$(CXX) -D__AQ_BUILD_LAUNCHER__ server/server.cpp $(LIBAQ_SRC) $(OS_SUPPORT) $(BINARYFLAGS) -o aq
server.so:
#	$(CXX) -z muldefs server/server.cpp server/monetdb_conn.cpp -fPIC -shared $(OS_SUPPORT) monetdb/msvc/monetdbe.dll --std=c++1z -O3 -march=native -o server.so -I./monetdb/msvc 
	$(CXX) $(PCHFLAGS) $(LIBAQ_SRC) server/server.cpp $(OS_SUPPORT) $(SHAREDFLAGS) -o server.so 
server_uselib:
	$(CXX) $(LINKER_FLAGS) server/server.cpp libaquery.a $(SHAREDFLAGS) -o server.so

snippet:
	$(CXX) $(PCHFLAGS) out.cpp $(LIBAQ_SRC) $(SHAREDFLAGS) -o dll.so
snippet_uselib:
	$(CXX) $(PCHFLAGS) out.cpp libaquery.a $(SHAREDFLAGS) -o dll.so

docker:
	docker build -t aquery .

clean:
	rm .cached *.shm *.o dll.so server.so server.bin libaquery.a libaquery.lib -rf 2> $(NULL_DEVICE) || true; \
	rm -rf *.dSYM || true

.PHONY: clean
