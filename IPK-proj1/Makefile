CXX = g++48
FLAGS = -static-libstdc++ -Wall -Wextra -std=c++11 -O2
SRCS = main.cpp FtpSession.cpp Socket.cpp
BIN = ftpclient

all:
	$(CXX) $(FLAGS) -o $(BIN) $(SRCS)

clean:
	rm -f $(BIN)

pack:
	tar czvf xmilko01.tar.gz *.cpp *.h Makefile
