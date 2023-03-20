ifeq ($(PYTHON_EXEC),)
	PYTHON_EXEC=python3
endif
PYTHON_CONFIG=$(PYTHON_EXEC)-config
PYTHON_LDFLAGS=$(shell echo `$(PYTHON_CONFIG) --ldflags` | sed 's/^-L\([^[:space:]]*\).*$$/\1/')/lib$(PYTHON_EXEC)*.a
all:
	rm ../../libdraw.so; \
	$(CXX) --shared app2.cpp --std=c++2a -I../.. -I/usr/local/opt/matplotplusplus/include -L../.. -L/usr/local/opt/matplotplusplus/lib/ -lmatplot -laquery -o ../../libdraw.so
py:
	rm ../../libdraw.so; \
	$(CXX) --shared app.cpp --std=c++2a -I../.. `$(PYTHON_CONFIG) --cflags` -O0 -g3 $(PYTHON_LDFLAGS) ../../libaquery.a -o ../../libdraw.so
exe:
	$(CXX) app.cpp --std=c++2a -I../.. `$(PYTHON_CONFIG) --cflags` -O0 -g3 $(PYTHON_LDFLAGS) ../../libaquery.a -o a.out
info:
	$(info $(PYTHON_LDFLAGS))