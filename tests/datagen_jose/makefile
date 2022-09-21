.PHONY: clean

all: histgen tickgen

clean:
	rm -rf *.o histgen tickgen
	
%.o: %.cpp
	$(CXX) -Ofast -march=native -g -c $<

tickgen: cal.o Time.o tickgen.o
	$(CXX) -lstdc++ -Ofast -march=native -flto -o tickgen cal.o Time.o tickgen.o

histgen: cal.o histgen.o 
	$(CXX) -lstdc++ -Ofast -flto -march=native -o histgen cal.o histgen.o

timetest: Time.o timetest.o
	$(CXX) -lstdc++ -g -o timetest Time.o timetest.o


