//
// Created by Will Krasnoff on 3/10/20.
//

#ifndef _p2pApp_h
#define _p2pApp_h

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros


/*--- Parameters ---*/
#define NUM_SERVERS 4
#define MAX_MSG_LEN 200
#define MAX_MSGS 1000
#define TRUE 	 1
#define FALSE 	 0
#define ROOT_ID 20000

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
    uint16_t from; //sender of the message, by server_id
    uint16_t seqnum;
    uint16_t vector_clock[NUM_SERVERS]; //status vector
    char msg[MAX_MSG_LEN]; //MAX_MSG_LEN is defined to be 200
} message;

typedef struct client_command{
    enum command_type cmd_type;
    char msg[MAX_MSG_LEN];
};

/*--- Functions ---*/
void parse_input(char *cmd_string, client_command *client_cmd)
void


#endif //HOMEWORK_2_P2PAPP_H
