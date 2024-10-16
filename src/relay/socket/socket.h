#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include "../error/error.h"
#include "../structs/structs.h"

// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *relay_addr, socklen_t relay_len);

// Función para recibir un mensaje desde el socket por parte del cliente.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *element_addr, socklen_t *element_len);

// Función para enviar un mensaje a través del socket.
int send_message(int fd, const char *buffer, int message_len, struct sockaddr_in *actor_addr, socklen_t actor_len, int is_client);

#endif
