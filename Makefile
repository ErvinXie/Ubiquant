CXX = g++
CXXFLAGS = -std=c++17 -Wall -g
LD = g++
LDFLAGS = -std=c++17 -g
HDF5 = /opt/anaconda3/bin/h5c++

all: exchange ubi-read-test

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange.o: exchange.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange: exchange.o network.o
	$(LD) $(LDFLAGS) $^ -o $@

ubi-read-test: ubi-read-test.cpp
	$(HDF5) -o $@ $^ -g 

clean:
	rm -f *.o exchange ubi-read-test 
