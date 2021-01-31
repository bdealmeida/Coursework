#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
#pragma ide diagnostic ignored "cert-msc51-cpp"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#include <thread>
#include <sys/epoll.h>
#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
using namespace std;


void patient_thread_function(int n, int patientNum, BoundedBuffer* reqBuf) {
    datamsg d(patientNum, 0, 1);
    for(int i = 0; i < n; i++) {
//        cout << "Pushing datamsg " << i << " for patient " << patientNum << endl;
        reqBuf->push((char*) &d, sizeof(datamsg));
        d.seconds += 0.004;
    }
}

void event_polling_function(BoundedBuffer* reqBuf, FIFORequestChannel* chans[], HistogramCollection* hc, int mb, int w){
    char buf[mb];
    char recvBuf[mb];

    struct epoll_event ev;
    struct epoll_event events[w];
    unordered_map<int, int> fdToIndex;
    vector<vector<char>> msgState(w);

    int nSent = 0, nRecv = 0;
    bool doneSending = false;

    // Create empty epoll list, get file discriptor
    int epollfd = epoll_create1(0);
    if(epollfd == -1)
        EXITONERROR("epoll could not be created.");

    // Fill epoll list with initial w requests, send first w requests
    for(int i = 0; i < w; i++) {
        if(!doneSending) {
            int reqSize = reqBuf->pop(buf, mb);

            if(*(MESSAGE_TYPE*) buf == QUIT_MSG) {
//                cout << "FOUND QUIT MSG BEFORE W REQUESTS SENT\n";
                doneSending = true;
                continue;
            }

            chans[i]->cwrite(buf, reqSize);
            nSent++;

            // Save orig message
            msgState[i] = vector<char>(buf, buf + reqSize);

            // Add chan's read side to epoll list
            int chanrfd = chans[i]->getrfd();
            fcntl(chanrfd, F_SETFL, O_NONBLOCK);
            fcntl(chans[i]->getwfd(), F_SETFL, O_NONBLOCK);

            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = chanrfd;
            fdToIndex[chanrfd] = i;
            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, chanrfd, &ev) == -1)
                EXITONERROR("epoll could not listen to channel read side.");
        }
    }
//    cout << "DONE WITH INITIAL SENDING FOR W CHANNELS\n";
    while(true) {
        // If no more requests and all have been processed break out of loop
        if(doneSending && nSent == nRecv)
            break;

        // Wait until epoll says ready, get number of chans ready
        int nChansReady = epoll_wait(epollfd, events, w, -1); // wait indefinitely until activity is found
        if(nChansReady == -1)
            EXITONERROR("epoll could not wait.");

        for(int i = 0; i < nChansReady; i++) {
            // Get index of channel using mapping from ready event's fd
            int index = fdToIndex[events[i].data.fd];

            // Get response
            chans[index]->cread(recvBuf, mb);
            nRecv++;

            // Get original request's type and process response
            vector<char> origReq = msgState[index];
            char* req = origReq.data();
            MESSAGE_TYPE m = *(MESSAGE_TYPE*) req;
            if(m == DATA_MSG) {
//                cout << "HAVE DATA MSG RESPONSE: " << *(double*) recvBuf << endl;
                hc->update(((datamsg*) req)->person, *(double*) recvBuf);
            } else if(m == FILE_MSG) {
                // Get filemsg info from orig request
                filemsg f = *(filemsg*) req;
                string fname = req + sizeof(filemsg);

                // Open and write response to file
                string recvfname = "received/" + fname;
                FILE* fp = fopen(recvfname.c_str(), "r+");
                fseek(fp, f.offset, SEEK_SET); // Need to seek so data goes in correct place
                fwrite(recvBuf, 1, f.length, fp);
                fclose(fp);
            }

            // Reuse channel if there are more requests
            if(!doneSending) {
                int reqSize = reqBuf->pop(buf, mb);

                if (*(MESSAGE_TYPE *) buf == QUIT_MSG)
                    doneSending = true;
                else { // Don't send quit message yet for this channel
                    chans[index]->cwrite(buf, reqSize);
                    msgState[index] = vector<char>(buf, buf + reqSize);
                    nSent++;
                }
            }
        }
    }

//    cout << "FINISHED READING ALL RESPONSES\n";

    // Clean up all channels
    for(int i = 0; i < w; i++) {
        MESSAGE_TYPE quitMsg = QUIT_MSG;
        chans[i]->cwrite(&quitMsg, sizeof(QUIT_MSG));
        delete chans[i];
    }
    delete[] chans;

//    cout << "FINISHED EVENT POLLING THREAD\n";
}

void file_thread_function(string* fname, BoundedBuffer* reqBuf, FIFORequestChannel* chan, int mb) {
    if(fname->empty())
        return;

    //1. Get file length from server
    char* buf = new char[sizeof(filemsg) + fname->size() + 1];
    filemsg f(0, 0);
    memcpy(buf, &f, sizeof(f));
    strcpy(buf + sizeof(f), fname->c_str());
    chan->cwrite(buf, sizeof(f) + fname->size() + 1);
    __int64_t fLength;
    chan->cread(&fLength, sizeof(fLength));
    delete[] buf;

    //2. Create new file, init with original file length
    string recvfname = "received/" + *fname;
    FILE* fp = fopen(recvfname.c_str(), "w");
    fseek(fp, fLength, SEEK_SET);
    fclose(fp);

    //2. Generate all file msgs
    __int64_t remLength = fLength;

    while(remLength > 0) {
        f.length = min((__int64_t) mb, remLength);
        buf = new char[f.length > mb ? f.length : mb];
        memcpy(buf, &f, sizeof(f));
        strcpy(buf + sizeof(f), fname->c_str());
        reqBuf->push(buf, sizeof(f) + fname->size() + 1);
        f.offset += f.length;
        remLength -= f.length;
        delete[] buf;
    }
}

FIFORequestChannel* createFIFOchan(FIFORequestChannel* ctrlChan) {
    char name[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    ctrlChan->cwrite(&m, sizeof(m));
    ctrlChan->cread(name, 1024);
//    cout << "Making new worker channel: " << name << endl;
    auto* newChan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
    return newChan;
}

int main(int argc, char *argv[]) {
    int n = 15000;    //default number of requests per "patient"
    int p = 1;     // number of patients [1,15]
    int w = 200;    //default number of worker channels
    int b = 500;    // default capacity of the request buffer, you should change this default
    int m = MAX_MESSAGE;    // default capacity of the message buffer
    char* size = (char *) "256";
    string f;
    srand(time_t(NULL));

    // Process cmdline arguments
    int opt;
    while ((opt = getopt(argc, argv, "m:n:b:w:p:f:")) != -1) {
        switch (opt) {
            case 'm':
                m = stoi(optarg);
                size = optarg;
                break;
            case 'n':
                n = stoi(optarg);
                break;
            case 'b':
                b = stoi(optarg);
                break;
            case 'w':
                w = stoi(optarg); // # of req. channels
                break;
            case 'p':
                p = stoi(optarg);
                break;
            case 'f':
                f = optarg;
                break;
            default:
                cout << "Argument -" << opt << " not recognized." << endl;
                exit(1);
        }
    }

    // Start server process
    int pid = fork();
    if (pid == 0) {
        char *sargs[] = {(char *) "./server", (char *) "-m", size, nullptr};
        execvp(sargs[0], sargs);

        // If execvp returns something went wrong... exit server thread here
        EXITONERROR("Server could not be started.");
    }

    auto *chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    HistogramCollection hc;

    // Populate hc with P histograms
    for (int i = 0; i < p; i++)
        hc.add(new Histogram(10, -2.0, 2.0));

    // Make worker channels
    auto** workerChans = new FIFORequestChannel*[w];

    for (int i = 0; i < w; i++)
        workerChans[i] = createFIFOchan(chan);

    struct timeval start, end;
    gettimeofday(&start, nullptr);

    /* Start all threads here */
    thread patients[p];
    if(f.empty())
        for (int i = 0; i < p; i++)
            patients[i] = thread(patient_thread_function, n, i + 1, &request_buffer);
    thread filethread(file_thread_function, &f, &request_buffer, chan, m);

    // Create single worker thread
    thread worker(event_polling_function, &request_buffer, workerChans, &hc, m, w);

    /* Join all threads here */
    if (f.empty()) {
        for (int i = 0; i < p; i++)
            patients[i].join();
        filethread.detach(); // We don't need this thread, not doing file transfer
    } else
        filethread.join();

    // Send quit msg to worker thread
    MESSAGE_TYPE q = QUIT_MSG;
    request_buffer.push((char *) &q, sizeof(q));

    worker.join();

    gettimeofday(&end, nullptr);
    // print the results
    hc.print();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) / (int) 1e6;
    int usecs = (int) (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    // Clean up main channel
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
}

#pragma clang diagnostic pop