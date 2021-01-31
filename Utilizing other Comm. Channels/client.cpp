/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"

using namespace std;


int main(int argc, char *argv[]) {
    double time = -1;
    int p = -1, e = -1,max_buffer = MAX_MESSAGE, numChannels = 1;
    char copt;
    string filename, i = "f";
    char *size = (char *) "256";

    /* Check through all flags using getopt()... if match is found then ensure it is a proper input */
    while ((copt = getopt(argc, argv, "c:p:e:f:i:m:t:")) != -1) {
        switch (copt) {
            case 'c':
                numChannels = stoi(optarg);
                break;
            case 'p':
                p = stoi(optarg);
                break;
            case 'e':
                e = stoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'i':
                i = optarg;
                break;
            case 'm':
               max_buffer = stoi(optarg);
                size = optarg;
                break;
            case 't':
                time = stod(optarg);
                break;
            default:
                std::cout << "Arg flag -" << copt << " not recognized." << endl;
        }
    }

    // Fork off to start server
    if (!fork()) { // Currently on child process, start server
        char *sargs[] = {(char *) "./server", (char *) "-m", size, (char *) "-i", (char *) i.c_str(), NULL};
        execvp(sargs[0], sargs);

        // If execvp returns something went wrong... exit here
        EXITONERROR("Server could not be started.");
    }

    //cout << "CLIENT CREATING CTRL CHANNEL" << endl;

    // create control channel using desired ipc meathod
    RequestChannel *ctrl_chan = NULL;
    if (i.compare("f") == 0) { // Use FIFO named pipe
        ctrl_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    } else if (i.compare("q") == 0) { // Use msg queue
        ctrl_chan = new MQRequestChannel("control", MQRequestChannel::CLIENT_SIDE);
    } else if (i.compare("s") == 0) { // Use shrd mem
        ctrl_chan = new SHMRequestChannel("control", SHMRequestChannel::CLIENT_SIDE,max_buffer);
    }


    // Create any extra channels in desired, if n == 1 or not input by user no extra channels are created
    vector<RequestChannel*> allChannels;
    allChannels.push_back(ctrl_chan); // Push first channel
    RequestChannel* chan;
    for(int n = 1; n < numChannels; n++) {
        if(i.compare("f") == 0) { // Use FIFO named pipe
            chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
        } else if(i.compare("q") == 0) { // Use msg queue
            chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
        } else if(i.compare("s") == 0) { // Use shrd mem
            chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, max_buffer);
        }
        allChannels.push_back(chan);
    }


    // Start clock for measuring time length of process
    struct timeval start, end;
    gettimeofday(&start, NULL);

    //cout << "CLIENT DONE INITIALIZING" << endl;

    if (filename.length() != 0) {
        //cout << "Copying file " << filename << endl;
        // 
        // get length of file in question
        //
        filemsg fm(0, 0);

        // Build buffer that contains filemsg with filename appended behind it (including the null character at its end)
        char* buf = new char[sizeof(filemsg) + filename.size() + 1];
        memcpy(buf, &fm, sizeof(filemsg));
        memcpy(buf + sizeof(filemsg), filename.c_str(),filename.size() + 1);
        //cout << "CLIENT REQUESTING FILELEN" << endl;
        ctrl_chan->cwrite(buf, sizeof(filemsg) + filename.size() + 1);
        //cout << "CLIENT READING FILELEN" << endl;
        __int64_t filelength;
        //sleep(0.0005);
        ctrl_chan->cread(&filelength, sizeof(__int64_t));
        delete[] buf;

        //std::cout << "File length: " << filelength << " bytes" << endl;

        // Compute number of required requests using max buffer size
        __int64_t numSends = ceil((filelength + 0.0) / max_buffer);
        //double amountPerChannel = (filelength + 0.0) / numChannels;

        //cout << "MAX_MESSAGE = " << MAX_MESSAGE << endl << "Sends required = " << numSends << endl;
        //cout << "LAST SEND SIZE = " << (filelength - (numSends-1)*MAX_MESSAGE) << endl;

        //
        // Copy requested file
        //
        ofstream outfile("received/" + filename);

        for (int count = 0; count < numSends; count++) {
            // Set offset & build buffer
            fm.offset = count * max_buffer;
            fm.length = count == numSends - 1 ? (filelength - count*max_buffer) : max_buffer;
            buf = new char[fm.length];
            memcpy(buf, &fm, sizeof(filemsg));
            memcpy(buf + sizeof(filemsg), filename.c_str(), filename.size() + 1);

            //if(i == numSends-1) // If the last request
            //    cout << "Last request of size " << fm.length << " and offset " << fm.offset << endl;
            // Send Request
            allChannels[count % numChannels]->cwrite(buf, fm.length);
            //sleep(0.0005); // small time buffer to ensure server could respond
            // Read response
            allChannels[count % numChannels]->cread(buf, fm.length);

            // Append to 'new' file
            outfile.write(buf, fm.length);
            delete[] buf;
        }

        outfile.close();

    } else if(p != -1 && time >= 0.0 && e != -1) {
        for(RequestChannel* chann: allChannels) {
            char *buf[MAX_MESSAGE];
            //cout << "Getting single data point for <p, t, e> = " << p << ',' << time << ',' << e << endl;
            datamsg *dm = new datamsg(p, time, e);
            chann->cwrite(dm, sizeof(datamsg));
            chann->cread(buf, MAX_MESSAGE);
            double reply = *(double *) buf;
            cout << "Received response " << reply << " from server." << endl;
            delete dm;
        }
    } else if(p != -1) { // Copy 1K data points for e == 1,2, or both
        char* buf[MAX_MESSAGE];
        double reply1, reply2;

        std::ofstream outfile("x1.csv");
        outfile.clear();

        time = 0; // Use t as 0 for this test to make comparing result file and the original easier
        for(int count = 0; count < 1000; count++) {
            for(RequestChannel* chann: allChannels) {
                // Send msgs and read reply for each
                datamsg *dm1 = new datamsg(p, time, 1);
                datamsg *dm2 = new datamsg(p, time, 2);

                outfile << time;

                if (e != 2) { // If not only wanting e=2
                    chann->cwrite(dm1, sizeof(datamsg));
                    chann->cread(buf, max_buffer);
                    reply1 = *(double *) buf;
                    outfile << ',' << reply1;
                }

                if (e != 1) { // If not only wanting e=1
                    chann->cwrite(dm2, sizeof(datamsg));
                    chann->cread(buf, max_buffer);
                    reply2 = *(double *) buf;
                    outfile << ',' << reply2;
                }

                outfile << endl;
                // Increment t for next data point
                time += 0.004;
                delete dm1;
                delete dm2;
            }
        }
        outfile.close();
    } else {
        cerr << "Invalid flags detected, please run with '-f <filename>' or '-p [1-15] -e [1|2]'" << endl;
        exit (EXIT_FAILURE);
    }

    // Calculate time to receive the data point, display to user
    gettimeofday(&end, NULL);
    std::cout << "Time taken: " << ((end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec)) * 1e-6 << "s" << endl;

    // Tell server to close
    MESSAGE_TYPE mt = QUIT_MSG;
    ctrl_chan->cwrite(&mt, sizeof(MESSAGE_TYPE));
    // Delete all channels
    for(RequestChannel* chann: allChannels)
        delete chann;

    
    // Wait for server to finish before exiting
    wait(0);

    return 0;
}
