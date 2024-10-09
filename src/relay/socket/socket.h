#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../error/error.h"
#include "../structs/relay_structs.h"

// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *relay_addr, socklen_t relay_len);

// Función para recibir un mensaje desde el socket por parte del cliente.
ssize_t receive_message_client(int fd, char *buffer, struct sockaddr_in *client_addr, socklen_t *client_len);

// Función para recibir un mensaje desde el socket por parte del servidor.
ssize_t receive_message_server(int fd, char *buffer, struct sockaddr_in *server_addr, socklen_t *server_len);

#endif
