
#include "p2pApp.h"

int main(int argc , char *argv[])
{
    int pid = atoi(argv[1]);
    int num_procs = atoi(argv[2]);
    int tcp_port = atoi(argv[3]);
    int udp_port = 20000 + pid;


    struct sockaddr_in client_address;
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
    int tcp_socket, new_tcp_socket, udp_socket, valread, i;
    char buffer[1025];

    fd_set active_fd_set;
    fd_set read_fd_set;

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
        perror("Intial TCP connection send error");
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

                    /*Echo back the message_t that came in*/
                    else {
                        buffer[valread] = '\0';
                        send(new_tcp_socket , buffer , strlen(buffer) , 0 );
                    }
                    /* Send new message to nearby server */
                    //TODO: Make all the below stuff into a function under gossip stuff
                    char *test_msg = "This message should be sent over UDP";
                    struct sockaddr_in test_serv_addr;
                    test_serv_addr.sin_family = AF_INET;
                    test_serv_addr.sin_addr.s_addr = INADDR_ANY;
                    int test_serv_port = get_open_neighbors(pid);
                    test_serv_addr.sin_port =htons(test_serv_port);

                    sendto(udp_socket, (const char *)test_msg, strlen(test_msg), MSG_DONTWAIT, (const struct sockaddr *) &test_serv_addr, sizeof(test_serv_addr));
                    printf("Test message sent on UDP.\n");

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

                    if (n = recvfrom(udp_socket, (char *)udp_buffer, 1024, MSG_DONTWAIT, ( struct sockaddr *) &peer_serv_addr, &len) == -1) {
                        perror("Recv From Failed\n");
                    }

                    udp_buffer[n] = '\0';
                    printf("Client : %s\n", udp_buffer);
                }
            }
        }
    }
}

int get_open_neighbors(int pid) {
    //TODO: Write proper logic here to find nearby neighbours
    //For now just writing test code
    if (pid == 1) {
        return 20002;
    } else {
        return 20001;
    }
}
