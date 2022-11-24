all: server

clean:
	@rm -rf *.o
	@rm build/server

server: server.o http.o
	g++ -std=c++11 -o build/server $^

server.o: src/server.cc
	g++ -std=c++11 -c -o server.o src/server.cc

http.o: src/http.cc
	g++ -std=c++11 -c -o http.o src/http.cc