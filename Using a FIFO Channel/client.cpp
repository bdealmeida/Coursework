/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;


int main(int argc, char *argv[]){
    double t = -1;
    int p = -1, e = -1, max_buffer = MAX_MESSAGE;
    char copt;
    string filename;
    char* size = "256";

    /* Check through all flags using getopt()... if match is found then ensure it is a proper input */
    while((copt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
        switch(copt) {
            case 'p':
                p = stoi(optarg);
                break;
            case 't':
                t = stod(optarg);
                break;
            case 'e':
                e = stod(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'm':
                max_buffer = stoi(optarg);
                size = optarg;
                break;
            default:
                std::cout << "Arg flag -" << copt << " not recognized." << endl;
        }
    }

    // Split with fork
    if(fork() == 0) { // Currently on child process, start server
        char* sargs[] = {"./server", "-m", size, NULL};
        execvp(sargs[0], sargs);

        // If execvp returns something went wrong... exit here
        cout << "Server could not be started." << endl;
        return -1;
    }
   
    // Request channel with server
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

    struct timeval start, end;
    gettimeofday(&start, NULL); // Start clock for measuring time length of process

    if(filename.length() != 0) {
        // 
        // get length of file in question
        //
        filemsg fm(0, 0);

        // Build buffer that contains filemsg with filename appended behind it (including the null character at its end)
        char buf [sizeof(filemsg) + filename.size() + 1];
        memcpy(buf, &fm, sizeof(filemsg));
        memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1); // Converting the filename to a c_str gives us the null character

        chan.cwrite(buf, sizeof(filemsg) + filename.size() + 1);

        __int64_t filelength;
        chan.cread(&filelength, sizeof(__int64_t));

        //std::cout << "File length: " << filelength << " bytes" << endl;
        int windowSize = max_buffer;

        // Compute number of required requests using max buffer size
        __int64_t numSends = ceil((filelength + 0.0) / windowSize);

        //cout << "MAX_MESSAGE = " << MAX_MESSAGE << endl << "Sends required = " << numSends << endl;
        //cout << "LAST SEND SIZE = " << (filelength - (numSends-1)*MAX_MESSAGE) << endl;

        //
        // Copy requested file
        //
        ofstream outfile("received/" + filename);

        for(int i = 0; i < numSends; i++) {
            // Set offset & build buffer
            fm.offset = i*windowSize;
            fm.length = i == numSends-1 ? (filelength - i*windowSize) : windowSize;
            char receivingBuf [fm.length];
            memcpy(receivingBuf, &fm, sizeof(filemsg));
            memcpy(receivingBuf + sizeof(filemsg), filename.c_str(), filename.size() + 1);

            //if(i == numSends-1) // If the last request
            //    cout << "Last request of size " << fm.length << " and offset " << fm.offset << endl;

            // Send Request
            chan.cwrite(receivingBuf, fm.length);

            // Read response
            chan.cread(receivingBuf, fm.length);

            // Append to 'new' file
            outfile.write(receivingBuf, fm.length);
        }
        outfile.close();

    } else if(t == -1 && p != -1) { // Normally would copy all 15k data points but for the sake of time only does 1k
        t = 0;

        char* buf[MAX_MESSAGE];
        double reply1, reply2;

        // Get as many data points as requested, write responses in x1.csv
        struct timeval start, end;
        gettimeofday(&start, NULL); // Start clock for measuring time length of process

        std::ofstream outfile("x1.csv");
        outfile.clear();

        t = 0; // Use t as 0 for this test to make comparing result file and the original easier
        for(int i = 0; i < 1000; i++) {
            // Send msgs and read reply for each
            datamsg* dm1 = new datamsg(p, t, 1);
            datamsg* dm2 = new datamsg(p, t, 2);

            outfile << t;

            if(e != 2) { // If not only wanting e=2
                chan.cwrite(dm1, sizeof(datamsg));
                int nbytes = chan.cread(buf, MAX_MESSAGE);
                reply1 = *(double *) buf;
                outfile << ',' << reply1;
            }

            if(e != 1) { // If not only wanting e=1
                chan.cwrite(dm2, sizeof(datamsg));
                int nbytes = chan.cread(buf, MAX_MESSAGE);
                reply2 = *(double *) buf;  
                outfile << ',' << reply2;  
            }

            outfile << endl;

            // Increment t for next data point
            t += 0.004;
        }
        outfile.close();

        // Calculate time to complete the 1000 data point collection, display to user
        gettimeofday(&end, NULL);
        std::cout << "Time taken for 1000 points was: " << ((end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec)) * 1e-6 << "s" << endl;
    } else { // Test channel creation using data points
        char* buf[MAX_MESSAGE];
        datamsg* dm1 = new datamsg(1, 0, 1);
        chan.cwrite(dm1, sizeof(datamsg));
        int nbytes = chan.cread(buf, MAX_MESSAGE);
        double reply = *(double *) buf;

        std::cout << "Reply " << reply << " from server using first channel... creating second." << std::endl;

        // Request 2nd channel
        MESSAGE_TYPE req = NEWCHANNEL_MSG;
        chan.cwrite(&req, sizeof(MESSAGE_TYPE));

        char chanName[100];
        chan.cread(chanName, sizeof(char[100]));
        cout << chanName << endl;

        // Make 2nd channel using server's given name
        FIFORequestChannel chan2 (chanName, FIFORequestChannel::CLIENT_SIDE);

        datamsg* dm2 = new datamsg(1, 0, 1);
        chan2.cwrite(dm2, sizeof(datamsg));
        nbytes = chan2.cread(buf, MAX_MESSAGE);
        reply = *(double *) buf;
        std::cout << "Reply " << reply << " from server using second channel... removing this channel." << std::endl;

        // Close 2nd channel
        MESSAGE_TYPE m = QUIT_MSG;
        chan2.cwrite(&m, sizeof(MESSAGE_TYPE));

        datamsg* dm3 = new datamsg(1, 0, 1);
        chan.cwrite(dm3, sizeof(datamsg));
        nbytes = chan.cread(buf, MAX_MESSAGE);
        reply = *(double *) buf;
        std::cout << "Reply " << reply << " from server using first channel... done testing." << std::endl;
    }

    // Calculate time to receive the data point, display to user
    gettimeofday(&end, NULL);
    std::cout << "Time taken: " << ((end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec)) * 1e-6 << "s" << endl;

    // closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite (&m, sizeof (MESSAGE_TYPE));

    // Wait for child server to close
    wait(NULL);
    cout << "Server process terminated, client exiting." << endl;
    
    return 0;
}
