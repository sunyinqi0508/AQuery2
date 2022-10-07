example:
	$(CXX) -shared -fPIC example.cpp aquery_mem.cpp -fno-semantic-interposition -Ofast -march=native -flto --std=c++1z -o ../test.so
irf:
	$(CXX) -shared -fPIC RF.cpp irf.cpp incrementalDecisionTree.cpp aquery_mem.cpp Evaluation.cpp -fno-semantic-interposition -Ofast -march=native -flto --std=c++1z -o ../libirf.so
all: example
