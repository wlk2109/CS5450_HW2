#include "gbn.h"

state_t s;

uint16_t checksum(uint16_t *buf, int nwords)
{
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){
	printf("\n<---------------------- GBN_SEND() ---------------------->\n\n");

	s.sender = TRUE;

	/* Get total number of packets*/
	int num_packets = 1; /* Min Number of packets */
	int remainder = len % DATALEN;
	int ack_count = 0;

	if (len > DATALEN){
		num_packets = len / DATALEN;
		if (remainder >0) {
			num_packets++;
		}

	}


	printf("Sending %d bytes of data. Total Packets = %d.\n", len, num_packets);

	
	/* Iterate when receiving a valid acknowledgement.
	 * track initial sequence number (in the case of a cumulative ack)
	 */
	uint16_t packets_sent = 0; /* Count of packets sent SUCCESSFULLY */
	uint32_t initial_seq_num = s.seq_num+1; /* First Non-Acked SeqNum */
	uint32_t most_recent_ack;
	uint32_t target_ack = packets_sent + s.window_size; /* Ack for window expansions*/
	uint8_t packets_out = 0; /* Number of outstanding packets */
	uint8_t attempts = 0;
	uint32_t expected_ack = initial_seq_num;
	int timeout_count = 0;


	/* Allocate memory for incoming ack packet 
	 * Should only need 1 because ACKs are coming in 1 at a time.
	 */
	 gbnhdr *DATAACK_packet = alloc_pkt();


	/* Allocate Memory for outgoing packets
	 * Use array of packet headers *
	 * Number of elements is the number of packets TOTAL
	 */
	gbnhdr *outgoing_packets[num_packets];

		int i;
		for (i = 0; i < num_packets; i++){
			outgoing_packets[i] = alloc_pkt();
				/* Build Payload packets 
				* Can simply start buffer at correct position, since build data packet writes 1024 each time.
				*/
			if (i == num_packets -1 && remainder >0){
				build_data_packet(outgoing_packets[i], DATA, initial_seq_num + i, buf+i*DATALEN, remainder);
			}
			else{
				build_data_packet(outgoing_packets[i], DATA, initial_seq_num + i, buf+i*DATALEN, 1024);
			}
		}

	while(TRUE){
		
		printf(" Total Packets Sent %d, Num Packets %d \n", packets_sent, num_packets);
		if (packets_sent == num_packets){
			printf("All Packets sent\n");
			break;
		}

		/* Send packets.
		* Track number of outstanding packets. Make less than or equal to window size.
		*/
		printf("\n<---------------------- Sending Packets ---------------------->\n\n");

		if (s.window_size == 0){
			s.window_size = 1;
		}

		while(packets_out < s.window_size && (packets_out + packets_sent) < num_packets){
			
			if (attempts >= MAX_ATTEMPTS){
				perror("Max Attempts exceded. Exiting.\n");
				int j;
				for (j = 0; j < num_packets; j++){
					free(outgoing_packets[j]);
				}
				free(DATAACK_packet);
				return (-1);
			}
			
			gbnhdr *out_pkt = outgoing_packets[packets_sent + packets_out];
			int i;
			for(i =0; i<2; i++){
				if(maybe_sendto(sockfd, out_pkt, sizeof(*out_pkt), flags, &s.address, s.sock_len) == -1){
					printf("Send to Failed, retrying\n");
					/* Reset Timeout*/
					attempts++;
					continue;
				}
			}

			/*printf("Successfully Sent Packet %d. Sequence Num: %d\n", packets_sent + packets_out, out_pkt->seqnum);*/

			attempts = 0;
			packets_out++;

		}
		/*
		printf("Total Packets Outstanding: %d\n", packets_out);
		printf("starting alarm\n");
		*/
		alarm(TIMEOUT);

		/*printf("Outstanding Packets saturated\n");*/
		printf("Packets Sent: %d. Packets out: %d. Num_packets: %d. Window_size: %d", packets_sent, packets_out, num_packets, s.window_size);
		
		printf("\n<---------------------- Receiving Acks ---------------------->\n\n");
		
		attempts = 0;
		while(TRUE){
			


			if (attempts >= MAX_ATTEMPTS || timeout_count >= MAX_ATTEMPTS){
				printf("timeouts %d, attempts %d\n",timeout_count, attempts);
				perror("Max Attempts exceded. Exiting.\n");
				int j;
				for (j = 0; j < num_packets; j++){
					free(outgoing_packets[j]);
				}
				free(DATAACK_packet);
					return (-1);
			}
			
			/*printf("Attempt %d. Expecting DATAACK %d, Last Acked sequnece number %d\n",attempts, expected_ack, s.seq_num);*/

			size_t recvd_bytes = maybe_recvfrom(sockfd, DATAACK_packet, sizeof(*DATAACK_packet), flags, &s.address, &s.sock_len);
			if(recvd_bytes == -1){
				if (errno != EINTR) {
					perror("Recv From Failed. Retrying\n");
					/* Attempt again */
					attempts++;
					continue;
					}
				else{
					perror("Timeout");
					printf("Reducing window_size, resending packets\n");
					timeout_count++;
					s.window_size = s.window_size/2;
					packets_out = 0;	
					break;
				}
			}
			/* Reset Attempts */
			attempts = 0;
			alarm(0);
			timeout_count = 0;

			/* printf("Received Packet. Type: %d, seq_num %d\n", DATAACK_packet->type, DATAACK_packet->seqnum);
			 * printf("Target Packet: %d, Seq_num = %d\n\n", expected_ack, (uint8_t) expected_ack);
			 */

			/* Process Ack */
			if ((validate(DATAACK_packet) == TRUE) && DATAACK_packet->type == DATAACK){
			
				uint8_t recv_seqnum = DATAACK_packet ->	seqnum;

				if (recv_seqnum >= (uint8_t) expected_ack || recv_seqnum <= (uint8_t)expected_ack+MAX_WINDOW_SIZE){

					/*  printf("Seq Num Received: %d. Expected Ack %d\n\n",recv_seqnum,(uint8_t)expected_ack);
					* If Ack is good: 
					*
					* -Increment packets_sent
					* -Increment expected seq_num
					* -If Target sequence number reached, increase window size
					* -Decrement Outstanding Packets 
					* -Reset Timer, Break and send more packets
					*
					* Window Size:
					* Sliding: send new packets as acks come in and keep track of target_ack for window size increase
					*/

					alarm(0);
					timeout_count = 0;

					/* Handle Cumulative ack*/
					uint8_t packets_acked = (recv_seqnum - (uint8_t) expected_ack)  + 1;
					packets_sent += packets_acked;
					packets_out -= packets_acked;
					most_recent_ack = s.seq_num;
					s.seq_num += packets_acked;
					expected_ack = s.seq_num +1;
					printf("Total Packets Acked: %d. New packets Out: %d\n", packets_acked, packets_out);

					if (s.seq_num >= target_ack){
						// printf("Received target ack %d\n", target_ack);
						s.window_size = 2*s.window_size;
						target_ack = most_recent_ack + s.window_size; 
						// printf("Increasing Window Size to%d\n",s.window_size);
					}

					break;

				}
				else{
				/* If Ack is bad/duplicate Ack).
				 * 1. Reduce window size.
				 * 2. Set packets_out to 0.
				 * 3. Break and resend all packets
				 */ 
					alarm(0);
					timeout_count = 0;
					if (recv_seqnum == (uint8_t) expected_ack){
						ack_count++;
						if (ack_count<3){
							break;
						}
					}
					printf("Reducing window size\n");
					if (s.window_size >1){
						s.window_size = s.window_size/2;
					}
					packets_out = 0;
					break;
				}
			}
			/* printf("Invalid/Corrupted DATAACK Packet, retrying receive\n"); */

			/* corrupted Ack: Try again if window size sufficiently large */
			if(s.window_size<4){
				/*printf("Reducing window_size, resending packets\n");*/
				if (s.window_size >1){
					s.window_size = s.window_size/2;
					}
				timeout_count = 0;
				packets_out = 0;
			}
			break;
		}
	}

	int j;
	for (j = 0; j < num_packets; j++){
		free(outgoing_packets[j]);
	}
	free(DATAACK_packet);
	return (DATALEN*N);
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){
	printf("\n<---------------------- GBN_RECV() ---------------------->\n\n");

	s.sender = FALSE;
	int first_packet = FALSE;
	
	ssize_t payload_len;
	
	/* track expected sequence number */
	
	uint8_t expected_seq_num = (uint8_t) s.seq_num + 1;
	
	printf("Expected packet seq_num: %d (should be %d)\n", expected_seq_num, (uint8_t)s.seq_num+1);
	/* Allocate packet to receive? */

	gbnhdr *incoming_packet = alloc_pkt();

	int attempts = 0;
	ssize_t recvd_bytes;
	int proper_read = FALSE;

	/*  allocate an ack packet, to be filled later. */
	gbnhdr *ACK_packet = alloc_pkt();
	
	while (s.current_state == ESTABLISHED){

		if (attempts >= MAX_ATTEMPTS){
			/* Max Attempts exceeded */
			perror("Maximum attempts exceded. Exiting\n");
			free(incoming_packet);
			free(ACK_packet);
			return(-1);
		}
		

		recvd_bytes = maybe_recvfrom(sockfd, incoming_packet, sizeof(*incoming_packet), flags, &s.address, &s.sock_len);
		if(recvd_bytes == -1){
			if (errno != EINTR) {
				perror("Recv From Failed\n");
				/* Attempt again */
				attempts++;
				continue;
				}
			else{
				perror("Timeout Occured");
			}
		}
		attempts = 0;
		
		printf("Received packet: %d. Expected Packet: %d packet_type = %d\n", incoming_packet->seqnum, expected_seq_num, incoming_packet->type);

		if(validate(incoming_packet)==1){

			/* Check Packet Type only if packet is not corrupted. */
			if (incoming_packet->type != DATA){
				if (incoming_packet->type == FIN) {
					printf("FIN Packet Received. Changing State.");
					s.current_state = FIN_RCVD;
					free(incoming_packet);
					free(ACK_packet);
					return 0;
				}
				perror("Incorrect Packet Type Received. Connection out of Synch. Exiting.\n");
				free(incoming_packet);
				free(ACK_packet);
				return(-1);
			}

			/* validate SeqNum*/
			if (incoming_packet->seqnum == expected_seq_num){
				s.seq_num++;
				payload_len = incoming_packet->payload_len;
				build_empty_packet(ACK_packet, DATAACK, expected_seq_num);

				memcpy(buf, incoming_packet->data, payload_len);
				proper_read = TRUE;
				printf("\nCopied %d bytes to buf and increased seq_num to %d\n\n", payload_len, s.seq_num);
			}
			else{
				/* Packet out of order. */
				build_empty_packet(ACK_packet, DATAACK, s.seq_num);
				/*printf("Packet is Out of Order. Sending old acknowledgement %d\n", ACK_packet->seqnum);*/
			}
		}
		else{
			/* Packet is corrupted, send old ack. */
			build_empty_packet(ACK_packet, DATAACK, s.seq_num);
			/*printf("Packet is Corrupted. Sending old acknowledgement %d\n", ACK_packet->seqnum);*/
		}

		/* Send Acknowledgement */
		while (TRUE){
			if (attempts>=MAX_ATTEMPTS){
				perror("Max attempts exceed on sending ack. Connection compromised.\n");
				free(incoming_packet);
				free(ACK_packet);
				return(-1);
			}

			if(maybe_sendto(sockfd, ACK_packet, sizeof(*ACK_packet), flags, &s.address, s.sock_len) == -1){
				printf("Sendto Failed, retrying\n");
				attempts++;
				continue;
			}
			attempts = 0;
			break;
		}
		/*printf("Send To Succeeded. ACK sent. Ack Packet: %d, Checksum: %d\n\n", ACK_packet->seqnum, ACK_packet->checksum);*/

		if(proper_read){
			/* free memory */
			free(incoming_packet);
			free(ACK_packet);
			/* return bytes recieved */
			return(payload_len);
		}
	}

	/* free memory, turn off timer */
	free(incoming_packet);
	free(ACK_packet);
	
	return(-1);
}

int gbn_close(int sockfd){
	printf("\n<---------------------- GBN_CLOSE() ---------------------->\n\n");

	/*printf("gbn_close() called. socket: %d\n", sockfd); 
     *printf("Current state: %d\n", s.current_state); 
	 */

	/* logic for sender */
	if (s.sender == TRUE) {
		
		int counter = 0;

		gbnhdr *FIN_pkt = alloc_pkt();
		build_empty_packet(FIN_pkt, FIN, s.seq_num+1);

		gbnhdr *FINACK_pkt = alloc_pkt();

		printf("FIN and FINACK packet created\n");

		while (TRUE) {

			if (counter >= MAX_ATTEMPTS) {
				printf("Reached Max attempts");
				s.current_state = CLOSED;
				free(FIN_pkt);
				free(FINACK_pkt);
				return(-1);
			}

			if (s.current_state != CLOSED) {

				printf("Sending FIN packet. Seqnum: %d\n", FIN_pkt->seqnum);
				counter++; 

				if (maybe_sendto(sockfd, FIN_pkt, sizeof(FIN_pkt), 0, &s.address, s.sock_len) == -1) {
					perror("Sending FIN Packet returned an error\n");
					free(FIN_pkt);
					free(FINACK_pkt);
					continue;
				}

				s.current_state = FIN_SENT;
				printf("Changed current state to FIN_SENT now waiting for FIN_ACK\n");
			}
			
			if (s.current_state == FIN_SENT) {
			
				alarm(TIMEOUT);

				if (maybe_recvfrom(sockfd, FINACK_pkt, sizeof(FINACK_pkt), 0, &s.address, &s.sock_len) == -1) {
					if (errno == EINTR) {
						perror("TIMEOUT error in waiting for FINACK packet\n");
					} else {
						perror("Error in recieving FINACK packet\n");
					}
					continue;
				}
			}
			if ((FINACK_pkt->type == FINACK) && (validate(FINACK_pkt) == TRUE)) {
				alarm(0);
				printf("Recieved FINACK packet successfully\n");
				free(FIN_pkt);
				free(FINACK_pkt);
				s.current_state = CLOSED;
				return 1; 
			}
		}	
	/* logic for receiver */
	} else {

		gbnhdr *FINACK_pkt = alloc_pkt();
		build_empty_packet(FINACK_pkt, FINACK, s.seq_num);

		gbnhdr *FIN_pkt = alloc_pkt();
		
		if (s.current_state == FIN_SENT) {
			int counter = 0;

			while(TRUE){
				
				if (counter >= MAX_ATTEMPTS) {
					free(FINACK_pkt);
					free(FIN_pkt);
					printf("Reached Maximum attempts");
					s.current_state = CLOSED;
					return -1;
				}

				alarm(TIMEOUT);
				
				counter++;
				if (maybe_recvfrom(sockfd, FIN_pkt, sizeof(FIN_pkt), 0, &s.address, s.sock_len) == -1) {
					
					if (errno == EINTR) {
						perror("TIMEOUT error in waiting for FINACK packet\n");
					} else {
						perror("Error in recieving FINACK packet\n");
					}
					continue;
				}

				if ((FIN_pkt->type == FIN) && (validate(FIN_pkt) == TRUE)) {
					alarm(0);
					printf("Recieved FIN packet successfully\n");
					s.current_state = FIN_RCVD;
				}
			}
		}

		if (s.current_state == FIN_RCVD) {
			int counter = 0; 
			
			while(TRUE) {
				if (counter >= MAX_ATTEMPTS) {
					free(FINACK_pkt);
					free(FIN_pkt);
					printf("Reached Maximum attempts");
					s.current_state = CLOSED;
					return -1;
				}

				/*printf("Sending FINACK packet. Seqnum: %d\n", FINACK_pkt->seqnum);*/

				counter++; 

				if (maybe_sendto(sockfd, FINACK_pkt, sizeof(FINACK_pkt), 0, &s.address, s.sock_len) == -1) {
					perror("Sending FINACK Packet returned an error\n");
					free(FIN_pkt);
					free(FINACK_pkt);
					continue;
				}

				s.current_state = CLOSED; 
				free(FIN_pkt);
				free(FINACK_pkt);
				return 1;
			}
		}
	}
	return(-1);
}

int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen) {

	/* 1. Create SYN Packet
	   2. Send SYN Packet to reciever and move to SYN_SENT
	   3. Wait for SYNACK and update state machine to ESTABLISHED
	   ?. Handle rejection 
	   ?. Handle Timeout
	   ?. Handle Max Attempts */
	
	printf("\n<---------------------- GBN_CONNECT() ---------------------->\n\n");

	printf("gbn_connect() called. socket: %d, server address: %d socklen: %d\n", sockfd, server->sa_family, socklen);
    printf("Current state: %d\n", s.current_state);

	/* Create SYN Packet */
	gbnhdr *SYN_pkt = alloc_pkt();
	build_empty_packet(SYN_pkt, SYN , s.seq_num);
	
	printf("Creating SYNACK\n");

	/* Create SYNACK Packet */
	gbnhdr *SYNACK_pkt = alloc_pkt();

	printf("Counter = 0\n");
	/* Begin trying to establish connection */
	int counter = 0;

	/* Try 5 attempts max */
	while(TRUE) {

		if (counter >= MAX_ATTEMPTS){
			printf("Reached Max attempts\n");
            s.current_state = CLOSED;
            free(SYN_pkt);
            free(SYNACK_pkt);
            return(-1);
		}

		if (s.current_state == CLOSED) {

			/*Try sending packet to establish connection */
			printf("Sending SYN packet. Seqnum: %d\n", SYN_pkt->seqnum);

			if (maybe_sendto(sockfd, SYN_pkt, sizeof(SYN_pkt), 0, server, socklen) == -1) {
				perror("Sending SYN Packet returned an error\n");
				return(-1);
			}

			/* Change state to sent */
			s.current_state = SYN_SENT;
			
			printf("Changed current state to SYN_SENT now waiting for SYN_ACK\n");
		}
		
		if (s.current_state == SYN_SENT) {
			
			counter++;
			alarm(TIMEOUT);

			if (maybe_recvfrom(sockfd, SYNACK_pkt, sizeof(SYNACK_pkt), 0, server, &socklen) == -1) {
				
				if (errno == EINTR) {
					perror("TIMEOUT error in waiting for SYNACK packet\n");
				} else {
					perror("Error in recieving SYNACK packet\n");
				}
				
				s.current_state = CLOSED;
				continue;
			}
		}

		if ((SYNACK_pkt->type == SYNACK) && (validate(SYNACK_pkt) == TRUE)) {

			alarm(0);
			printf("Recieved SYNACK packet successfully\n");

			s.address =  *server;
			s.sock_len = socklen;
			s.current_state = ESTABLISHED;
			s.seq_num = SYN_pkt->seqnum;
			s.current_state = ESTABLISHED;

			printf("Connection Established with server\n");
			free(SYN_pkt);
			free(SYNACK_pkt);

			return sockfd;
		}
	}
	return(-1);
}

int gbn_listen(int sockfd, int backlog){

	/* No "Listening" in UDP sockets */
	
	printf("'Listening'. sockfd: %d, backlog: %d \n", sockfd, backlog);

	/* Set the status to closed, since there is no connection active if we are listening*/
	s.current_state = CLOSED;

	return(0);
}

int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int bind_status;

    printf("Binding(),  sockfd: %d\n", sockfd);

	/*
	WHERE DOES THIS GO?
	int yes=1;

	lose the pesky "Address already in use" error message
	if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
 		perror("setsockopt");
    	exit(1);
	} 

	*/

    if ((bind_status = bind(sockfd, server, socklen)) == -1) {
        perror("Bind error");
        return(-1);
    }

	return bind_status;

	/* return(-1); */
}	

int gbn_socket(int domain, int type, int protocol){

	/*----- Randomizing the seed. This is used by the rand() function -----*/
	srand((unsigned)time(0));

	printf("Making Socket. domain: %d, type %d, protocol %d\n", domain, type, protocol);

	/* Initialize a random seq number used when sending or receiving packets */
    s.seq_num = (uint8_t)rand();

	/* Set initial window size to 1 (2^0)*/
    s.window_size = 1;

    /* initialize signal handler	*/
	signal(SIGALRM, timeout_hdler);
	siginterrupt(SIGALRM,1);

    printf("Seq num: %d, Window size: %d\n", s.seq_num, s.window_size);

    int sockfd = socket(domain, type, protocol);

	printf("Socket created %d\n", sockfd);

	return sockfd;
}

int gbn_accept(int sockfd, struct sockaddr *client, socklen_t *socklen){

	printf("\n<---------------------- GBN_ACCEPT() ---------------------->\n\n");

	/* Called by receiver */

	printf("gbn_accept() called. socket: %d, client address: %d\n", sockfd, client->sa_family);

    printf("Current state: %d\n", s.current_state);

	/* 1. Wait for a SYN Packet to arrive
	   2. Send a SYN_ACK Packet back. 
	   3. Upon Successful complete of these steps, update state machine to be connected. 
	   ?. Handle rejection 
	   ?. Handle Timeout
	   ?. Handle Max Attempts */

	/* Allocate memory for a SYN packet to be received*/
	gbnhdr *incoming_pkt = alloc_pkt();

	int attempts = 0;
	ssize_t recvd_bytes;

	/* Check Attempts Number*/
	while(attempts < MAX_ATTEMPTS){

		
		/* Wait for the SYN Packet, if socket is currently closed. */ 
		if (s.current_state == CLOSED){

			alarm(TIMEOUT*6);
			attempts++;
			printf("Waiting for SYNthia...\n Attempt #: %d\n", attempts);

			if ((recvd_bytes = maybe_recvfrom(sockfd, incoming_pkt, sizeof(incoming_pkt), 0, client, socklen)) == -1){
				/* Maybe_recvfrom failed, TIMEOUT did not occur */
				if (errno != EINTR) {
					perror("Recv From Failed\n");
					s.current_state = CLOSED;
					/* Attempt again */
					continue;
				}
			}
			/* Received data successfully */
			/* Turn off alarm*/
			alarm(0);

			printf("\nPacket received! Bytes: %d, Packet Type: %d\n", (int)recvd_bytes, incoming_pkt->type);
			
		}
		else{
			/* Connection isn't closed */
			printf("Rejecting connection. Socket already in use\n");
			attempts = 0;
			gbnhdr* RST_packet = alloc_pkt();
			build_empty_packet(RST_packet, RST, s.seq_num);

			if (maybe_sendto(sockfd, RST_packet, sizeof(RST_packet), 0, client, *socklen) == -1){
				/* Maybe_sendto failed, TIMEOUT did not occur */
					perror("sendto Failed.\n");
					/* Attempt again */
					attempts++;
					if (attempts == MAX_ATTEMPTS){
						/* Too Many Attempts*/
						printf("Sendto max attempts exceeded. Connection closed. \n");
						s.current_state = CLOSED;
						s.seq_num = 0;

					}
			}
			free(incoming_pkt);
			free(RST_packet);
			return(-1);
		}

		/* Validate */
		if ((incoming_pkt->type == SYN) && (validate(incoming_pkt) == TRUE)) {
			
			printf("SYN City\n");

			s.current_state = SYN_RCVD;
			s.seq_num = incoming_pkt -> seqnum;

			
			/* Create a SYN_ACK Packet to be sent */
			printf("Making SYN_ACK packet\n");
			gbnhdr* SYNACK_packet = alloc_pkt();
			build_empty_packet(SYNACK_packet, SYNACK, s.seq_num);

			/* Send SYN_ACK*/
			attempts = 0;
			while(attempts < MAX_ATTEMPTS){

				if (maybe_sendto(sockfd, SYNACK_packet, sizeof(SYNACK_packet), 0, client, *socklen) == -1){
				/* Maybe_sendto failed, TIMEOUT did not occur */
					perror("sendto Failed.\n");
					/* Attempt again */
					attempts++;
					if (attempts == MAX_ATTEMPTS){
						/* Too Many Attempts*/
						printf("Sendto max attempts exceeded. Connection closed. \n");
						s.current_state = CLOSED;
						s.seq_num = 0;

						free(incoming_pkt);
						free(SYNACK_packet);
						return(-1);
					}
					continue;
				}
				break;
			}

            s.current_state = ESTABLISHED;
            s.address = *client;
            s.sock_len = *socklen;

            printf("Current state ESTABLISHED: %d\n", s.current_state);

            free(incoming_pkt);
            free(SYNACK_packet);
            return sockfd;
		}
		else{
			printf("Invalid SYN Packet Received. Retrying.\n");
			continue;
		}

	}
	printf("\nMax Attempts exceeded. Connection Broken. Exiting\n");
	s.current_state = CLOSED;

	free(incoming_pkt);
	return(-1);

}

ssize_t maybe_recvfrom(int  s, char *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen){

	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){


		/*----- Receiving the packet -----*/
		int retval = recvfrom(s, buf, len, flags, from, fromlen);

		/*----- Packet corrupted -----*/
		if (rand() < CORR_PROB*RAND_MAX){
			/*----- Selecting a random byte inside the packet -----*/
			int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

			/*----- Inverting a bit -----*/
			char c = buf[index];
			if (c & 0x01)
				c &= 0xFE;
			else
				c |= 0x01;
			buf[index] = c;
		}

		return retval;
	}
	/*----- Packet lost -----*/
	return(len);  /* Simulate a success */
}

ssize_t maybe_sendto(int  s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){

    char *buffer = malloc(len);
    memcpy(buffer, buf, len);
    
    
    /*----- Packet not lost -----*/
    if (rand() > LOSS_PROB*RAND_MAX){
        /*----- Packet corrupted -----*/
        if (rand() < CORR_PROB*RAND_MAX){
            
            /*----- Selecting a random byte inside the packet -----*/
            int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

            /*----- Inverting a bit -----*/
            char c = buffer[index];
            if (c & 0x01)
                c &= 0xFE;
            else
                c |= 0x01;
            buffer[index] = c;
        }

        /*----- Sending the packet -----*/
        int retval = sendto(s, buffer, len, flags, to, tolen);
        free(buffer);
        return retval;
    }
    /*----- Packet lost -----*/
    else
        return(len);  /* Simulate a success */
}

gbnhdr *alloc_pkt(){
	/* Allocated memory for an incoming packet */
	gbnhdr *packet = malloc(sizeof(*packet));
	/* Set data to 0's */
	memset(packet, 0, sizeof(*packet));
	return(packet);
}

uint8_t validate(gbnhdr *packet){
	uint16_t rcvd_checksum = packet->checksum;
	packet->checksum = (uint16_t)0;
	uint16_t pkt_checksum = checksum((uint16_t  *)packet, sizeof(*packet) / sizeof(uint16_t));
	/*printf("rcvd checksum: %d. calculated checksum: %d\n", rcvd_checksum, pkt_checksum);*/
	if (rcvd_checksum == pkt_checksum){
		return(TRUE);
	}	
	return (FALSE);
};

void build_data_packet(gbnhdr *data_packet, uint8_t pkt_type ,uint32_t pkt_seqnum, const void *buffr, size_t len){

	/* Construct a packet */
	/* Memory Already Allocated */
	/* Zero out data  and checksum */
	memset(data_packet->data, 0, sizeof(data_packet->data));
	data_packet->checksum = (uint16_t)0;
	/* Set Packet type  */
	data_packet->type = pkt_type;
	/* Set Packet Seqnum */
	data_packet->seqnum = (uint8_t)pkt_seqnum;
	/* Copy Data from buff*/
	memcpy(data_packet->data, buffr, len);
	data_packet->payload_len = len;

	/* Add Checksum*/
	uint16_t calc_checksum = checksum((uint16_t  *)data_packet, sizeof(*data_packet) / sizeof(uint16_t));

	data_packet->checksum = calc_checksum;
}

void build_empty_packet(gbnhdr *data_packet, uint8_t pkt_type ,uint32_t pkt_seqnum){
	/* Construct a packet */

	/* Zero out checksum */
	data_packet->checksum = (uint16_t)0;
	data_packet->payload_len = (uint16_t)0;
	/* Set Packet type  */
	data_packet->type = pkt_type;

	/* Set Packet Seqnum */
	data_packet->seqnum = pkt_seqnum;

	/* Add Checksum*/
	uint16_t calc_checksum = checksum((uint16_t  *)data_packet, sizeof(*data_packet) / sizeof(uint16_t));

	data_packet->checksum = calc_checksum;
}

void timeout_hdler(int signum) {
    printf("\nTIMEOUT has occured with signum: %d\n", signum);
    signal(SIGALRM, timeout_hdler);
}

