//
// Created by Ishan Virk on 3/13/20.
//

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "p2pApp.h"
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRUE   1
#define FALSE  0


int main(int argc , char *argv[])
{

    int pid = strtol(argv[1], NULL, 10);
    int num_procs = strtol(argv[2], NULL, 10);
    int PORT = atoi(argv[3]);

    printf("The port is: %d\n", PORT);
    fflush(stdout);

    int opt = TRUE;
    int master_socket , p2p_socket, addrlen , new_socket , client_socket[num_procs] ,
            max_clients = num_procs , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;
    struct sockaddr_in p2p_address;

    char msg_log[MAX_MSGS][MAX_MSG_LEN]; /*MESSAGE LOG. 2 additional chars for '<server_id>:'*/
    uint16_t vector_clock[num_procs]; /* Vector clock. with n entries*/
    char incoming_message[256];
    size_t num_msgs = 0;

    char chat_log_out[(MAX_MSG_LEN+1)*MAX_MSGS];

    char buffer[1025];  //data buffer of 1K

    //set of socket descriptors
    fd_set readfds;

    /*
     * for testing
     */
    if(TRUE){
        strcpy(msg_log[0],"Hello");
        num_msgs++;
        printf("Message %d:, %s\n", num_msgs, msg_log[num_msgs-1]);
        fflush(stdout);
    }

    if(TRUE){
        send_log(msg_log, num_msgs, chat_log_out);
        printf("Chat log: %s\n", chat_log_out);
        fflush(stdout);
    }


    //a message
    char *message = "ECHO Daemon v1.0 \r\n";

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //create a p2p socket
    if( (p2p_socket = socket(AF_INET , SOCK_DGRAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port =htons(PORT);

    printf("Port is: %d \n", address.sin_port);

    //bind the socket to localhost port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", address.sin_port);
    fflush(stdout);

    p2p_address.sin_family = AF_INET;
    p2p_address.sin_addr.s_addr = INADDR_ANY;
    int p2p_port = 20000 + pid;
    p2p_address.sin_port =htons(p2p_port);

    if (bind(p2p_socket, (struct sockaddr *)&p2p_address, sizeof(p2p_address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    client_socket[0] = p2p_socket;

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    fflush(stdout);

    while(TRUE)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        FD_SET(p2p_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("This is the port in int: %d\n", ntohs(address.sin_port));
            fflush(stdout);

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                    (address.sin_port));

            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
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
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds))
            {
                //Check if it was for closing , and also read the
                //incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }

                    //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end
                    //of the data read
                    buffer[valread] = '\0';
                    memcpy(incoming_message, buffer, valread+1);
                    printf("Sucks my balls\n");


                    printf("Message from client: %s\n", incoming_message);
                    fflush(stdout);

                    send(sd , buffer , strlen(buffer) , 0 );
                }
            }
        }
    }

    return 0;
}

