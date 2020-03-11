//
// Created by Will Krasnoff on 3/11/20.
//

#include p2pApp.h
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

int main(int argc, char **argv) {

    int master_socket;
    int addrlen;
    int peer_sockets[2];
    int curr_port;
    struct sockaddr_in address;

    int option = TRUE;

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < NUM_SERVERS-1; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket - this will be the proxy one
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections
    //TODO: Look into if using the right option here after
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( curr_port );

    //bind the socket to localhost defined port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    while True{

    }

}
