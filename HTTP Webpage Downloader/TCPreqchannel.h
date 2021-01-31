#ifndef TCPREQCHANNEL_H
#define TCPREQCHANNEL_H

#include "common.h"
#include <sys/socket.h>
#include <netdb.h>

class TCPRequestChannel {
private:
    int sockfd;

public:
    /* Since a TCP socket is full-duplex, we need only one.
	This is unlike FIFO that needed one read fd and another
    for write from each side  */
    TCPRequestChannel(const string host_name, const string port_no);

    /* Constructor takes 2 arguments: hostname and port not
	If the host name is an empty string, set up the channel for
    the server side. If the name is non-empty, the constructor
	works for the client side. Both constructors prepare the
	sockfd  in the respective way so that it can works as a
    server or client communication endpoint*/
    TCPRequestChannel(int);

    /* Deconstructor */
    ~TCPRequestChannel();

    int cread(void* msgbuf, int buflen);
    int cwrite(void* msgbuf, int msglen);

    /* For adding socket to epoll watch list */
    int getfd();
};

#endif
