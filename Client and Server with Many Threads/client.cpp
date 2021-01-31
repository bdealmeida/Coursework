#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
#pragma ide diagnostic ignored "cert-msc51-cpp"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#include <thread>
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

void worker_thread_function(BoundedBuffer* reqBuf, FIFORequestChannel* chan, HistogramCollection* hc, int mb){
    char* buf = new char[mb];
    double response;
    while(true) {
        // Get message type
        reqBuf->pop(buf, mb);
        MESSAGE_TYPE m = *(MESSAGE_TYPE*) buf;
        //cout << "FOUND FILEMSG = " << m << endl;

        // Process req based on type of message
        if(m == DATA_MSG) {
            chan->cwrite(buf, sizeof(datamsg));
            chan->cread(&response, sizeof(double));
            hc->update(((datamsg*) buf)->person, response);
        } else if(m == QUIT_MSG) {
            chan->cwrite(&m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        } else if(m == FILE_MSG) {
            // Push and receive response
            filemsg f = *(filemsg*) buf;
            string fname = buf + sizeof(filemsg);
            int size = sizeof(filemsg) + fname.size() + 1;
            chan->cwrite(buf, size);
            chan->cread(buf, mb);

            // Open and write to new file
            string recvfname = "received/" + fname;
            FILE* fp = fopen(recvfname.c_str(), "r+");
            fseek(fp, f.offset, SEEK_SET); // Need to seek so data goes in correct place
            fwrite(buf, 1, f.length, fp);
            fclose(fp);
        }
    }
    delete[] buf;
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
    int w = 200;    //default number of worker threads
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
                w = stoi(optarg);
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

    int pid = fork();
    if (pid == 0) {
        char *sargs[] = {(char *) "./server", (char *) "-m", size, NULL};
        execvp(sargs[0], sargs);

        // If execvp returns something went wrong... exit here
        EXITONERROR("Server could not be started.");;
    }

    auto *chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
    HistogramCollection hc;

    // Populate hc with P histograms
    for (int i = 0; i < p; i++)
        hc.add(new Histogram(10, -2.0, 2.0));

    // Make worker channels
    FIFORequestChannel *workerChans[w];
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

    thread workers[w];
    for (int i = 0; i < w; i++)
        workers[i] = thread(worker_thread_function, &request_buffer, workerChans[i], &hc, m);

    /* Join all threads here */
    if (f.empty()) {
        for (int i = 0; i < p; i++)
            patients[i].join();
        filethread.detach(); // We don't need this thread
//        cout << "Patient threads finished" << endl;
    } else {
        filethread.join();
//        cout << "Filethread finished" << endl;
    }

    for (int i = 0; i < w; i++) {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char *) &q, sizeof(q));
    } // Done forming queue for workers

    for (int i = 0; i < w; i++)
        workers[i].join();
//    cout << "Worker threads finished" << endl;

    gettimeofday(&end, nullptr);
    // print the results
    hc.print();
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) / (int) 1e6;
    int usecs = (int) (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) % ((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    // Clean up main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
}

#pragma clang diagnostic pop