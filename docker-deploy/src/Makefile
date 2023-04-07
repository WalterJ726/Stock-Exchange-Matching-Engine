CFLAGS=-std=gnu++11 -pedantic -Wall -ggdb3
PROGS=daemon
OBJS=$(patsubst %,%.o,$(PROGS)) *.o
EXTRAFLAGS=-lpqxx -lpq -pthread

all: $(PROGS)
daemon: daemon.cpp Server.cpp Database.cpp
	g++ -g $(CFLAGS) -o daemon daemon.cpp Server.cpp Database.cpp $(EXTRAFLAGS)
	
.PHONY: clean
clean:
	rm -f *~ $(PROGS) $(OBJS)
