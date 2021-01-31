# makefile

all: server client

common.o: common.h common.cpp
	g++ -g -w -std=c++11 -c common.cpp

client: client.cpp RequestChannel.h FIFOreqchannel.h common.o
	g++ -g -w -std=c++11 -o client client.cpp RequestChannel.h FIFOreqchannel.h MQreqchannel.h SHMreqchannel.h common.o -lpthread -lrt -fsanitize=address

server: server.cpp RequestChannel.h FIFOreqchannel.h common.o
	g++ -g -w -std=c++11 -o server server.cpp RequestChannel.h FIFOreqchannel.h MQreqchannel.h SHMreqchannel.h common.o -lpthread -lrt -fsanitize=address

clean:
	rm -rf *.o *.csv fifo_* server client data*_*
