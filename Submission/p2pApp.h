
#ifndef _p2pApp_h
#define _p2pApp_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>


/*--- Parameters ---*/
#define NUM_SERVERS 4
#define MAX_MSG_LEN 200
#define MAX_MSGS 1000
#define TRUE 	 1
#define FALSE 	 0
#define ROOT_ID 20000
#define HEADS 1
#define TAILS 0
#define ANTI_ENT 10

/*----- Message Types -----*/
enum message_type {
    STATUS=0,
    RUMOR
} message_type;

enum command_type{
    MSG = 0,
    GET,
    CRASH,
    EXIT
} command_type;

/* Message Struct */
typedef struct message {
    enum message_type type;
    uint16_t message_len;
    uint16_t from;
    uint16_t origin; /*original sender of the message_t, by server_id*/
    uint16_t seqnum;
    uint16_t vector_clock[NUM_SERVERS]; /*status vector*/
    char msg[MAX_MSG_LEN]; /*MAX_MSG_LEN is defined to be 200*/
} message_t;

typedef struct client_command{
    enum command_type cmd_type;
    uint16_t msg_id;
    char msg[MAX_MSG_LEN];
} client_command;

/*--- Functions ---*/
int parse_input(char *cmd_string, struct client_command *client_cmd);
void process_cmd(struct client_command *client_cmd, char **msg_log, uint16_t *vector_clock);

void send_log(char **msg_log, size_t num_msg, char *chat_log);
size_t update_log(message_t *msg, char **msg_log, size_t num_msg, uint16_t **msg_ids,
                  uint16_t *vector_clock, int num_procs);
int search_for_message(uint16_t **msg_ids, size_t num_msg, uint16_t tar_server, uint16_t tar_seqnum);
void update_vector_clock(uint16_t * vector_clock, uint16_t **msg_ids, size_t num_msg,
                         uint16_t new_msg_server, int num_procs);
void print_vector_clock(uint16_t *vector_clock, int num_procs);
void print_message(message_t *msg, int num_procs);
void fill_message(message_t *msg_buff, enum message_type type, uint16_t server_pid, uint16_t origin_pid,
                  uint16_t seqnum, uint16_t *vector_clock, char *msg, int num_procs);
int init_neighbors(int pid, int num_procs, int *potential);
void get_neighbor_ports(int pid, int num_procs, int neighbor_ports[]);
int pick_neighbor(int num_neighbors);
size_t add_new_message(char *msg, uint16_t pid, uint16_t seqnum, char **msg_log,
                       size_t num_msg, uint16_t **msg_ids, uint16_t *vector_clock, int num_procs);
int read_status_message(int *next_msg, message_t *msg, uint16_t *vector_clock, int num_procs);
int get_neighbor_port_idx(uint16_t neighbor_pid, uint16_t server_pid, int num_neighbors);
void timeout_hdler(int signum);

#endif