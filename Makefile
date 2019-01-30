source =  socket.cc payload.cc acker.cc Sat_server.cc Sat_acker.cc saturateservo.cc 
objects = socket.o payload.o acker.o saturateservo.o 
executables = Sat_server Sat_acker 

CXX = g++
CXXFLAGS = -g -O3 -std=c++0x -ffast-math -pedantic -Werror -Wall -Wextra -Weffc++ -fno-default-inline -pipe -D_FILE_OFFSET_BITS=64 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE
LIBS = -lm -lrt

all: $(executables)

Sat_server: Sat_server.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

Sat_acker: Sat_acker.o $(objects)
	$(CXX) $(CXXFLAGS) -o $@ $+ $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include depend

depend: $(source)
	$(CXX) $(INCLUDES) -MM $(source) > depend

.PHONY: clean
clean:
	-rm -f $(executables) depend *.o *.rpo
