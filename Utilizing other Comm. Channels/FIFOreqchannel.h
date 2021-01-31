
#ifndef FIFOreqchannel_H
#define FIFOreqchannel_H

#include "RequestChannel.h"
#include "common.h"

class FIFORequestChannel : public RequestChannel {
private:
	int open_ipc(string _pipe_name, int mode) {
		mkfifo (_pipe_name.c_str(), 0600);
		int fd = open(_pipe_name.c_str(), mode);
		if (fd < 0)
			EXITONERROR(_pipe_name);
		return fd;
	}
	
public:
	FIFORequestChannel(const string _name, const Side _side) : RequestChannel(_name, _side) {
		streamToClient = "fifo_" + my_name + "1";
		streamToServer = "fifo_" + my_name + "2";

		if (_side == SERVER_SIDE) {
			wfd = open_ipc(streamToClient, O_WRONLY);
			rfd = open_ipc(streamToServer, O_RDONLY);
		} else {
			rfd = open_ipc(streamToClient, O_RDONLY);
			wfd = open_ipc(streamToServer, O_WRONLY);
		}
	}

	~FIFORequestChannel() { 
		close(wfd);
		close(rfd);

		remove(streamToClient.c_str());
		remove(streamToServer.c_str());
	}

	int cread(void* msgbuf, int bufcapacity) {
		return read (rfd, msgbuf, bufcapacity); 
	}
	
	int cwrite(void* msgbuf, int len) {
		return write (wfd, msgbuf, len);
	}
};

#endif
