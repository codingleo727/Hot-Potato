CXX = g++
CXXFLAGS = -g -std=c++17 -Wall -Wextra -Werror -pedantic
TARGET = ringmaster player

all: $(TARGET)

ringmaster: ringmaster_main.o ringmaster.o Socket.o potato.o
	$(CXX) $(CXXFLAGS) -o $@ $^

player: player_main.o player.o Socket.o potato.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c $<

%_main.o: %_main.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o

rebuild: clean all