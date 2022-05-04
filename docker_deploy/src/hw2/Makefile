all: main

main: main.cpp server.cpp client.cpp parser.cpp cache.cpp
	g++ -g -o main main.cpp server.cpp client.cpp parser.cpp cache.cpp -lpthread

.PHONY: clean

clean:
	rm -f main