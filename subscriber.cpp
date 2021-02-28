#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include "helpers.h"
#include "Subscriber_message.h"
#include "Tcp_message.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		usage(argv[0]);
	}

	int socket_tcp, socket_udp, n, ret;
	char buffer[BUFLEN];
	char backup_buffer[BACKUP_BUFLEN];
	uint16_t backup_buffer_occupied = 0;

	fd_set read_fds;
	fd_set tmp_fds;
	FD_ZERO(&tmp_fds);
	FD_ZERO(&read_fds);
	int fdmax;

	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socket_tcp < 0, "Socket tcp failed");

	FD_SET(socket_tcp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = socket_tcp;

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "ip_server error");

	ret = connect(socket_tcp, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect failed");
	char id_client[MAX_ID_LEN + sizeof(uint16_t)];
	memcpy(id_client, argv[1], strlen(argv[1]) + 1);
	send_message_via_tcp(socket_tcp, id_client, MAX_ID_LEN, true);

	while (1) {
  		tmp_fds = read_fds;
  		select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

  		// Reading from stdin.
  		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}
			Subscriber_message message_to_server;
			bool msg_computed = false;
			if (strncmp(buffer, "subscribe ", 10) == 0) {
				msg_computed = true;

				message_to_server.subscribe = true;
				
				// Finds topic's length.
				int topic_len = 0;
				int i = SUBSCRIBE_LETTERS;
				while (buffer[i] != ' ') {
					topic_len++;
					i++;
				}

				// Sets the SF field.
				if (buffer[SUBSCRIBE_LETTERS + topic_len + 1] == '0') {
					message_to_server.SF = false;
				} else if (buffer[SUBSCRIBE_LETTERS + topic_len + 1] == '1'){
					message_to_server.SF = true;
				} else {
					msg_computed = false;
				}
				if (msg_computed == true) {
					// Puts the topic in the message.
					topic_len = std::min(TOPIC_MAX_LEN, topic_len);
					memcpy(message_to_server.topic, buffer + SUBSCRIBE_LETTERS,
						topic_len);
					message_to_server.topic_len = topic_len;
				}
			} else if (strncmp(buffer, "unsubscribe", 11) == 0) {
				message_to_server.subscribe = false;

				// Finds topic's length.
				int topic_len = 0;
				int i = UNSUBSCRIBE_LETTERS;
				while (buffer[i] != '\n') {
					topic_len++;
					i++;
				}

				// Puts the topic in the message.
				topic_len = std::min(TOPIC_MAX_LEN, topic_len);
				memcpy(message_to_server.topic, buffer + UNSUBSCRIBE_LETTERS,
					topic_len);
				message_to_server.topic_len = topic_len;

				msg_computed = true;
			}
			if (msg_computed == true ){
				send_message_via_tcp(socket_tcp, (char*)&message_to_server,
					sizeof(message_to_server), false);
				if (message_to_server.subscribe == true) {
					printf("subscribed ");
				} else {
					printf("unsubscribed ");
				}
				for (int i = 0; i < message_to_server.topic_len; ++i) {
					printf("%c", message_to_server.topic[i]);
				}
				printf("\n");
			}
  		} else {
  			// Gets a message from the server.
	  		memset(buffer, 0, BUFLEN);
	  		receive_message_via_tcp(socket_tcp, buffer, BUFLEN,
	  			backup_buffer, backup_buffer_occupied);
	  		if (strncmp(buffer + sizeof(uint16_t), "ID already in use.", ID_USED_LEN) == 0 ||
	  			strncmp(buffer + sizeof(uint16_t), "SERVER CLOSED", SERVER_CLOSED_LEN) == 0) {
	  			printf("%s\n", buffer + sizeof(uint16_t));
				break;
			} else {
				// Subscription notification
				Tcp_message msg;
				memcpy(&msg, buffer, sizeof(msg));
				int w = 0;
				do {
					if (w == 1) {
						// Searches for other messages in the buffer backup.
						if (backup_buffer_occupied > 0) {
							uint16_t bytes_needed;
							memcpy(&bytes_needed, backup_buffer, sizeof(uint16_t));
							if (backup_buffer_occupied >= bytes_needed) {
								memcpy(&msg, backup_buffer, bytes_needed);
								backup_buffer_occupied -= bytes_needed;
								memcpy(backup_buffer, backup_buffer + bytes_needed,
									backup_buffer_occupied);
								memset(backup_buffer + backup_buffer_occupied, 0,
									bytes_needed);
							} else {
								break;
							}
						} else {
							break;
						}
					}
					struct in_addr in;
					in.s_addr = msg.cli_ip;
					char *addr = inet_ntoa(in);

					std::cout << addr << ":" << ntohs(msg.cli_port)<< " - " <<
					convertTopicToString(msg.topic) << " - ";
					char data_type = msg.data_type;
					if (data_type == DATA_TYPE_INT) {

						std::cout << "INT - ";
						uint32_t number = msg.number32;
						char sign = msg.sign;
						if (sign == 1) {
							std::cout << "-";
						}
						std::cout << number << "\n";

					} else if (data_type == DATA_TYPE_REAL) {

						std::cout << "SHORT_REAL - ";
						uint16_t number = msg.number16;
						if (number / LAST_TWO_DIGITS > 0) {
							// Integer part is not zero.

							uint16_t decimal_part = number % LAST_TWO_DIGITS;
							uint16_t integer_part = (number - decimal_part) / LAST_TWO_DIGITS;
							std::cout << integer_part;
							if (decimal_part != 0) {
								uint16_t first_digit = decimal_part / LAST_DIGIT;
								uint16_t second_digit = decimal_part % LAST_DIGIT;
								std::cout << "," << first_digit << second_digit;
							}
							std::cout << "\n";
						} else if (number > 0) {
							// Integer part is zero.

							std::cout << "0,";
							uint16_t first_digit = number / LAST_DIGIT;
							uint16_t second_digit = number % LAST_DIGIT;
							std::cout << first_digit << second_digit << "\n";
						} else {
							std::cout << "0\n";
						}

					} else if (data_type == DATA_TYPE_FLOAT) {

						std::cout << "FLOAT - ";
						uint32_t number = msg.number32;
						char sign = msg.sign;
						char negative_power = msg.negative_power;

						if (sign == 1) {
							std::cout << "-";
						}

						char number_of_digits = 0;
						uint32_t copy = number;
						while (copy > 0) {
							copy /= LAST_DIGIT;
							number_of_digits++;
						}
						if (negative_power > 0) {
							if (number_of_digits > negative_power) {
								// Integer part is not zero.
								uint32_t modulo = 1;
								for (int i = 0; i < negative_power; ++i) {
									modulo *= LAST_DIGIT;
								}
								uint32_t decimal_part = number % modulo;
								uint32_t integer_part = number / modulo;
								std::cout << integer_part << "," << decimal_part <<"\n";
							} else {
								// Integer part is zero.
								std::cout << "0,";
								for (int i = negative_power; i > number_of_digits; --i) {
									std::cout << "0";
								}
								std::cout << number << "\n";
							}
						} else {
							// Negative power is zero.
							std::cout << number << "\n";
						}

					} else if (data_type == DATA_TYPE_STRING) {
						std::cout << "STRING - ";
						memset(buffer, 0, BUFLEN);
		  				receive_message_via_tcp(socket_tcp, buffer,
		  					msg.content_len, backup_buffer, backup_buffer_occupied);
		  				for (int i = 0; i < msg.content_len; ++i) {
		  					printf("%c", buffer[i + sizeof(uint16_t)]);
		  				}
		  				printf("\n");
					}
					w = 1;
				} while (true);
			}
  		}
	}

	close(socket_tcp);

	return 0;
}
