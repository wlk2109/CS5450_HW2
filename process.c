//
// Created by Will Krasnoff on 3/11/20.
//

#include "p2pApp.h"

int main(int argc, char **argv) {

    int master_socket;
    int addrlen;
    int peer_sockets[2];
    int curr_port;
    struct sockaddr_in address;

    int option = TRUE;
    int new_socket , client_socket[30] ,
            max_clients = 30 , activity, i , valread , sd;
    int max_sd;


    char buffer[1025];  //data buffer of 1K

    char msg_log[MAX_MSGS][MAX_MSG_LEN+2]; /*MESSAGE LOG. 2 additional chars for '<server_id>:'*/
    uint16_t vector_clock[*argv[2]]; /* Vector clock. with n entries*/

    //set of socket descriptors
    fd_set readfds;

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < argv[2]; i++)
    {
        peer_sockets[i] = 0;
    }

    //create a master socket, all sockets will go through this hole
    if( (master_socket = socket(AF_INET , SOCK_DGRAM , 0)) == 0)
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

    //try to specify maximum of 2 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /**
     * int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
     */

    /*
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t*)&addrlen))<0)

    {
        perror("accept");
        exit(EXIT_FAILURE);
    }



    valread = read( new_socket , buffer, 1024);
    printf("%s\n",buffer );
    char *hello = "Hello from server";
    send(new_socket , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");
    */

    while (TRUE){
        return(-1);
    }
    /*
     *
     * LISTEN
     *
     *
     * RECEIVE
     *
     *
     * PARSE
     *
     *
     * PROCESS
     *
     *
     * UPDATE
     *
     *
     * SEND STUFF
     *
     *
     * LISTEN AGAIN
     *
     * */

}
