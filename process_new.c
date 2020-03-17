
#include "p2pApp.h"

int main(int argc , char *argv[])
{
    int pid = atoi(argv[1]);
    int num_procs = atoi(argv[2]);
    int tcp_port = atoi(argv[3]);
    int udp_port = 20011 + pid;

    printf("TCP Port %d \n", tcp_port);

    struct sockaddr_in client_address;
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
    int tcp_socket, new_tcp_socket, udp_socket, i;
    int opt = TRUE;

    fd_set active_fd_set;
    fd_set read_fd_set;

    printf("In Process New\n");
    fflush(stdout);

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
    else
        printf("Server-setsockopt() is OK\n");
        fflush(stdout);

    /*Create TCP address */
    memset(&tcp_address, 0, sizeof(struct sockaddr_in));
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_address.sin_port = htons(tcp_port);

    printf("Server added port %d to struct\n", ntohs(tcp_address.sin_port));

    /*Create UDP address */
    memset(&tcp_address, 0, sizeof(struct sockaddr_in));
    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_address.sin_port = htons(udp_port);

    int tcp_addr_len = sizeof(tcp_address);

    /*Bind the TCP socket to localhost port specified in start command*/
    if (bind(tcp_socket, (struct sockaddr *) &tcp_address, sizeof(tcp_address)) < 0) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }
    printf("TCP Bind Successful on port %d \n", tcp_address.sin_port);
    fflush(stdout);

    /*try to specify maximum of 3 pending connections for the master socket*/
    if (listen(tcp_socket, 3) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("TCP Listen\n");
    printf("Listener on port %d \n", tcp_address.sin_port);
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
