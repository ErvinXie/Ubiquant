CXX = g++
CXXFLAGS = --std=c++17

all: 

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

clean: 
	rm -f *.o
