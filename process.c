
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "p2pApp.h"
#include <sys/time.h>

#define TRUE   1
#define FALSE  0


int main(int argc , char *argv[])
{

    int pid = strtol(argv[1], NULL, 10);
    int num_procs = strtol(argv[2], NULL, 10);
    int PORT = atoi(argv[3]);

    int cmd_type;

    printf("The port is: %d\n", PORT);

    int opt = TRUE;
    int master_socket , p2p_socket, addrlen , new_socket , client_socket[num_procs] ,
            max_clients = num_procs , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;
    struct sockaddr_in p2p_address;


    /*--- Things that need to be freed ---*/

    /* msg_log holds the message content */
    char **msg_log = (char**) malloc(MAX_MSGS*sizeof(char *));

    /* msg_ids holds the message identifier as an int array
     * in the format [<originalsender>,<seqno>]
     * for example msg 15 from server 1: [1,15]
     */
    uint16_t **msg_ids = (uint16_t **) malloc(MAX_MSGS*sizeof(uint16_t *));

    /* a single client command structure to buffer incoming commands*/
    struct client_command *cmd_buf = malloc(sizeof(struct client_command));
    struct message *peer_msg_buf = malloc(sizeof(struct message));


    uint16_t vector_clock[num_procs]; /* Vector clock. with n entries*/

    char incoming_message[256];

    size_t num_msgs = 0;

    char chat_log_out[(MAX_MSG_LEN+1)*MAX_MSGS];

    char buffer[1025];  /*data buffer of 1K*/

    for (i=0; i<num_procs; i++){
        vector_clock[i] = (uint16_t) 0;
    }


    /*** TESTING ***/
    /*testing parse_input*/
    char fake_cmd[250];
    strcpy(fake_cmd, "msg 12 chatLog for me and you");

    cmd_type = parse_input(fake_cmd, cmd_buf);

    printf("The command message received is of type %d. ",cmd_buf->cmd_type);
    if (cmd_type == MSG){
        printf("Message ID: %d, Message: %s\n",cmd_buf->msg_id, cmd_buf->msg);
    }

    strcpy(fake_cmd, "This is a test message my hombre");
    /* test build message */
    fill_message(peer_msg_buf, RUMOR, pid, 1, vector_clock, fake_cmd, num_procs);

    /* test update log*/
    update_log(peer_msg_buf, msg_log, num_msgs, msg_ids, vector_clock, num_procs);
    /*** END TESTING ***/



    /*set of socket descriptors*/
    fd_set readfds;

    /*a message*/
    char *message = "ECHO Daemon v1.0 \r\n";

    /*initialise all client_socket[] to 0 so not checked*/
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    /*create a master socket*/
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /*create a p2p socket*/
    if( (p2p_socket = socket(AF_INET , SOCK_DGRAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /*set master socket to allow multiple connections ,
    this is just a good habit, it will work without this*/
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /*type of socket created*/
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port =htons(PORT);

    printf("Port is: %d \n", address.sin_port);

    /*bind the socket to localhost port*/
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

    /*try to specify maximum of 3 pending connections for the master socket*/
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /*accept the incoming connection*/
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    fflush(stdout);

    while(TRUE)
    {
        /*clear the socket set*/
        FD_ZERO(&readfds);

        /*add master socket to set*/
        FD_SET(master_socket, &readfds);
        FD_SET(p2p_socket, &readfds);
        max_sd = master_socket;

        /*add child sockets to set*/
        for ( i = 0 ; i < max_clients ; i++)
        {
            /*socket descriptor*/
            sd = client_socket[i];

            /*if valid socket descriptor then add to read list*/
            if(sd > 0)
                FD_SET( sd , &readfds);

            /*highest file descriptor number, need it for the select function*/
            if(sd > max_sd)
                max_sd = sd;
        }

        /*wait for an activity on one of the sockets , timeout is NULL ,
        so wait indefinitely*/
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        /*If something happened on the master socket ,
        then its an incoming connection*/
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

            /*inform user of socket number - used in send and receive commands*/
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs
                    (address.sin_port));

            /*send new connection greeting message*/
            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
            {
                perror("send");
            }

            puts("Welcome message sent successfully");

            /*add new socket to array of sockets*/
            for (i = 0; i < max_clients; i++)
            {
                /*if position is empty*/
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        /*else its some IO operation on some other socket*/
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET( sd , &readfds))
            {
                /*Check if it was for closing , and also read the
                incoming message*/
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    /*Somebody disconnected , get his details and print*/
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" ,
                           inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

                    /*Close the socket and mark as 0 in list for reuse*/
                    close( sd );
                    client_socket[i] = 0;
                }

                    /*Echo back the message that came in*/
                else
                {
                    /*set the string terminating NULL byte on the end
                    of the data read*/
                    buffer[valread] = '\0';

                    /*Get incoming message.*/
                    memcpy(incoming_message, buffer, valread+1);
                    printf("Message from client: %s\n", incoming_message);

                    /*parse the message*/
                    cmd_type = parse_input(incoming_message, cmd_buf);

                    printf("command Type: %d\n", cmd_type);


                    if (cmd_type == GET){
                        send_log(msg_log, num_msgs, chat_log_out);
                        printf("this is the correct version!\n");
                        send(sd , chat_log_out , strlen(chat_log_out) , 0 );
                    }

                }
            }
        }
    }

    return 0;
}

