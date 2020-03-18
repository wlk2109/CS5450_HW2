
#include "p2pApp.h"

/*
 * Get a message_t from the Client.
 * Parse the message_t command.
 *
 * Fill client_command
 *
 */
int parse_input(char *cmd_string, client_command *client_cmd){

    printf("parse_input - Incoming: %s\n", cmd_string);

    char* token = strtok(cmd_string, " ");

    /* msg <messagID> <message_t>
     * crash
     * get chatLog
     */

    if (strcmp(token, "msg")==0){

        client_cmd ->cmd_type = MSG;

        token = strtok(NULL, " ");
        printf("new token: %s\n", token);

        long m_id= strtol(token, NULL, 10);
        printf("Message ID: %ld\n",m_id);
        client_cmd->msg_id = (uint16_t) m_id;

        token = strtok(NULL, "\0");
        printf("Message: %s\n", token);

        //client_cmd->msg[strlen(client_cmd->msg)-1] = '\0';

        strcpy(client_cmd->msg, token);

        return MSG;
    }
    else if (strcmp(token, "get")==0){
        printf("get request\n");
        client_cmd->cmd_type = GET;
        return GET;
    }
    else if (strcmp(token, "crash")==0){
        printf("crash\n");
        client_cmd->cmd_type=CRASH;
        return CRASH;
    }
    else{
        perror("Incorrect command type");
        return -1;
    }
}

int pick_neighbor(int num_neighbors){
    if (num_neighbors == 1){
        return 0;
    }
    return rand();
}

int init_neighbors(int pid, int num_procs, int *potential){
    int num_neighbors = 0;
    if (pid > num_procs){
        return 0;
    }
    if (pid > 0){
        potential[0]=TRUE;
        num_neighbors++;
    }
    if (pid < num_procs - 1){
        potential[1] = TRUE;
        num_neighbors++;
    }
    return num_neighbors;
}

void get_neighbor_ports(int pid, int num_procs, int neighbor_ports[]) {
    if (pid == 0) {
        neighbor_ports[0] = 20000 + pid+1;
        return;
    }
    if (pid == num_procs-1) {
        neighbor_ports[0] = 20000 + pid-1;
        return;
    }
    else {
        neighbor_ports[0] = 20000 + pid-1;
        neighbor_ports[1] = 20000 + pid+1;
        return;
    }
}

/*
 * Process a command.
 * Based on command type, do the thing.
 */
void process_cmd(client_command *client_cmd, char **msg_log, uint16_t *vector_clock){
}

/*
 * Crash the server.
 * Run a local kill process command.
 */
int crash(){
    return (0);
}

/*
 * Exit. We may not need this here.
 */
void exit(){}

/*
 * Put message_t in message_t log.
 * Iterate the damn clock.
 * Send messages to neighbors.
 */
void send_msg(){
}

/*
 * may be able to handle this logic in process as it has the buffer.
 */
void send_log(char **msg_log, size_t num_msg, char *chat_log){
    /*
     * Zero out old chatlog
     */
    char *str[MAX_MSG_LEN];
    printf("parsing log to send\n");
    memset(chat_log, 0, sizeof(*chat_log));
    strcat(chat_log, "chatLog ");
    int i,j, len;

    for(i =0; i<num_msg; i++){
        char *curr_msg = msg_log[i];
        curr_msg[strlen(curr_msg)-1] = 0;

        if (i>0){
            strcat(chat_log, ",");
        }
        strcat(chat_log, curr_msg);
    }
    strcat(chat_log, "\n");
}

/**
 * Process an incoming message_t from a peer.
 * Add the message_t contents into the msg log.
 * Update vector clock
 *
 */
size_t update_log(message_t *msg, char **msg_log, size_t num_msg, uint16_t **msg_ids,
        uint16_t *vector_clock, int num_procs){

    /* Check Vector clock to see if the message_t is present */
    int expected_seq_num = vector_clock[msg->origin];

    printf("New message_t from server: %d. Expecting seqnum: %d, received seq_num: %d\n",
           msg->origin, expected_seq_num, msg->seqnum);

    if (msg->seqnum!=expected_seq_num){
        /* Seqnum is not expected sequence number*/
        if (msg->seqnum<expected_seq_num){
            /* Less than expected */
            /* Already have this message_t. Return*/
            printf("Low seq num, returning");
            return 0;
        }
        else{
            /* message_t is greater. */
            if(search_for_message(msg_ids, num_msg, msg->origin, msg->seqnum) >= 0){
                /* Already have this message_t. Return*/
                printf(" Duplicate high seq num, returning");
                return 0;
            }
        }
    }

    /*allocated pointers to store message_t and msg_id*/
    //printf("Allocating memory for new storage\n");

    char *msg_text = malloc(MAX_MSG_LEN* sizeof(char));
    uint16_t *msg_id = malloc(2*sizeof(uint16_t));

    /* assign values to the pointers. */

    strncpy(msg_text,msg->msg, msg->message_len);

    msg_id[0] = msg->origin;
    msg_id[1] = msg->seqnum;


    //printf("Copied message_t: %s. From: %d. seqnum: %d\n",msg_text,msg_id[0], msg_id[1]);


    /* add the pointers to the cache. */
    msg_log[num_msg] = msg_text;
    msg_ids[num_msg] = msg_id;

    /* update vector clock */
    update_vector_clock(vector_clock, msg_ids, num_msg+1, msg_id[0], num_procs);

    return 1;
}

size_t add_new_message(char *msg, uint16_t pid, uint16_t seqnum, char **msg_log,
        size_t num_msg, uint16_t **msg_ids, uint16_t *vector_clock, int num_procs){

    char *msg_text = malloc(MAX_MSG_LEN* sizeof(char));
    uint16_t *msg_id = malloc(2*sizeof(uint16_t));

    /* assign values to the pointers. */
    strncpy(msg_text, msg, strlen(msg));

    msg_id[0] = pid;
    msg_id[1] = seqnum;

    /*
    printf("Copied message_t: %s. From: %d. seqnum: %d\n",msg_text,msg_id[0], msg_id[1]);
    */

    /* add the pointers to the cache. */
    msg_log[num_msg] = msg_text;
    msg_ids[num_msg] = msg_id;

    /* update vector clock */
    update_vector_clock(vector_clock, msg_ids, num_msg+1, msg_id[0], num_procs);

    return 1;
}



/* Returns index of specified message_t, or -1 if not in msg log*/
int search_for_message(uint16_t **msg_ids, size_t num_msg, uint16_t tar_server, uint16_t tar_seqnum){
    int i;
    printf("searching\n");
    for (i = 0; i<num_msg; i++){
        printf("Message from server: %d, seq: %d\n", msg_ids[i][0], msg_ids[i][1]);
        if(msg_ids[i][0] == tar_server && msg_ids[i][1] == tar_seqnum){
            printf("message found\n");
            return i;
        }
    }
    printf("message not found\n");
    return -1;
}

/** receives a status message.
 *  gets the vector clock.
 *  return: -1 if server needs to get messages, 0 if nothing to do, 1 if server needs to send message.
 * */
int read_status_message(int *next_msg, message_t *msg, uint16_t *vector_clock, int num_procs){
    printf("processing Status. Current Status:\n");
    print_vector_clock(vector_clock, num_procs);


    int need_msgs = FALSE;
    int j;

    uint16_t rcvd_status[num_procs];
    memcpy(rcvd_status, msg->vector_clock, sizeof(vector_clock[0])*num_procs);
    printf("received status\n");
    print_vector_clock(rcvd_status,num_procs );

    for(j = 0; j<num_procs; j++){
        /* Check each peer's seqnum to determine if the status sender needs messages*/
        if (vector_clock[j]  > rcvd_status[j]){
            /* If servere has a higher number, sender needs message.
             * Make msg to send first unread message and return immediately. */

            //memcpy(next_msg[0],j, sizeof(int));
            next_msg[0] = j;
            next_msg[1] = rcvd_status[j];
            return 1;
        }
        else if (vector_clock[j]  < rcvd_status[j]) {
            /* Sender has messages to propagate to server.*/
            need_msgs = TRUE;
        }
    }
    if (need_msgs == TRUE){
        return -1;
    }
    return 0;
}

/**
 * Update the vector clock after receiving an incoming message_t.
 * Checks to see if message_t fills in any gaps.
 * Create a temp array for the given server.
 * and fill with zeros or ones depending on if the message_t is there.
 *
 */
void update_vector_clock(uint16_t * vector_clock, uint16_t **msg_ids, size_t num_msg,
        uint16_t new_msg_server, int num_procs){

    printf("%d Total Messages. Updating vector clock:\n", num_msg);
    //print_vector_clock(vector_clock, num_procs);

    int *temp[MAX_MSGS+1];
    int count = 0;
    memset(temp, FALSE, sizeof(*temp));

    int i;
    for (i = 0; i<num_msg; i++) {
        /*
        printf("Message %d. From: %d. seqnum: %d\n", i, msg_ids[i][0], msg_ids[i][1]);
        */
        if (msg_ids[i][0] == new_msg_server) {
            /*
            printf("Found a message_t on server %d, seq_num %d\n",new_msg_server, msg_ids[i][1]);
            */
            temp[msg_ids[i][1]-1] = TRUE;
            count++;
        }
    }
    for (i = 0; i<count+1; i++){
        if (temp[i]==FALSE){
            /*printf("Found first zero at index: %d\n", i);*/
            break;
        }
    }

    vector_clock[new_msg_server] = i+1;

    //printf("New Vector Clock:\n");
    //print_vector_clock(vector_clock, num_procs);
    return;
}
/**
 * Fill the empty message_t (msg_buff) with the information provided by the parameters.
 * This will make a message_t ready to send.
 */
void fill_message(message_t *msg_buff, enum message_type type, uint16_t server_pid,uint16_t origin_pid,
                  uint16_t seqnum, uint16_t *vector_clock, char *msg, int num_procs){

    msg_buff->seqnum = seqnum;
    msg_buff->type = type;
    msg_buff->message_len = strlen(msg);
    msg_buff->origin = origin_pid;
    msg_buff->from = server_pid;
    memcpy(msg_buff->vector_clock, vector_clock, sizeof(vector_clock[0])*MAX_MSG_LEN);

    if (type == STATUS){
        memset(msg_buff->msg, 0, MAX_MSG_LEN);
    }
    else{
        strcpy(msg_buff->msg, msg);
    }

    return;
}

void print_vector_clock(uint16_t *vector_clock, int num_procs){
    int i;
    for (i = 0; i<num_procs; i++){
        printf("Server: %d. Next Seqnum: %d\n",i,vector_clock[i]);
    }
    printf("\n");
}

void print_message(message_t *msg, int num_procs){
    printf("Message Type: %d\n"
           "From Server: %d\n"
           "sequence Number:%d\n"
           "message_t contents: %s\n", msg->type, msg->origin, msg->seqnum, msg->msg);
    printf(" Status vector from message_t:\n");
    print_vector_clock(msg->vector_clock, num_procs);
    return;
}

ssize_t sendto_peer(int sockfd, const void *out_pkt, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){

    char *buffer = malloc(len);
    memcpy(buffer, out_pkt, len);

    int retval = sendto(sockfd, buffer, len, flags, to, tolen);
    free(buffer);
    return retval;
}