CXX = g++
CXXFLAGS = -I lib/httplib -std=c++23
LDFLAGS = -lwiringPi

main: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o main $(LDFLAGS)