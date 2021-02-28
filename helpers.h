#ifndef _HELPERS_H
#define _HELPERS_H 1

 

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <iostream>

 

#define DIE(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, "(%s, %d): ",    \
                    __FILE__, __LINE__);    \
            perror(call_description);        \
            exit(EXIT_FAILURE);                \
        }                                    \
    } while(0)

 

#define BUFLEN        1578    // dimensiunea maxima a calupului de date
#define BACKUP_BUFLEN    1578
#define BACKUP_BUFLEN_SERVER    1000
#define MAX_ID_LEN     10
#define TOPIC_MAX_LEN     50
#define DATA_TYPE_LEN    1
#define SUBSCRIBE_LETTERS    10
#define UNSUBSCRIBE_LETTERS    12
#define ID_USED_LEN 18
#define CONTENT_MAX_LEN 1500
#define LAST_TWO_DIGITS    100
#define LAST_DIGIT     10
#define SERVER_CLOSED_LEN    14
#define POSITIVE_NUMBER    1
#define DATA_TYPE_STRING	3
#define DATA_TYPE_INT	0
#define DATA_TYPE_FLOAT	2
#define DATA_TYPE_REAL	1

 

std::string convertToString(char* id) {
    std::string s = "";
    int i = 0;
    while (id[i] != 0 && i < MAX_ID_LEN) {
        s += id[i++];
    }
    return s;
}

 

std::string convertTopicToString(char* topic) {
    std::string s = "";
    int i = 0;
    while (topic[i] != 0 && i < TOPIC_MAX_LEN) {
        s += topic[i++];
    }
    return s;
}

 

// Assumes there is enough memory in buffer.
void send_message_via_tcp(int socket, char *buffer, uint16_t len,
	bool add_header) {
    
    uint16_t bytes_remaining = len;
    uint16_t bytes_sent = 0;
    if (add_header) {
        bytes_remaining = len + sizeof(uint16_t);
        memcpy(buffer + sizeof(uint16_t), buffer, len);
        memcpy(buffer, &bytes_remaining, sizeof(uint16_t));
    }
    while (bytes_remaining > 0) {
        bytes_sent += send(socket, buffer + bytes_sent, bytes_remaining, 0);
        bytes_remaining -= bytes_sent;
    }
}

 

// n = recv(socket_tcp, buffer, BUFLEN, 0);
int receive_message_via_tcp(int socket, char *buffer, uint16_t buffer_size,
    char *backup_buffer, uint16_t &backup_buffer_occupied) {
    uint16_t bytes_remaining = 0;
    uint16_t bytes_received;
    if (backup_buffer_occupied == 0) {
        bytes_received = recv(socket, buffer, buffer_size, 0);
        if (bytes_received == 0) {
            return 0;
        }
        // Read the bytes that contain the length of the message.
        while (bytes_received < sizeof(uint16_t)) {
            bytes_received += recv(socket, (char*)buffer + bytes_received,
            	buffer_size - bytes_received, 0);
        }
    } else {
        uint16_t bytes_first_msg;
        memcpy(&bytes_first_msg, backup_buffer, sizeof(uint16_t));
        if (bytes_first_msg <= backup_buffer_occupied) {
	        memcpy(buffer, backup_buffer, bytes_first_msg);
	        memcpy(backup_buffer, backup_buffer + bytes_first_msg,
	        	backup_buffer_occupied - bytes_first_msg);
	        backup_buffer_occupied -= bytes_first_msg;
	        memset(backup_buffer + backup_buffer_occupied, 0, bytes_first_msg);
	        bytes_received = bytes_first_msg;
        } else {
        	memcpy(buffer, backup_buffer, backup_buffer_occupied);
        	memset(backup_buffer, 0, backup_buffer_occupied);
        	bytes_received = backup_buffer_occupied;
        	backup_buffer_occupied = 0;
        }
    }
    memcpy(&bytes_remaining, buffer, sizeof(uint16_t));
    if (bytes_remaining >= bytes_received) {
        // The messages are not concatenated.
        bytes_remaining -= bytes_received;
        while (bytes_remaining > 0) {
            bytes_received = recv(socket, buffer + bytes_received,
            	bytes_remaining, 0);
            bytes_remaining -= bytes_received;
            if (bytes_received == 0) {
                return 0;
            }
        }
    } else {
        memcpy(backup_buffer + backup_buffer_occupied,
            buffer + bytes_remaining, bytes_received - bytes_remaining);
        backup_buffer_occupied += bytes_received - bytes_remaining;
    }
    return POSITIVE_NUMBER;
}

 

#endif