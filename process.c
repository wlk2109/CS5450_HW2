
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
    int tcp_socket, new_tcp_socket, udp_socket, i,j, neighbor_index, cmd_type, num_neighbors, valread;
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
    int next_msg[2];

    /** Allocated variables */
    char **msg_log = (char**) malloc(MAX_MSGS*sizeof(char *));
    /* msg_ids holds the message_t identifier as an int array
    * in the format [<originalsender>,<seqno>]
    * for example msg 15 from server 1: [1,15]
     */
    uint16_t **msg_ids = (uint16_t **) malloc(MAX_MSGS*sizeof(uint16_t *));
    struct client_command *cmd_buf = malloc(sizeof(struct client_command));
    struct message *out_peer_msg_buf = malloc(sizeof(message_t));
    struct message *in_peer_msg_buf = malloc(sizeof(message_t));

    /* Init vector clock with 1's*/
    for (i=0; i<num_procs; i++){
        vector_clock[i] = (uint16_t) 1;
    }

    /*** TESTING ***/
//    if (pid == 0) {
//        char fake_cmd[250];
//        strcpy(fake_cmd, "ShartlogInYoAss");
//        num_msgs += add_new_message(fake_cmd, pid, local_seqnum, msg_log,
//                                    num_msgs, msg_ids, vector_clock, num_procs);
//        local_seqnum++;}

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

    if ((new_tcp_socket = accept(tcp_socket, (struct sockaddr *) &client_address, (socklen_t *) &tcp_addr_len)) < 0) {
        perror("accept");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    int status = fcntl(new_tcp_socket, F_SETFL, fcntl(new_tcp_socket, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1){
        perror("calling fcntl");
        // handle the error.  By the way, I've never seen fcntl fail in this way
    }

    /*inform user of socket number - used in send and receive commands*/
    printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_tcp_socket,
           inet_ntoa(tcp_address.sin_addr), ntohs(tcp_address.sin_port));


    /*Bind the UDP socket to localhost and port with PID + 20000*/
    if (bind(udp_socket, (struct sockaddr *) &udp_address, sizeof(udp_address)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Server %d , UDP socket on port: %d\n", pid, ntohs(udp_address.sin_port));


    /* Get P2P Ports to send on*/
    num_neighbors = init_neighbors(pid, num_procs, potential_neighbors);
    printf("Process %d has %d neighbors\n",pid,num_neighbors);
    fflush(stdout);

    int neighbor_ports[num_neighbors];
    get_neighbor_ports(pid, num_procs, neighbor_ports);

//    /* Initialize the set of active sockets. */
//    FD_ZERO (&active_fd_set);
//    FD_SET (new_tcp_socket, &active_fd_set);
//    FD_SET(udp_socket, &active_fd_set);

    printf("SERVER STARTED AND WAITING!\n\n");

    while(active == TRUE) {
        if (num_msgs > MAX_MSGS){
            printf("Max Messages reached. Quitting\n");
            break;
        }
        printf("Top of While Loop\n\n");

        /* Block until input arrives on one or more active sockets. */
        //read_fd_set = active_fd_set;

        /* Initialize the set of active sockets. */
        FD_ZERO (&active_fd_set);
        FD_SET (new_tcp_socket, &active_fd_set);
        FD_SET(udp_socket, &active_fd_set);


        if (select (FD_SETSIZE, &active_fd_set, NULL, NULL, NULL) < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }


        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {

            if (FD_ISSET (i, &active_fd_set)) {
                /* PROXY -- SERVER TCP CONNECTION I/O */
                if ( i == new_tcp_socket) {
                    printf("Server %d Received a message on TCP\n", pid);
                    fflush(stdout);

                    /*Check if it was for closing , and also read the incoming message_t*/
                    if ((valread = read( new_tcp_socket , buffer, 1024)) == 0) {
                        /*Somebody disconnected , get his details and print*/
                        int client_addr_len = sizeof(client_address);
                        getpeername(new_tcp_socket , (struct sockaddr*)&client_address , (socklen_t *) &client_addr_len);
                        printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(client_address.sin_addr) , ntohs(client_address.sin_port));
                        close( new_tcp_socket );
                    }
                    else if(valread == -1){
                        perror("TCP Read Error");
                    }
                    else {
                        buffer[valread] = '\0';
                        strcpy(incoming_message, buffer);

                        /* Process the client command */
                        if ((cmd_type = parse_input(incoming_message, cmd_buf)) == -1) {
                            printf("cmd error");
                        }

                        /*TODO: Handle malformed msg command */
                        if (cmd_type == CRASH) {
                            active = FALSE;
                            break;
                        } else if (cmd_type == GET) {
                            /* prepare chatlog to send. */
                            send_log(msg_log, num_msgs, chat_log_out);
                            printf("Filled Chatlog: %s\n", chat_log_out);

                            if (send(new_tcp_socket, chat_log_out, strlen(chat_log_out), 0) != strlen(chat_log_out)) {
                                printf("ChatLog Send wrong size\n");
                            }
                        } else if (cmd_type == MSG) {
                            /* New Message from client! */
                            /* Always new */
                            num_msgs += add_new_message(cmd_buf->msg, pid, local_seqnum, msg_log,
                                                        num_msgs, msg_ids, vector_clock, num_procs);
                            fill_message(out_peer_msg_buf, RUMOR, pid, pid, local_seqnum,
                                         vector_clock, cmd_buf->msg, num_procs);
                            local_seqnum++;

                            printf("Filled Message:\n");
                            print_message(out_peer_msg_buf, num_procs);

                            /* Send new message to nearby server */
                            struct sockaddr_in peer_serv_addr;
                            peer_serv_addr.sin_family = AF_INET;
                            peer_serv_addr.sin_addr.s_addr = INADDR_ANY;

                            neighbor_index = pick_neighbor(num_neighbors);

                            //printf("selected Neghbor number %d\n",i);
                            peer_serv_addr.sin_port = htons(neighbor_ports[neighbor_index]);

                            printf("Sending Message to neighbor on UDP port: %d to port: %d\n ", udp_port,
                                   ntohs(peer_serv_addr.sin_port));

                            sendto(udp_socket, (const char *) out_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT,
                                   (const struct sockaddr *) &peer_serv_addr, sizeof(peer_serv_addr));
                            printf("Message sent on UDP.\n\n");
                        }
                    }


                    /* SERVER -- SERVER UDP CONNECTION I/O */
                } else if(i == udp_socket){
                    printf("Server %d Received a message on UDP\n",pid);

                    struct sockaddr_in peer_serv_addr;
                    memset(&peer_serv_addr, 0, sizeof(peer_serv_addr));
                    char udp_buffer[1024];
                    int n, stat, msg_idx;
                    socklen_t len;

                    len = sizeof(peer_serv_addr);

                    if ((n = recvfrom(udp_socket, in_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT, ( struct sockaddr *) &peer_serv_addr, &len)) == -1) {
                        perror("Recv From Failed\n");
                        break;
                    }


                    printf("Read %d bytes from udp. From socket %d, on port %d\n", n, udp_socket, ntohs(peer_serv_addr.sin_port));
//                    printf("Read %d bytes from udp. Message is: \n", n);
//
//                    perror("is there an error?");
//                    printf("Read %d bytes from udp. Message is: \n", n);

//                    print_message(in_peer_msg_buf, num_procs);
//
//                    printf('bloop\n');
//
                    if (in_peer_msg_buf->type == 1){
                        printf("Schmang\n");
                    }
                    else{
                        printf("bang\n");
                    }
                    enum message_type inc_type = in_peer_msg_buf->type;
                    printf("type %d\n", inc_type);
                    if (inc_type == 1){
                        printf("Schmang");
                    }

                    if (inc_type == RUMOR){

                        printf(" Rumor Message Received from server %d\n", in_peer_msg_buf->from);

                        j = update_log(in_peer_msg_buf, msg_log, num_msgs, msg_ids, vector_clock, num_procs);
                        num_msgs+=j;

                        /* New Message: */
                        if (j == 1){
                            if(num_neighbors > 1) {
                                /* Resend Rumor to random neighbor if it is a new message*/
                                printf("I have more neighbors. Resend rumor here.\n");
                            }
                        }

                        /* Any Rumor message. Send an Ack*/

                        printf("Message is from server: %d, pid is %d. difference is %d\n",in_peer_msg_buf->from, pid, in_peer_msg_buf->from - pid);
                        j = get_neighbor_port_idx(in_peer_msg_buf->from, pid, num_neighbors);
                        printf("Neigher idx is: %d\n", j);

                        peer_serv_addr.sin_family = AF_INET;
                        peer_serv_addr.sin_addr.s_addr = INADDR_ANY;
                        peer_serv_addr.sin_port = htons(neighbor_ports[j]);

                        /* Send status ack. */
                        fill_message(out_peer_msg_buf, STATUS, pid, pid, 0, vector_clock, msg_log[0], num_procs);

                        printf("Sending ACK Message from UDP port: %d to port: %d\n ", udp_port, ntohs(peer_serv_addr.sin_port));

                        /* Send new message to nearby server */

                        sendto(udp_socket, (const char *) out_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT,
                               (const struct sockaddr *) &peer_serv_addr, sizeof(peer_serv_addr));
                        printf("Ack message sent on UDP.\n\n");
                    }

                    if (inc_type == STATUS){
                        printf("Status Mess received. From server: %d, seqnum: %d\n",in_peer_msg_buf->from, in_peer_msg_buf->seqnum);

                        /* Compare Vector Clocks */
                        if ((stat = read_status_message(next_msg, in_peer_msg_buf, vector_clock, num_procs)) == 1){
                            /* We have messages to send*/
                            /* send the next message back to that guy.*/

                            printf("Message is from server: %d, pid is %d. difference is %d\n",in_peer_msg_buf->from, pid, in_peer_msg_buf->from - pid);
                            j = get_neighbor_port_idx(in_peer_msg_buf->from, pid, num_neighbors);
                            printf("Neigher idx is: %d\n", j);

//                            struct sockaddr_in peer_serv_addr;
                            peer_serv_addr.sin_family = AF_INET;
                            peer_serv_addr.sin_addr.s_addr = INADDR_ANY;
                            peer_serv_addr.sin_port = htons(neighbor_ports[j]);

                            printf("Next Message is server %d, message %d\n", next_msg[0], next_msg[1]);
                            msg_idx = search_for_message(msg_ids, num_msgs,next_msg[0],next_msg[1]);
                            fill_message(out_peer_msg_buf, RUMOR, pid, next_msg[0], next_msg[1], vector_clock, msg_log[msg_idx], num_procs);

                            printf("Sending Message from UDP port: %d to port: %d\n ", udp_port, ntohs(peer_serv_addr.sin_port));

                            /* Send new message to nearby server */


                            sendto(udp_socket, (const char *) out_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT,
                                   (const struct sockaddr *) &peer_serv_addr, sizeof(peer_serv_addr));
                            printf("Test message sent on UDP.\n\n");

                        } else if (stat == -1){
                            /* we need messages.
                             * send a status message.
                             * */

                            printf("I need msgs\n");

                            printf("Message is from server: %d, pid is %d. difference is %d\n",in_peer_msg_buf->from, pid, in_peer_msg_buf->from - pid);
                            j = get_neighbor_port_idx(in_peer_msg_buf->from, pid, num_neighbors);
                            printf("Neigher idx is: %d\n", j);

//                            struct sockaddr_in peer_serv_addr;
                            peer_serv_addr.sin_family = AF_INET;
                            peer_serv_addr.sin_addr.s_addr = INADDR_ANY;
                            peer_serv_addr.sin_port = htons(neighbor_ports[j]);

                            fill_message(out_peer_msg_buf, STATUS, pid, pid, 0, vector_clock, msg_log[0], num_procs);

                            printf("Sending Message from UDP port: %d to port: %d\n ", udp_port, ntohs(peer_serv_addr.sin_port));

                            /* Send new message to nearby server */

                            sendto(udp_socket, (const char *) out_peer_msg_buf, sizeof(struct message), MSG_DONTWAIT,
                                   (const struct sockaddr *) &peer_serv_addr, sizeof(peer_serv_addr));
                            printf("Test message sent on UDP.\n\n");

                        }
                        else{
                            /* Nothing to do.
                             * flip a coin for more rumor mongering
                             * */
                            printf("Nothing to do\n");

                        }
                    }
                    printf("after the if statement\n");

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