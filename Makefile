OS_SUPPORT = 

ifeq ($(OS),Windows_NT)
	OS_SUPPORT += server/winhelper.cpp
endif
$(info $(OS_SUPPORT))

server.bin:
	g++ server/server.cpp $(OS_SUPPORT) --std=c++1z -O3 -march=native -o server.bin
server.so:
	g++ server/server.cpp -shared $(OS_SUPPORT) --std=c++1z -O3 -march=native -o server.so
snippet:
	g++ -shared -fPIC --std=c++1z out.cpp -O3 -march=native -o dll.so
clean:
	rm *.shm -rf