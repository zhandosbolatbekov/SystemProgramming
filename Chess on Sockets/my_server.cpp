#include <cstdio> 
#include <string>   
#include <cstdlib> 
#include <cerrno> 
#include <unistd.h>  
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <iostream>
#include <cstdlib>
    
#define PORT 8888 

using namespace std;

const string WHITE = "white";
const string BLACK = "black";
const string GO = "GO";
const string STOP = "STOP";

int sendTCP(int socket, string buf) {
    int n = send(socket, buf.c_str(), buf.size() + 1, 0);
    if(n < 0) {
        cerr << buf << endl;
        printf("send error\n");
        exit(EXIT_FAILURE);
    }
    return n;
}
    
int main(int argc, char *argv[])  
{  
    int listening;
        
    char buf[1025]; 

    if( (listening = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    struct sockaddr_in address;  
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
        
    if (bind(listening, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  

    printf("Listener on port %d \n", PORT);  
        
    if (listen(listening, 2) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  

    int client[2];
    int addrlen = sizeof(address);

    //accept incoming connections 
    for(int i = 0; i < 2; ++i) {
        puts("Waiting for a player");
        client[i] = accept(listening, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        //inform user of socket number - used in send and receive commands 
        printf("New connection, socket fd is : %d , ip is : %s, port : %d \n", 
                client[i] , inet_ntoa(address.sin_addr), ntohs(address.sin_port));  
        if(client[i] < 0) {
            printf("error accepting client #%d", i + 1);
            exit(EXIT_FAILURE);
        }
    }

    sendTCP(client[0], WHITE);
    sendTCP(client[1], BLACK);

    for(int i = 0; i < 2; ++i) {
        recv(client[i], buf, 1024, 0);
        cout << i + 1 << ": " << buf << "\n";
    }

    int turn = 0;
    bool running = true;
        
    while(running)  
    {  
        int cur = client[turn];
        int waiting = client[turn ^ 1];

        recv(cur, buf, 1024, 0);
        sendTCP(waiting, string(buf));
        cerr << buf << endl;

        turn ^= 1;
    }  

    close(listening);
        
    return EXIT_SUCCESS;  
}  