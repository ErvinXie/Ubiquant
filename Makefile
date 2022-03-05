
CXX = g++
HDF5 = /opt/anaconda3/bin/h5c++

all: ubi-read-test

ubi-read-test: ubi-read-test.cpp
	$(HDF5) -o $@ $^ -g -O0

clean:
	rm -f *.o ubi-read-test
