#include "TCPreqchannel.h"

TCPRequestChannel::TCPRequestChannel(const string host_name, const string port_no) {
    struct addrinfo hints, *res;

    // load address
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;
    // getaddrinfo("www.example.com", "8080", &hints, &res
    if((status = getaddrinfo(host_name.c_str(), port_no.c_str(), &hints, &res)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << endl;
        exit(-1);
    }

    // make socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd < 0) {
        perror("Cannot create socket");
        exit(-1);
    }

    // connect socket
    if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Cannot connect");
        exit(-1);
    }

    freeaddrinfo(res);
//    cout << "Connected to " << host_name << endl;
}

TCPRequestChannel::TCPRequestChannel(int fd): sockfd(fd) {}

TCPRequestChannel::~TCPRequestChannel() {
    close(sockfd);
}

int TCPRequestChannel::cread(void* msgbuf, int buflen) {
    return recv(sockfd, msgbuf, buflen, 0);
}

int TCPRequestChannel::cwrite(void* msgbuf, int msglen) {
    return send(sockfd, msgbuf, msglen, 0);
}

int TCPRequestChannel::getfd() {
    return sockfd;
}
