CXX = g++48
CXXFLAGS = -static-libstdc++ -pthread -Wall -Wextra -std=c++11 -g
LXXFLAGS = -lpthread

SERVER_OBJS = ServerMain.o Server.o
CLIENT_OBJS = ClientMain.o Client.o

RM = rm -rf

TAR = tar czvf
ARCHIVE = xmilko01.tar.gz

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_OBJS) $(LXXFLAGS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_OBJS) $(LXXFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(SERVER_OBJS) $(CLIENT_OBJS) server client

pack:
	$(TAR) $(ARCHIVE) *.cpp *.h Makefile

.PHONY: all server client clean pack
