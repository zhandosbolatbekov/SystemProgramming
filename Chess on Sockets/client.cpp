#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    // setup default arguments
    int option;
    int port = 8888;
    string host = "localhost";

    // use DNS to get IP address
    struct hostent *hostEntry;
    hostEntry = gethostbyname(host.c_str());
    if (!hostEntry) {
        cout << "No such host name: " << host << endl;
        exit(-1);
    }

      // setup socket address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    memcpy(&server_addr.sin_addr, hostEntry->h_addr_list[0], hostEntry->h_length);

      // create socket
    int server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
    }

    // connect to server
    if (connect(server, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }

    // allocate buffer
    int buflen = 1024;
    char* buf = new char[buflen + 1];

    bool started = false;
    while(!started) {
        memset(buf, 0, buflen);
        recv(server, buf, buflen, 0);  
        char message[] = "GAME STARTED!";
        if(strcmp(buf, message) == 0) {
            cout << buf << "\n";
            started = true;
            break;
        }
    }

    while (true) {
        // read the response
        memset(buf, 0, buflen);
        recv(server, buf, buflen, 0);

        cout << "\n" << buf << "\n";

        if(strcmp(buf, "YOUR TURN:")) {
            cout << "\nYOUR TURN:\n";
        }
        
        string line;
        getline(cin, line);
        send(server, line.c_str(), line.length(), 0);
    }

    // Close socket
    close(server);
}