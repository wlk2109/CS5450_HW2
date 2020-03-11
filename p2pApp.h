//
// Created by Will Krasnoff on 3/10/20.
//

#ifndef HOMEWORK_2_P2PAPP_H
#define HOMEWORK_2_P2PAPP_H

#include <GBN/*>

#define NUM_SERVERS 4
#define MAX_MSG_LEN 200

/*----- Message Types -----*/
enum message_type {
    STATUS=0,
    RUMOR
} message_type;

/* Message Struct */
typedef struct message {
    enum message_type type;
    uint16_t message_len;
    uint16_t from; //sender of the message, by server_id
    uint16_t seqnum;
    uint16_t vector_clock[NUM_SERVERS]; //status vector
    char msg[MAX_MSG_LEN]; //MAX_MSG_LEN is defined to be 200
} message;


#endif //HOMEWORK_2_P2PAPP_H
