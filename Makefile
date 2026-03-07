CXX = g++
CXXFLAGS = -I lib/httplib -std=c++23 -DCPPHTTPLIB_COMPILE
LDFLAGS = -lwiringPi

httplib.o: lib/httplib/httplib.cc
	$(CXX) $(CXXFLAGS) -c lib/httplib/httplib.cc -o httplib.o

main: main.cpp httplib.o
	$(CXX) $(CXXFLAGS) main.cpp httplib.o -o main $(LDFLAGS)