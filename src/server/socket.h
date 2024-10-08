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
#include "error.h"
#include "dhcp_structs.h"

// Función para inicializar el socket.
int initialize_socket();

// Función para recibir un mensaje desde el socket.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *client_addr, socklen_t *client_len);

#endif
