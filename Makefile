PORT = 10000

build: httpproxy

httpproxy: myProxy.o helpers.o
	g++ -std=c++11 -Wall -g -pthread myProxy.o helpers.o -o httpproxy

myProxy.o: myProxy.cpp
	g++ -std=c++11 -Wall -g -pthread -c myProxy.cpp

helpers.o: helpers.cpp
	g++ -std=c++11 -Wall -g -c helpers.cpp

run_server:
	./httpproxy ${PORT}

clean:
	-rm -f *.o httpproxy
	-rm -f *.html
	-rm -f cache-*