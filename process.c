
#include "p2pApp.h"

int main(int argc , char *argv[])
{

    /**
     * TODO: FREE ALLOCATED MEMORY
     *     char **msg_log
     *     msg_log contents
     *     uint16_t **msg_ids
     *     msg_ids contents
     *     cmd_buf
     *     peer_msg_buf;
     */
    srand(time(0));
    int pid = atoi(argv[1]);
    int num_procs = atoi(argv[2]);
    int tcp_port = atoi(argv[3]);
    int udp_port = ROOT_ID + pid;
    int local_seqnum = 1; /* Next seqnum the server will send. */

    /** Server variables */
    struct sockaddr_in client_address;
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
    int tcp_socket, new_tcp_socket, udp_socket, i, cmd_type, num_neighbors, valread;
    int active = TRUE;
    fd_set active_fd_set, read_fd_set;

    /** App Logic Variables*/
    /* Vector clock. with n entries
     * if vector_clock[i] == x:
     * lowest message NOT SEEN from peer i is x
     * */
    int potential_neighbors[2];
    uint16_t vector_clock[num_procs];
    char incoming_message[256];
    size_t num_msgs = 0;
    char chat_log_out[(MAX_MSG_LEN+1)*MAX_MSGS];
    char buffer[1025];  /*data buffer of 1K*/

    /** Allocated variables */
    char **msg_log = (char**) malloc(MAX_MSGS*sizeof(char *));
    /* msg_ids holds the message_t identifier as an int array
    * in the format [<originalsender>,<seqno>]
    * for example msg 15 from server 1: [1,15]
     */
    uint16_t **msg_ids = (uint16_t **) malloc(MAX_MSGS*sizeof(uint16_t *));
    struct client_command *cmd_buf = malloc(sizeof(struct client_command));
    struct message_t *out_peer_msg_buf = malloc(sizeof(message_t));
    struct message_t *in_peer_msg_buf = malloc(sizeof(message_t));

    /* Init vector clock with 1's*/
    for (i=0; i<num_procs; i++){
        vector_clock[i] = (uint16_t) 1;
    }

    /*** TESTING ***/
//    char fake_cmd[250];
//    strcpy(fake_cmd, "chatlog in yo ass");
//    num_msgs += add_new_message(fake_cmd, pid, local_seqnum, msg_log,
//                                num_msgs, msg_ids, vector_clock, num_procs);
//    local_seqnum++;
//    num_msgs += add_new_message(fake_cmd, pid, local_seqnum, msg_log,
//                                num_msgs, msg_ids, vector_clock, num_procs);
//    local_seqnum++;
//    num_msgs += add_new_message(fake_cmd, pid, local_seqnum, msg_log,
//                                num_msgs, msg_ids, vector_clock, num_procs);
//    local_seqnum++;
//    send_log(msg_log, num_msgs, chat_log_out);
//    printf("Filled Chatlog: %s\n", chat_log_out);

    /*
     * testing parse_input

    strcpy(fake_cmd, "msg 12 chatLog for me and you");

    cmd_type = parse_input(fake_cmd, cmd_buf);

    printf("The command message_t received is of type %d. \n",cmd_buf->cmd_type);
    if (cmd_type == MSG){
        printf("Message ID: %d, Message: %s\n\n",cmd_buf->msg_id, cmd_buf->msg);
    }

    strcpy(fake_cmd, "This is a test message my hombre");
     */
    /* test build message_t
    fill_message(peer_msg_buf, RUMOR, pid, pid,1, vector_clock, fake_cmd, num_procs);

    /* test update log
    update_log(peer_msg_buf, msg_log, num_msgs, msg_ids, vector_clock, num_procs);

    TODO: Test read Status Message
    */

    /*** END TESTING ***/



    /*Create a TCP socket*/
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("TCP Socket failed");
        exit(EXIT_FAILURE);
    }
    printf("TCP Socket Made\n");
    fflush(stdout);

    /*Create a UDP socket*/
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP Socket failed");
        exit(EXIT_FAILURE);
    }

    /*Create TCP address */
    memset(&tcp_address, 0, sizeof(struct sockaddr_in));
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(tcp_port);

    /*Create UDP address */
    memset(&udp_address, 0, sizeof(struct sockaddr_in));
    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_address.sin_port = htons(udp_port);

    int tcp_addr_len = sizeof(tcp_address);

    /*Bind the TCP socket to localhost port specified in start command*/
    if (bind(tcp_socket, (struct sockaddr *) &tcp_address, sizeof(tcp_address)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }

    /*try to specify maximum of 3 pending connections for the master socket*/
    if (listen(tcp_socket, 3) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
/*
    printf("TCP Listen\n");
    printf("Listener on port %d \n", ntohs(tcp_address.sin_port));
    fflush(stdout);
*/
    if ((new_tcp_socket = accept(tcp_socket, (struct sockaddr *) &client_address, (socklen_t *) &tcp_addr_len)) < 0) {
        perror("accept");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    /*inform user of socket number - used in send and receive commands*/
    printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_tcp_socket,
           inet_ntoa(tcp_address.sin_addr), ntohs(tcp_address.sin_port));

    /*
    char *message = "TCP Connection Established between proxy and server\r\n";
    fflush(stdout);

    /*send new connection greeting message_t
    if (send(new_tcp_socket, message, strlen(message), 0) != strlen(message)) {
        perror("Intial TCP connection send error");
    }
    */

    /*Bind the UDP socket to localhost and port with PID + 20000*/
    if (bind(udp_socket, (struct sockaddr *) &udp_address, sizeof(udp_address)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (new_tcp_socket, &active_fd_set);
    FD_SET(udp_socket, &active_fd_set);


    /* Get P2P Ports to send on*/
    num_neighbors = init_neighbors(pid, num_procs, potential_neighbors);
    printf("Process %d has %d neighbors\n",pid,num_neighbors);
    fflush(stdout);

    int neighbor_ports[num_neighbors];
    get_neighbor_ports(pid, num_procs, neighbor_ports);

    while(active == TRUE) {

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;

        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                /* PROXY -- SERVER TCP CONNECTION I/O */
                if ( i == new_tcp_socket) {
                    printf("Received a message on TCP\n");
                    fflush(stdout);

                    /*Check if it was for closing , and also read the incoming message_t*/
                    if ((valread = read( new_tcp_socket , buffer, 1024)) == 0) {
                        /*Somebody disconnected , get his details and print*/
                        int client_addr_len = sizeof(client_address);
                        getpeername(new_tcp_socket , (struct sockaddr*)&client_address , (socklen_t *) &client_addr_len);
                        printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(client_address.sin_addr) , ntohs(client_address.sin_port));
                        close( new_tcp_socket );
                    }
                    buffer[valread] ='\0';
                    strcpy(incoming_message, buffer);


                    /* Process the client command */
                    if ((cmd_type = parse_input(incoming_message, cmd_buf)) == -1){
                        printf("cmd error");
                    }

                    /*TODO: Handle malformed msg command */
                    if (cmd_type == CRASH){
                        active = FALSE;
                        break;
                    }
                    else if(cmd_type == GET){
                        /* prepare chatlog to send. */
                        send_log(msg_log, num_msgs, chat_log_out);
                        printf("Filled Chatlog: %s\n", chat_log_out);

                        if(send(new_tcp_socket, chat_log_out, strlen(chat_log_out), 0 ) != strlen(chat_log_out)){
                            printf("ChatLog Send wrong size\n");
                        }
                    }
                    else if(cmd_type == MSG){
                        /* New Message from client! */
                        /* Always new */
                        num_msgs += add_new_message(cmd_buf->msg, pid, local_seqnum, msg_log,
                                num_msgs, msg_ids, vector_clock, num_procs);
                        fill_message(out_peer_msg_buf, RUMOR, pid, pid, local_seqnum,
                                vector_clock, cmd_buf->msg , num_procs);
                        local_seqnum++;

                        printf("Filled Message:\n");
                        print_message(out_peer_msg_buf, num_procs);


                        /* Send new message to nearby server */
                        struct sockaddr_in peer_serv_addr;
                        peer_serv_addr.sin_family = AF_INET;
                        peer_serv_addr.sin_addr.s_addr = INADDR_ANY;

                        i = pick_neighbor(num_neighbors);

                        printf("selected Neghbor number %d\n",i);
                        peer_serv_addr.sin_port = htons(neighbor_ports[i]);

                        printf("Sending Message from UDP port: %d to port: %d\n ", udp_port, ntohs(peer_serv_addr.sin_port));

                        sendto(udp_socket, (const char *) out_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT,
                               (const struct sockaddr *) &peer_serv_addr, sizeof(peer_serv_addr));
                        printf("Test message sent on UDP.\n\n");
                    }



                    /* SERVER -- SERVER UDP CONNECTION I/O */
                } else {
                    printf("Received a message on UDP\n");
                    fflush(stdout);

                    //TODO: Put all this code in proper gossip logic functions
                    struct sockaddr_in peer_serv_addr;
                    memset(&peer_serv_addr, 0, sizeof(peer_serv_addr));
                    char udp_buffer[1024];
                    int len, n;

                    len = sizeof(peer_serv_addr);

                    if ((n = recvfrom(udp_socket, in_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT, ( struct sockaddr *) &peer_serv_addr, &len)) == -1) {
                        perror("Recv From Failed\n");
                    }
//                    print_message(in_peer_msg_buf,num_procs);
//                    udp_buffer[n] = '\0';
//                    printf("Client : %s\n", udp_buffer);
                }
            }
        }
    }
    /* Exit or Crash. Break the While Loop.
     * Free Memory and exit.
     */
    free(cmd_buf);
    free(out_peer_msg_buf);
    free(in_peer_msg_buf);
    for (i =0; i<num_msgs; i++){
        free(msg_log[i]);
        free(msg_ids[i]);
    }
    free(msg_log);
    free(msg_ids);
}
