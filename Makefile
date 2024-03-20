CPPFLAGS = -Wall -Werror -Wextra -std=c++17

all:
	g++ $(CPPFLAGS) archivator.cpp

clean:
	rm -rf *.out