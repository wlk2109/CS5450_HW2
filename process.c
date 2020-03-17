
#include "p2pApp.h"

int main(int argc , char *argv[])
{

    int pid = atoi(argv[1]);
    int num_procs = atoi(argv[2]);
    int tcp_port = atoi(argv[3]);
    int udp_port = ROOT_ID + pid;
    /** Server variables */
    struct sockaddr_in client_address;
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
    int tcp_socket, new_tcp_socket, udp_socket, i, cmd_type;
    int opt = TRUE;
    fd_set active_fd_set, read_fd_set;
    /** App Logic Variables*/
    /* Vector clock. with n entries
     * if vector_clock[i] == x:
     * lowest message NOT SEEN from peer i is x
     * */
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
    struct message_t *peer_msg_buf = malloc(sizeof(message_t));

    /* Init vector clock with 1's*/
    for (i=0; i<num_procs; i++){
        vector_clock[i] = (uint16_t) 1;
    }

    /** P2P sending stuff*/

    /*** TESTING ***/
    char fake_cmd[250];
    /*testing parse_input

    strcpy(fake_cmd, "msg 12 chatLog for me and you");

    cmd_type = parse_input(fake_cmd, cmd_buf);

    printf("The command message_t received is of type %d. \n",cmd_buf->cmd_type);
    if (cmd_type == MSG){
        printf("Message ID: %d, Message: %s\n\n",cmd_buf->msg_id, cmd_buf->msg);
    }
    */

    strcpy(fake_cmd, "This is a test message my hombre");
    /* test build message_t */
    fill_message(peer_msg_buf, RUMOR, pid, 1, vector_clock, fake_cmd, num_procs);

    /* test update log*/
    update_log(peer_msg_buf, msg_log, num_msgs, msg_ids, vector_clock, num_procs);

    /*** END TESTING ***/



    /*Create a TCP socket*/
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP Socket failed");
        exit(EXIT_FAILURE);
    }
    printf("TCP Socket Made\n");
    fflush(stdout);

    /*Create a UDP socket*/
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP Socket failed");
        exit(EXIT_FAILURE);
    }

    if((setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt))) < 0)
    {
        perror("Server-setsockopt() error");
        close(tcp_socket);
        exit (-1);
    }

    /*Create TCP address */
    memset(&tcp_address, 0, sizeof(struct sockaddr_in));
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(tcp_port);

    printf("Server added port %d to struct\n", ntohs(tcp_address.sin_port));

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
    printf("TCP Bind Successful on port %d \n", ntohs(tcp_address.sin_port));
    fflush(stdout);

    /*try to specify maximum of 3 pending connections for the master socket*/
    if (listen(tcp_socket, 3) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("TCP Listen\n");
    printf("Listener on port %d \n", ntohs(tcp_address.sin_port));
    fflush(stdout);

    if ((new_tcp_socket = accept(tcp_socket, (struct sockaddr *) &client_address, (socklen_t *) &tcp_addr_len)) < 0) {
        perror("accept");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    /*inform user of socket number - used in send and receive commands*/
    printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_tcp_socket,
           inet_ntoa(tcp_address.sin_addr), ntohs(tcp_address.sin_port));

    char *message = "TCP Connection Established between proxy and server\r\n";
    fflush(stdout);

    /*send new connection greeting message_t*/
    if (send(new_tcp_socket, message, strlen(message), 0) != strlen(message)) {
        perror("inital TCP connection send error");
    }

    /*Bind the UDP socket to localhost and port with PID + 20000*/
    if (bind(udp_socket, (struct sockaddr *) &udp_address, sizeof(udp_address)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }
    printf("UDgetP Bind Successful on port %d \n", ntohs(udp_address.sin_port));
    fflush(stdout);

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (new_tcp_socket, &active_fd_set);
    FD_SET(udp_socket, &active_fd_set);

    while(TRUE) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror ("select");
            exit (EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                if ( i == new_tcp_socket){
                    /* This is where we write stuff about proxy to server */
                    printf("OMG GOT A TCP THAAAAANG\n");
                    fflush(stdout);
                } else {
                    /* This is where we write stuff about proxy to server */
                    printf("DAMN WE GOT A DIF THANG\n");
                    fflush(stdout);
                }
            }
        }
    }
}
