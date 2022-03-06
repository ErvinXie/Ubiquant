CXX = g++
CXXFLAGS = -std=c++17 -Wall -ggdb -fsanitize=undefined,thread
LD = g++
LDFLAGS = $(CXXFLAGS)
HDF5 = /opt/anaconda3/bin/h5c++

all: exchange ubi-read-test persister-test

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange.o: exchange.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange: exchange.o network.o
	$(LD) $(LDFLAGS) $^ -o $@

ubi-read-test: ubi-read-test.cpp
	$(HDF5) -o $@ $^ 

persister-test: persister.cpp
	${HDF5} -o $@ $^ -g

clean:
	rm -f *.o exchange ubi-read-test persister-test
