
#include "p2pApp.h"

/*
 * Get a message_t from the Client.
 * Parse the message_t command.
 *
 * Fill client_command
 *
 */
int parse_input(char *cmd_string, client_command *client_cmd){

    printf("Incoming: %s\n", cmd_string);

    char* token = strtok(cmd_string, " ");

    /* msg <messagID> <message_t>
     * crash
     * get chatLog
     */


    if (strcmp(token, "msg")==0){
        printf("It's a message_t!\n");
        client_cmd ->cmd_type = MSG;

        token = strtok(NULL, " ");
        printf("new token: %s\n", token);

        long m_id= strtol(token, NULL, 10);
        printf("Message ID: %ld\n",m_id);
        client_cmd->msg_id = (uint16_t) m_id;

        token = strtok(NULL, "\0");
        printf("Message: %s\n", token);
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
void crash(){
}

/*
 * Exit. We may not need this here.
 */
void exit(){}

/*
 * Put message_t in message_t log.
 * Iterate the damn clock.
 * Send messages to neighbors.
 *
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
    printf("parsing log to send\n");
    memset(chat_log, 0, sizeof(*chat_log));
    strcat(chat_log, "chatLog ");
    int i;
    for(i =0; i<num_msg; i++){
        if (i>0){
            strcat(chat_log, ",");
        }
        strcat(chat_log, msg_log[i]);
    }
}

/**
 * Process an incoming message_t from a peer.
 * Add the message_t contents into the msg log.
 * Update vector clock
 *
 */
size_t update_log(message_t *msg, char **msg_log, size_t num_msg, uint16_t **msg_ids, uint16_t *vector_clock, int num_procs){
    /* Check Vector clock to see if the message_t is present */
    int expected_seq_num = vector_clock[msg->origin] + 1;

    printf("New message_t from server: %d. Expecting seqnum: %d, received seq_num: %d\n",
           msg->origin, expected_seq_num, msg->seqnum);

    if (msg->seqnum!=expected_seq_num){
        /* Seqnum is not expected sequence number*/
        if (msg->seqnum<expected_seq_num){
            /* Less than expected */
            /* Already have this message_t. Return*/
            printf("Low seq num, returning");
            return num_msg;
        }
        else{
            /* message_t is greater. */
            if(search_for_message(msg_ids, num_msg, msg->origin, msg->seqnum) >= 0){
                /* Already have this message_t. Return*/
                printf(" Duplicate high seq num, returning");
                return num_msg;
            }
        }
    }

    /*allocated pointers to store message_t and msg_id*/
    printf("Allocating memory for new storage\n");

    char *msg_text = malloc(MAX_MSG_LEN* sizeof(char));
    uint16_t *msg_id = malloc(2*sizeof(uint16_t));

    /* assign values to the pointers. */

    strncpy(msg_text,msg->msg, msg->message_len);

    msg_id[0] = msg->origin;
    msg_id[1] = msg->seqnum;

    printf("Copied message_t: %s. From: %d. seqnum: %d\n",msg_text,msg_id[0], msg_id[1]);

    /* add the pointers to the cache. */
    msg_log[num_msg] = msg_text;
    msg_ids[num_msg] = msg_id;

    printf("Copied message_t: %s. From: %d. seqnum: %d\n",msg_log[num_msg],msg_ids[num_msg][0], msg_ids[num_msg][1]);

    num_msg++;

    /* update vector clock */
    update_vector_clock(vector_clock, msg_ids, num_msg, msg_id[0], num_procs);

    return num_msg;
}

/* Returns index of specified message_t, or -1 if not in msg log*/
int search_for_message(int **msg_ids, size_t num_msg, uint16_t tar_server, uint16_t tar_seqnum){
    int i;
    for (i = 0; i<num_msg; i++){
        if(msg_ids[i][0] == tar_server && msg_ids[i][1] == tar_seqnum){
            return i;
        }
    }
    return -1;
}

/**
 * Update the vector clock after receiving an incoming message_t.
 * Checks to see if message_t fills in any gaps.
 * Create a temp array for the given server
 * and fill with zeros or ones depending on if the message_t is there.
 */
void update_vector_clock(uint16_t * vector_clock, uint16_t **msg_ids, size_t num_msg,
        uint16_t new_msg_server, int num_procs){
    printf("%d Total Messages. Updating vector clock:\n", num_msg);
    print_vector_clock(vector_clock, num_procs);
    int *temp[MAX_MSGS+1];
    int count = 0;
    memset(temp, FALSE, sizeof(*temp));


    int i;
    for (i = 0; i<num_msg; i++) {
        printf("Message %d. From: %d. seqnum: %d\n", i, msg_ids[i][0], msg_ids[i][1]);
        if (msg_ids[i][0] == new_msg_server) {
            printf("Found a message_t on server %d, seq_num %d\n",new_msg_server, msg_ids[i][1]);
            temp[msg_ids[i][1]] = TRUE;
            count++;
        }
    }
    for (i = 1; i<=count; i++){
        if (temp[i]==FALSE){
            printf("Found first zero at index: %d", i);
            break;
        }
    }

    vector_clock[new_msg_server] = i-1;

    printf("New Vector Clock:\n");
    print_vector_clock(vector_clock, num_procs);
    return;
}
/**
 * Fill the empty message_t (msg_buff) with the information provided by the parameters.
 * This will make a message_t ready to send.
 */
void fill_message(message_t *msg_buff, enum message_type type, uint16_t server_pid,
                  uint16_t seqnum, uint16_t *vector_clock, char *msg, int num_procs){
    msg_buff->seqnum = seqnum;
    msg_buff->origin = server_pid;
    msg_buff->message_len = strlen(msg);
    memcpy(msg_buff->vector_clock, vector_clock, sizeof(*vector_clock)*num_procs);
    strcpy(msg_buff->msg, msg);

    printf("Filled Message:\n");
    print_message(msg_buff, num_procs);

    return;
}

void print_vector_clock(uint16_t *vector_clock, int num_procs){
    int i;
    for (i = 0; i<num_procs; i++){
        printf("Server: %d. Last Seqnum: %d\n",i,vector_clock[i]);
    }
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