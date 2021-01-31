#ifndef SHMREQUESTCHANNEL_H
#define SHMREQUESTCHANNEL_H

#include "common.h"
#include "RequestChannel.h"

#include <semaphore.h>
#include <string>
#include <sys/mman.h>


class SHMRequestChannel : public RequestChannel {
private:
    int len;
    int fd;
    char* segment;
    sem_t* srd;
    sem_t* swd;
    sem_t* crd;
    sem_t* cwd;

public:
    SHMRequestChannel(const string _name, const Side _side, int _len) : RequestChannel(_name, _side) {
        len = _len;

        // Open and map shared memory segment
        fd = shm_open(("/shm_" + my_name).c_str(), O_RDWR | O_CREAT, 0600);
        ftruncate(fd, len);
        segment = (char *) mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        // Open semaphones
        srd = sem_open(("/Ssem_" + my_name + "_rd").c_str(), O_CREAT, 0600, 1);
        swd = sem_open(("/Ssem_" + my_name + "_wd").c_str(), O_CREAT, 0600, 0);
        crd = sem_open(("/Csem_" + my_name + "_rd").c_str(), O_CREAT, 0600, 1);
        cwd = sem_open(("/Csem_" + my_name + "_wd").c_str(), O_CREAT, 0600, 0);
        //string s1 = "/shm_" + my_name + "1";
        //string s2 = "/shm_" + my_name + "2";

        //shm1 = new SHMQ(s1, len);
        //shm2 = new SHMQ(s2, len);

        //if (_side == CLIENT_SIDE)
        //    swap(shm1, shm2);
    }

    ~SHMRequestChannel() {
        // close and unlink semaphores
        sem_close(srd);
        sem_close(swd);
        sem_close(crd);
        sem_close(cwd);
        sem_unlink(("/Ssem_" + my_name + "_rd").c_str());
        sem_unlink(("/Ssem_" + my_name + "_wd").c_str());
        sem_unlink(("/Csem_" + my_name + "_rd").c_str());
        sem_unlink(("/Csem_" + my_name + "_wd").c_str());

        // Unmap segment and deallocate
        munmap(segment, len);
        shm_unlink(("/shm_" + my_name).c_str());
        close(fd);
        //delete shm1;
        //delete shm2;
    }

    int cread(void* msgbuf, int bufcapacity) {
        //cout << "waiting on write...";
        sem_wait(my_side == SERVER_SIDE ? cwd : swd);
        //cout << "reading...";
        memcpy(msgbuf, segment, bufcapacity);
        //cout << "updating semaphore...";
        sem_post(my_side == SERVER_SIDE ? srd : crd);
        //cout << "done." << endl;
        return len;
    }

    int cwrite(void* msgbuf, int msglen) {
        //cout << "waiting on read...";
        sem_wait(my_side == SERVER_SIDE ? crd : srd);
        //cout << "writing...";
        memcpy(segment, msgbuf, msglen);
        //cout << "updating semaphore...";
        sem_post(my_side == SERVER_SIDE ? swd : cwd);
        //cout << "done." << endl;
        return len;
    }
};

#endif
