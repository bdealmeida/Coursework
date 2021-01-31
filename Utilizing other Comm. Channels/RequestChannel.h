
#ifndef RequestChannel_H
#define RequestChannel_H

#include "common.h"

class RequestChannel {
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};
	enum Mode {READ_MODE, WRITE_MODE};
protected:
    string my_name, streamToServer, streamToClient;
    Side my_side;
    int wfd, rfd;

    virtual int open_ipc(string _pipe_name, int mode) {};
public:
    // constructor/Deconstructor, implemented by each specific class
    RequestChannel(const string _name, const Side _side): my_name(_name), my_side(_side) {}
	virtual ~RequestChannel() {}

    // Returns # of chars read/written respectively, or -1 on failure
	virtual int cread(void* msgbuf, int bufcapacity) = 0;
	virtual int cwrite(void* msgbuf, int msglen) = 0;

    string name() { return my_name; } 
};

#endif
