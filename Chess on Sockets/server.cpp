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
    
#define PORT 8888 

using namespace std;
    
int main(int argc , char *argv[])  
{  
    int opt = true;  
    int master_socket, addrlen, new_socket, client_socket[2], 
          max_clients = 2, activity, valread, sd, fd;  
    int max_sd;  
    struct sockaddr_in address;  
        
    char buffer[1025]; 
        
    //set of socket descriptors 
    fd_set readfds;  
    
    for (int i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
        
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
        
    if (listen(master_socket, 2) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  

    int turn = -1;
        
    while(true)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  

        //add child sockets to set 
        for (int i = 0 ; i < max_clients ; i++)  
        {  
            sd = client_socket[i];       
            if(sd > 0)  FD_SET(sd, &readfds);  
            if(sd > max_sd) max_sd = sd;  
        }  
    
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno != EINTR))  
        {  
            printf("select error");  
        }  
            
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            
            //inform user of socket number - used in send and receive commands 
            printf("New connection, socket fd is : %d , ip is : %s, port : %d \n", 
                new_socket , inet_ntoa(address.sin_addr), ntohs(address.sin_port));  
                
            //add new socket to array of sockets 
            for (int i = 0; i < max_clients; i++)  
            {  
                //if position is empty 
                if( client_socket[i] == 0 )  
                {  
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                    break;  
                }  
            }
        }
        //else its some IO operation on some other socket
        if(client_socket[0] > 0 && client_socket[1] > 0) {
            if(turn == -1) {
                char message[] = "GAME STARTED!";
                message[strlen(message)] = '\0';
                if(send(client_socket[0], message, strlen(message), 0) > 0) {
                    send(client_socket[0], "YOUR TURN:\0", 10, 0);
                }
                send(client_socket[1], message, strlen(message), 0);
                turn = 0;
            } else {
                fd = client_socket[turn];  
                sd = client_socket[turn ^ 1];
                if (FD_ISSET( fd , &readfds))  
                {  
                    //Check if it was for closing , and also read the 
                    //incoming message 
                    if ((valread = read(fd , buffer, 1024)) == 0)  
                    {  
                        //Somebody disconnected , get his details and print 
                        getpeername(fd, (struct sockaddr*)&address , (socklen_t*)&addrlen);  
                        printf("Host disconnected : ip %s , port %d \n", 
                              inet_ntoa(address.sin_addr), ntohs(address.sin_port));  
                            
                        //Close the socket and mark as 0 in list for reuse 
                        close(fd);  
                        client_socket[turn] = 0;  
                        break;
                    }  
                    else
                    {  
                        cerr << buffer << "\n";
                        buffer[valread] = '\0';  
                        send(sd, buffer, strlen(buffer), 0);
                    }  
                }  
                turn ^= 1;
            }
        } 
    }  
        
    return 0;  
}  