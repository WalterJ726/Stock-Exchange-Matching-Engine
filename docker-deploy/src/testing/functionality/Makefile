CFLAGS=-std=gnu++11 -pedantic -Wall -ggdb3
PROGS=testHandleRequest
OBJS=$(patsubst %,%.o,$(PROGS)) *.o
EXTRAFLAGS=-lpqxx -lpq -pthread

all: $(PROGS)
testHandleRequest: testHandleRequest.cpp client.cpp
	g++ -g $(CFLAGS) -o testHandleRequest testHandleRequest.cpp client.cpp

.PHONY: clean
clean:
	rm -f *~ $(PROGS) $(OBJS)
