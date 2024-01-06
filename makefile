# Define compilers and flags
CXX=g++
CXXFLAGS=-Wall -Wextra -g

# Default target
all: twmailer-client twmailer-server

# Compile client
twmailer-client: twmailer-client.cpp
	$(CXX) $(CXXFLAGS) twmailer-client.cpp -o twmailer-client

# Compile server
twmailer-server: twmailer-server.cpp
	$(CXX) $(CXXFLAGS) twmailer-server.cpp -o twmailer-server

# Clean objects and executables
clean:
	rm -f twmailer-client twmailer-server