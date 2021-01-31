# makefile

all: client

common.o: common.h common.cpp
	g++ -g -w -std=c++11 -c common.cpp

TCPreqchannel.o: TCPreqchannel.h TCPreqchannel.cpp
	g++ -g -w -std=c++11 -c TCPreqchannel.cpp

client: client.cpp TCPreqchannel.o common.o
	g++ -g -w -std=c++11 -o client client.cpp TCPreqchannel.o common.o -lpthread -lrt -fsanitize=address

clean:
	rm -rf *.o client
