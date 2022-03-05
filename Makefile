CXX = g++
CXXFLAGS = --std=c++17
HDF5 = /opt/anaconda3/bin/h5c++

all: ubi-read-test

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

ubi-read-test: ubi-read-test.cpp
	$(HDF5) -o $@ $^ -g -O0

clean:
	rm -f *.o ubi-read-test




