CFLAGS=-std=gnu++11 -pedantic -Wall -ggdb3
PROGS=daemon testHandleRequest testScalability #
OBJS=$(patsubst %,%.o,$(PROGS)) *.o
EXTRAFLAGS=-lpqxx -lpq -pthread

all: $(PROGS)
daemon: daemon.cpp Server.cpp Database.cpp
	g++ -g $(CFLAGS) -o daemon daemon.cpp Server.cpp Database.cpp $(EXTRAFLAGS)



## DEBUG
testHandleRequest: testHandleRequest.cpp client.cpp
	g++ -g $(CFLAGS) -o testHandleRequest testHandleRequest.cpp client.cpp

testScalability: test_scalability.cpp client.cpp
	g++ -g $(CFLAGS) -o testScalability test_scalability.cpp client.cpp -pthread


testCache: proxy_daemon.cpp ProxyTest.cpp Server.cpp Request.cpp httprequest.cpp client.cpp HttpResponse.cpp Cache.cpp Time.cpp
	g++ -g $(CFLAGS) -o testCache proxy_daemon.cpp ProxyTest.cpp Server.cpp Request.cpp httprequest.cpp client.cpp HttpResponse.cpp Cache.cpp Time.cpp -lpthread

testHttpResponse: testHttpResponse.cpp Server.cpp Request.cpp httprequest.cpp client.cpp HttpResponse.cpp
	g++ -g $(CFLAGS) -o testHttpResponse testHttpResponse.cpp Server.cpp Request.cpp httprequest.cpp client.cpp HttpResponse.cpp
.PHONY: clean
clean:
	rm -f *~ $(PROGS) $(OBJS)
