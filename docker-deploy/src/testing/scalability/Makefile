CFLAGS=-std=gnu++11 -pedantic -Wall -ggdb3
PROGS=testScalability #
OBJS=$(patsubst %,%.o,$(PROGS)) *.o
EXTRAFLAGS=-lpqxx -lpq -pthread

all: $(PROGS)
## DEBUG
testScalability: test_scalability.cpp client.cpp genXml.cpp
	g++ -g $(CFLAGS) -o testScalability test_scalability.cpp client.cpp genXml.cpp -pthread
	

.PHONY: clean
clean:
	rm -f *~ $(PROGS) $(OBJS)
