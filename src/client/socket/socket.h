#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/udp.h> 
#include <netinet/ip.h>  
#include <linux/if_ether.h>
#include <linux/if_packet.h>


#include "../error/error.h"
#include "../structs/structs.h"

// Función para inicializar el socket DGRAM.
int initialize_DGRAM_socket(struct sockaddr_in *client_addr, socklen_t client_len);

// Función para inicializar el socket RAW.
int initialize_RAW_socket(struct sockaddr_in *client_addr, socklen_t client_len);

// Función para recibir un mensaje del socket.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *relay_addr, socklen_t *relay_len);

#endif
