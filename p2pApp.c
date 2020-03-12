//
// Created by Will Krasnoff on 3/10/20.
//


void start_peers(int num_peers) {

}

/*
 * Get a message from the Client.
 * Parse the message command.
 *
 * Fill client_command
 *
 */
void parse_input(char *cmd_string, client_command *client_cmd){

}

/*
 * Process a command.
 * Based on command type, do the thing.
 */
void process_cmd(client_command *client_cmd){

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
void exit(){

}

/*
 * Put message in message log.
 * Iterate the damn clock.
 * Send messages to neighbors.
 *
 */
void send_msg(){

}

/*
 * may be able to handle this logic in process as it has the buffer.
 */
char * send_log(char **msg_log){

}

/*
 * Process an incoming message.
 * Add the message contents into the msg log.
 *
 */
void update_log(message *msg, char **msg_log, int *vector_clock){

}




