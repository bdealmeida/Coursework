#ifndef MQREQUESTCHANNEL_H
#define MQREQUESTCHANNEL_H

#include "common.h"
#include "RequestChannel.h"

#include <mqueue.h>

class MQRequestChannel : public RequestChannel {
private:
    int open_ipc(string _pipe_name, int mode) {
        int fd = (int) mq_open(_pipe_name.c_str(), O_RDWR | O_CREAT, 0600, 0);
        if (fd < 0)
            EXITONERROR(_pipe_name);
        return fd;
    }

public:
    MQRequestChannel(const string _name, const Side _side) : RequestChannel(_name, _side) {

        streamToClient = "/mq_" + my_name + "1";
        streamToServer = "/mq_" + my_name + "2";

        if (_side == SERVER_SIDE) {
            wfd = open_ipc(streamToClient, O_WRONLY);
            rfd = open_ipc(streamToServer, O_RDONLY);
        } else {
            rfd = open_ipc(streamToClient, O_RDONLY);
            wfd = open_ipc(streamToServer, O_WRONLY);
        }
    }

    ~MQRequestChannel() {
        mq_close(wfd);
        mq_close(rfd);

        mq_unlink(streamToClient.c_str());
        mq_unlink(streamToServer.c_str());
    }

    int cread(void* msgbuf, int bufcapacity) {
        return mq_receive(rfd, (char *) msgbuf, 8192, NULL);
    }

    int cwrite(void* msgbuf, int len) {
        return mq_send(wfd, (char *) msgbuf, len, NULL);
    }
};

#endif
