OPT_FLASG = 
ifneq ($(DEBUG), 1)
	OPT_FLAGS = -Ofast -march=native -flto -DNDEBUG 
else 
	OPT_FLAGS = -g3 -D_DEBUG -fsanitize=leak -fsanitize=address
endif
example:
	$(CXX) -shared -fPIC example.cpp aquery_mem.cpp -fno-semantic-interposition -Ofast -march=native -flto --std=c++1z -o ../test.so
irf:
	$(CXX) -shared -fPIC RF.cpp irf.cpp incrementalDecisionTree.cpp aquery_mem.cpp Evaluation.cpp -fno-semantic-interposition $(OPT_FLAGS) --std=c++1z -o ../libirf.so
all: example
