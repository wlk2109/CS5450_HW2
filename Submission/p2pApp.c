
#include "p2pApp.h"

/*
 * Get a message_t from the Client.
 * Parse the message_t command.
 *
 * Fill client_command
 *
 */
int parse_input(char *cmd_string, client_command *client_cmd){
    if (strcmp(cmd_string, "crash\n")==0){
        client_cmd->cmd_type=CRASH;
        return CRASH;
    }
    char* token = strtok(cmd_string, " ");

    if (strcmp(token, "msg")==0){
        client_cmd ->cmd_type = MSG;

        token = strtok(NULL, " ");
        long m_id= strtol(token, NULL, 10);
        client_cmd->msg_id = (uint16_t) m_id;

        token = strtok(NULL, "\0");
        if (strlen(token)==0){
            return -1;
        }
        strcpy(client_cmd->msg, token);

        return MSG;
    }
    else if (strcmp(token, "get")==0){
        client_cmd->cmd_type = GET;
        return GET;
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
    return rand()%2;
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

int get_neighbor_port_idx(uint16_t neighbor_pid, uint16_t server_pid, int num_neighbors){
    if (num_neighbors ==1){
        return 0;
    }else{return fmax(0, (int)neighbor_pid - server_pid);}
}

/*
 * may be able to handle this logic in process as it has the buffer.
 */
void send_log(char **msg_log, size_t num_msg, char *chat_log){
    /*
     * Zero out old chatlog
     */

    memset(chat_log, 0, sizeof(*chat_log));
    strcat(chat_log, "chatLog ");
    int i, len;

    for(i =0; i<num_msg; i++){
        /* Remove newline if message has it */
        char *curr_msg = msg_log[i];
        len = strlen(curr_msg);
        if (curr_msg[len-1] == '\n') {
            curr_msg[strlen(curr_msg)-1] = 0;
        }

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

    if (msg->seqnum!=expected_seq_num){
        /* Seqnum is not expected sequence number*/
        if (msg->seqnum<expected_seq_num){
            /* Less than expected */
            /* Already have this message_t. Return*/
            return 0;
        }
        else{
            /* message_t is greater. */
            if(search_for_message(msg_ids, num_msg, msg->origin, msg->seqnum) >= 0){
                /* Already have this message_t. Return*/
                return 0;
            }
        }
    }

    /*allocated pointers to store message_t and msg_id*/
    char *msg_text = malloc(MAX_MSG_LEN* sizeof(char));
    uint16_t *msg_id = malloc(2*sizeof(uint16_t));

    /* assign values to the pointers. */

    strncpy(msg_text,msg->msg, msg->message_len);

    msg_id[0] = msg->origin;
    msg_id[1] = msg->seqnum;

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
    for (i = 0; i<num_msg; i++){
        if(msg_ids[i][0] == tar_server && msg_ids[i][1] == tar_seqnum){
            return i;
        }
    }
    return -1;
}

/** receives a status message.
 *  gets the vector clock.
 *  return: -1 if server needs to get messages, 0 if nothing to do, 1 if server needs to send message.
 * */
int read_status_message(int *next_msg, message_t *msg, uint16_t *vector_clock, int num_procs){
    int need_msgs = FALSE;
    int j;

    uint16_t rcvd_status[num_procs];
    memcpy(rcvd_status, msg->vector_clock, sizeof(vector_clock[0])*num_procs);

    for(j = 0; j<num_procs; j++){
        /* Check each peer's seqnum to determine if the status sender needs messages*/
        if (vector_clock[j]  > rcvd_status[j]){
            /* If server has a higher number, sender needs message.
             * Make msg to send first unread message and return immediately. */
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
    uint16_t temp[MAX_MSGS+1];
    int count = 0;
    memset(temp, 0, sizeof(temp));

    int i;
    for (i = 0; i<num_msg; i++) {

        if (msg_ids[i][0] == new_msg_server) {
            temp[msg_ids[i][1]-1] = TRUE;
            count++;
        }
    }
    for (i = 0; i<count+1; i++){
        if (temp[i]==FALSE){
            break;
        }
    }
    vector_clock[new_msg_server] = i+1;

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
    msg_buff->origin = origin_pid;
    msg_buff->from = server_pid;
    memcpy(msg_buff->vector_clock, vector_clock, sizeof(vector_clock[0])*MAX_MSG_LEN);
    if (type == STATUS){
        msg_buff->message_len = 0;
        memset(msg_buff->msg, '\0', MAX_MSG_LEN);
    }
    else{
        msg_buff->message_len = strlen(msg);
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
           "Origin Server: %d\n"
           "From Server: %d\n"
           "sequence Number:%d\n"
           "message_t contents: %s\n", msg->type, msg->origin, msg->from,msg->seqnum, msg->msg);
    printf(" Status vector from message_t:\n");
    print_vector_clock(msg->vector_clock, num_procs);
    return;
}
