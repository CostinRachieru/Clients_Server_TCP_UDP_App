#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <limits.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include "helpers.h"
#include "Subscriber_message.h"
#include "Tcp_message.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

std::pair<std::string, bool>* find_client(std::vector<std::pair<std::string,
	bool>> &arr, std::string id) {
	for (int i = 0; i < arr.size(); ++i) {
		if (arr[i].first.compare(id) == 0) {
			return &arr[i];
		}
	}
	return NULL;
}

void remove_client_from_topic(std::vector<std::pair<std::string, bool>> &arr,
	std::string id) {
	for (int i = 0; i < arr.size(); ++i) {
		if (arr[i].first.compare(id) == 0) {
			arr.erase(arr.begin() + i);
		}
	}
}

void remove_client_socket(std::map<std::string, int> &clients_map,
	std::string id) {
	clients_map[id] = - 1;
}

void close_clients_sockets(std::map<std::string, int> &clients_map) {
	for (auto it = clients_map.begin(); it != clients_map.end(); ++it) {
		int client_socket = it -> second;
		if (client_socket != -1) {
			char msg[SERVER_CLOSED_LEN + sizeof(uint16_t)] = "SERVER CLOSED";
			send_message_via_tcp(client_socket, (char*)&msg, SERVER_CLOSED_LEN, true);
		    close(client_socket);
		}
	}
}

// Searches for the id of a client after it's socket.
// Returns an empty string if there is no entry for it.
std::string search_id_client(std::map<std::string, int> &clients_map, int socket) {
	for (auto it = clients_map.begin(); it != clients_map.end(); ++it) {
	    if (it->second == socket) {
	        return it->first;
	    }
	}
	return "";
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
	}

	char buffer[BUFLEN];
	char backup_buffer[BACKUP_BUFLEN_SERVER];
	uint16_t backup_buffer_occupied = 0;

	struct sockaddr_in serv_addr, cli_addr;
	int n, r, i, ret;
	socklen_t clilen;


	std::map<std::string, int> clients_map;
	std::map<std::string, std::vector<std::pair<std::string, bool>>> topics;
	std::map<std::string, std::vector<Udp_message*>> sf_map;


	int socket_tcp, socket_udp, port_number;

	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socket_tcp < 0, "socket_tcp failed");
	int flag = 1;
	ret = setsockopt(socket_tcp, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
		sizeof(int));
	DIE(ret < 0, "TCP_NODELAY failed");

	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(socket_udp < 0, "Socket udp failed");

	port_number = atoi(argv[1]);
	DIE(port_number == 0, "port number failed");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_number);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(socket_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp failed");

	ret = bind(socket_udp, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "bind udp failed");

	ret = listen(socket_tcp, INT_MAX - 1);
	DIE(ret < 0, "listen failed");

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_SET(socket_tcp, &read_fds);
	FD_SET(socket_udp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = std::max(socket_tcp, socket_udp);

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select failed");
		
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == socket_tcp) {
					// New conection found.
					int new_socket_tcp;
					clilen = sizeof(cli_addr);
					new_socket_tcp = accept(socket_tcp, (struct sockaddr *)
						&cli_addr, &clilen);
					DIE(new_socket_tcp < 0, "accept failed");

					FD_SET(new_socket_tcp, &read_fds);
					fdmax = std::max(new_socket_tcp, fdmax);

					char id_client[MAX_ID_LEN + sizeof(uint16_t)];
					memset(id_client, 0, MAX_ID_LEN + sizeof(uint16_t));
					receive_message_via_tcp(new_socket_tcp, id_client,
						MAX_ID_LEN + sizeof(uint16_t), backup_buffer,
						backup_buffer_occupied);

					std::string id = convertToString(id_client + sizeof(uint16_t));
					if (clients_map.find(id) == clients_map.end()) {
						// ID is valid. First time here.
						clients_map.insert(std::pair<std::string, int>(id,
							new_socket_tcp));

						std::cout << "New client " << id << " connected from " <<
							inet_ntoa(cli_addr.sin_addr) << ":" << 
							ntohs(cli_addr.sin_port) << ".\n" << std::flush;

						ret = setsockopt(new_socket_tcp, IPPROTO_TCP,
							TCP_NODELAY, (char*) &flag, sizeof(int));
						DIE(ret < 0, "TCP_NODELAY failed");
					} else {
						// It is not the first time.
						auto entry = clients_map.find(id);
						if (entry -> second == -1) {
							entry -> second = new_socket_tcp;
							auto pending = sf_map.find(id);
							if (pending != sf_map.end()) {
								std::vector<Udp_message*> pending_messages =
									pending -> second;
								for (int k = 0; k < pending_messages.size(); ++k) {
									Udp_message* pending_msg = pending_messages[k];
									uint32_t cli_ip = pending_msg -> cli_ip;
									uint16_t cli_port = pending_msg -> cli_port;

									Tcp_message tcp_msg(cli_ip, cli_port, pending_msg);
									memcpy(buffer, &tcp_msg, sizeof(tcp_msg));
									send_message_via_tcp(new_socket_tcp, buffer, 
										sizeof(tcp_msg), false);

									if (pending_msg -> data_type == 3) {
										memcpy(buffer + sizeof(uint16_t),
											pending_msg -> content, 
											pending_msg -> content_len);
										// Puts len at the beginning of the message.
										uint16_t len = pending_msg -> content_len +
											sizeof(uint16_t);
										memcpy(buffer, &len, sizeof(uint16_t));
										send_message_via_tcp(new_socket_tcp, buffer,
											len, false);
									} 
									delete(pending_msg);
								}
								sf_map.erase(pending);
							}
						} else {
							// Client's ID already in use.
							char error[ID_USED_LEN + sizeof(uint16_t)] =
								"ID already in use.";

							send_message_via_tcp(new_socket_tcp, error,
								ID_USED_LEN, true);
							FD_CLR(new_socket_tcp, &read_fds);
							close(new_socket_tcp);
						}
					}
				} else if (i == socket_udp) {
					// Gets messages from udp clients.
					memset(buffer, 0, BUFLEN);
					clilen = sizeof(cli_addr);
					r = recvfrom(socket_udp, buffer, BUFLEN, 0,
						(struct sockaddr*) &cli_addr, &clilen);

					uint32_t cli_ip = cli_addr.sin_addr.s_addr;
					uint16_t cli_port = cli_addr.sin_port;
					Udp_message *msg_from_udp = new Udp_message(buffer);
					msg_from_udp -> cli_ip = cli_ip;
					msg_from_udp -> cli_port = cli_port;
					std::string topic = convertTopicToString(msg_from_udp -> topic);

					auto entry = topics.find(topic);
					if (entry != topics.end()) {
						std::vector<std::pair<std::string, bool>> subscribers =
							entry -> second;

						Tcp_message tcp_msg(cli_ip, cli_port, msg_from_udp);
						for (int i = 0; i < subscribers.size(); ++i) {
							auto client = clients_map.find(subscribers[i].first);
							int socket = client -> second;
							if (socket != -1) {
								// Subscriber is connected.
								memcpy(buffer, &tcp_msg, sizeof(tcp_msg));
								send_message_via_tcp(socket, buffer, sizeof(tcp_msg),
									false);
								if (msg_from_udp -> data_type == 3) {
									memcpy(buffer + sizeof(uint16_t),
										msg_from_udp -> content, msg_from_udp -> content_len);
									// Puts len at the beginning of the message.
									uint16_t len = msg_from_udp -> content_len +
										sizeof(uint16_t);
									memcpy(buffer, &len, sizeof(uint16_t));
									send_message_via_tcp(socket, buffer, len, false);
								} 
							} else {
								if (subscribers[i].second == true) {
									// Creates a copy of the message. 
									// The other one will be deleted.
									Udp_message *copy = new Udp_message();
									msg_from_udp -> createCopy(copy);
									auto entry = sf_map.find(subscribers[i].first);
									if (entry != sf_map.end()) {
										// This subscribers already has pending messages.
										entry -> second.push_back(copy);
									} else {
										std::vector<Udp_message*> pending_messages;
										pending_messages.push_back(copy);
										sf_map.insert(std::pair<std::string,
											std::vector<Udp_message*>>(subscribers[i].first,
												pending_messages));
									}
								}
							}
						}
					}
					delete msg_from_udp;
				} else if (i == STDIN_FILENO) {
					// Receives exit command.
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);
					if (strncmp(buffer, "exit", 4) == 0) {
						close_clients_sockets(clients_map);
						close(socket_tcp);
						close(socket_udp);
						return 0;
					}
				} else {
					// Gets messages from clients.
					memset(buffer, 0, BUFLEN);
					n = receive_message_via_tcp(i, buffer, BUFLEN, backup_buffer,
						backup_buffer_occupied);
					DIE(n < 0, "recv");
					if (n == 0) {
						// conexiunea s-a inchis
						std::string client_id = search_id_client(clients_map, i);
						if (client_id.compare("") != 0) {
							std::cout << "Client " << client_id << " disconnected.\n";
							remove_client_socket(clients_map, client_id);
						}
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						Subscriber_message msg_from_client;
						memcpy(&msg_from_client, buffer, sizeof(Subscriber_message));
						if (msg_from_client.subscribe == true) {
							// Susbcribe message

							char *topic_char = msg_from_client.topic;
							std::string topic = convertTopicToString(msg_from_client.topic);
							std::string client_id = search_id_client(clients_map, i);
							bool is_SF = msg_from_client.SF;
							auto it = topics.find(topic);

							if (it == topics.end()) {
								// First appearance of this topic.
								std::vector<std::pair<std::string, bool>> aux;
								aux.push_back(std::pair<std::string, bool>(client_id, is_SF));
								topics.insert(std::pair<std::string,
									std::vector<std::pair<std::string, bool>>>(topic, aux));
							} else {
								// Topic already in data base.
								std::pair<std::string, bool> *entry = 
									find_client(it -> second, client_id);
								if (entry == NULL) {
									// New subscriber.
									it -> second.push_back(std::pair<std::string, bool>
										(client_id, is_SF));
								} else {
									// Already subscribed.
									entry -> second = is_SF;
									std::vector<std::pair<std::string, bool>> arr = it->second;
								}
							}
						} else {
							// Unsubscribe message
							char *topic_char = msg_from_client.topic;
							std::string topic = convertTopicToString(msg_from_client.topic);
							std::string client_id = search_id_client(clients_map, i);
							auto it = topics.find(topic);
							if (it != topics.end()) {
								remove_client_from_topic(it -> second, client_id);
							}
							if (it -> second.size() == 0) {
								topics.erase(it);
							}
						}
					}
				}
			}
		}
	}

	close(socket_tcp);
	close(socket_udp);
	close_clients_sockets(clients_map);
	return 0;
}
