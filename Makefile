snippet:
	g++ -shared --std=c++1z out.cpp -O3 -march=native -o dll.so
clean:
	rm *.shm -rf