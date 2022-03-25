all:
	g++ mmw.cpp --std=c++1z -shared -fPIC -Ofast -march=native -g0 -s -o mmw.so
avx512:
	g++ mmw.cpp --std=c++1z -shared -fPIC -Ofast -mavx512f -g0 -s -o mmw.so
debug:
	g++ mmw.cpp --std=c++1z -shared -fPIC -O0 -march=native -g3 -o mmw.so
clean:
	rm  mmw.so -rf
