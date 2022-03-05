CXX = g++
CXXFLAGS = -std=c++17 -Wall
LD = g++
LDFLAGS = -std=c++17

all: 

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange.o: exchange.cpp network.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

exchange: exchange.o network.o
	$(LD) $(LDFLAGS) $^ -o $@

clean: 
	rm -f *.o exchange
