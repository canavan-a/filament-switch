CXX = g++
CXXFLAGS = -I lib/httplib -std=c++23
LDFLAGS = -lwiringPi

lib/httplib/httplib.h.gch: lib/httplib/httplib.h
	$(CXX) $(CXXFLAGS) lib/httplib/httplib.h

main: lib/httplib/httplib.h.gch main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o main $(LDFLAGS)