.PHONY: clean

all: histgen tickgen

clean:
	rm -rf *.o histgen tickgen
	
%.o: %.C
	g++-12 -Ofast -march=native -g -c $<

tickgen: cal.o Time.o tickgen.o
	g++-12 -lstdc++ -Ofast -march=native -flto -o tickgen cal.o Time.o tickgen.o

histgen: cal.o histgen.o 
	g++-12 -lstdc++ -Ofast -flto -march=native -o histgen cal.o histgen.o

timetest: Time.o timetest.o
	g++-12 -lstdc++ -g -o timetest Time.o timetest.o


