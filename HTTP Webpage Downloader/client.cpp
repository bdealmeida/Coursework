#include "common.h"
#include "TCPreqchannel.h"

using namespace std;

void httprequest(string url) {
    cout << "Attempting to download from: " << url << endl;

    /*====================================
    / Get host and req from given url
    /===================================*/
    string host, request;

    if(url.find("https://") != string::npos) {
        cout << "HTTPS found, exiting." << endl;
        return;
    }

    // Remove "http://" if present (since a HTTP connection is implied this means nothing here)
    if(url.find("http://") != string::npos)
        url = url.substr(7);
    cout << "URL : " << url << endl;
    if(url.find('/') != string::npos) { // Has a request portion
        request = url.substr(url.find('/'));
        host = url.substr(0, url.find('/'));
    } else {
        host = url;
        request = "/";
    }
    while(request[request.size()-1] == '\n' || request[request.size()-1] == '\r')
        request = request.substr(0, request.size()-1);
    cout << "Host: " << host << endl;
    cout << "Reqt: " << request << endl << endl;

    /*====================================
    / Create TCP connection with host
    /===================================*/
    auto* TCPchannel = new TCPRequestChannel(host, "80"); // Port 80 for HTTP

    /*====================================
    / Format and send HTTP GET request
    /===================================*/
    string HTTPreq = "GET " + request + " HTTP/1.1\r\nHost: " + host + "\r\nAccept-Language: en-us\r\nConnection: Close\r\n\r\n";
    char* buf = new char[HTTPreq.size()+1];
    strcpy(buf, HTTPreq.c_str());
//    cout << "\n----------HTTP REQUEST----------\n" << HTTPreq << endl << "--------END HTTP REQUEST--------" << endl;
    TCPchannel->cwrite(buf, HTTPreq.size()+1);
    delete[] buf;

    /*====================================
    / Read first* part of host response
    /===================================*/
    char* resBuf = new char[1024]; // This is long enough to hold header
    memset(resBuf, '\0', 1024);
    TCPchannel->cread(resBuf, 1024);
    string response = string(resBuf, 1024);

    /*====================================
    / Process
    /===================================*/
    int retCode = stoi(response.substr(9, 3));
    if(retCode == 200) { // Successful, copy file using remainder of resBuf and read again until done
        // Open file for writing
        ofstream outfile;
        outfile.open("response.html");

        int startingLoc = response.find("<", response.find("Connection: close"));
        if(startingLoc != string::npos)
            for(int i = startingLoc; i < 1024; i++) {
                outfile << resBuf[i];
                //outfile.flush();
            }
cout << "FIRST PART:" <<endl<<response<<endl;

        int bytesRead;
        do {
            memset(resBuf, '\0', 1024);
            bytesRead = TCPchannel->cread(resBuf, 1024);
            for(int i = 0; i < bytesRead; i++)
                outfile << resBuf[i];
        } while(bytesRead > 0);

        outfile.close();
//        cout << "HTML FILE START-19? = " << response.find("connection: close") << " -> START = " << response[response.find("connection: close")+21] <<endl;
        cout << "SUCCESS" << endl;
    } else if(retCode == 302 || retCode == 301) { // Redirected elsewhere
//        cout << response << endl;
        int location = response.find("Location: ")+10;
        if(location == string::npos)
            location = response.find("location: ")+10;
        int endLoc = response.find("\n", location);
        httprequest(response.substr(location, endLoc - location));
    } else {
        cout << "Host returned code: " << retCode << endl;
    }

    delete[] resBuf;
    delete TCPchannel;
}

int main(int argc, char *argv[]) {
    struct timeval start, end;
    gettimeofday(&start, nullptr);

    httprequest(argv[1]);

    // Calculate and print time taken
    gettimeofday(&end, nullptr);
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) / (int) 1e6;
    int usecs = (int) (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) % ((int) 1e6);
    cout << endl << "Operation Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
}