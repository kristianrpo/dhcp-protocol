#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>           // Proporciona funciones estándar de entrada y salida como printf() y scanf().
#include <stdlib.h>          // Ofrece funciones para la gestión de memoria dinámica (malloc, free) y control del programa (exit()).
#include <string.h>          // Permite manipular y operar sobre cadenas de caracteres y bloques de memoria (strlen, strcpy, memset, etc.).
#include <arpa/inet.h>       // Proporciona funciones para la manipulación de direcciones IP, como conversión de direcciones entre texto y formato binario (inet_ntoa, inet_addr).
#include <sys/socket.h>      // Define las funciones y estructuras necesarias para la creación y manipulación de sockets (socket, bind, connect, etc.).
#include <netinet/in.h>      // Proporciona las definiciones y estructuras necesarias para trabajar con direcciones de red en la familia de protocolos AF_INET (IPv4 e IPv6), como sockaddr_in y sockaddr_in6.
#include <net/if.h>          // Contiene definiciones relacionadas con la configuración de interfaces de red, como la estructura ifreq, utilizada con ioctl().
#include <unistd.h>          // Define funciones estándar del sistema UNIX como read(), write(), close() y otras relacionadas con el control de procesos y la gestión de archivos.
#include <sys/types.h>       // Define tipos de datos comunes utilizados en las llamadas al sistema, como pid_t, size_t, off_t, etc. Proporciona tipos más portables y generalizados para diferentes plataformas.

#include "../error/error.h"
#include "../structs/structs.h"

// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *relay_addr, socklen_t relay_len);

// Función para recibir un mensaje desde el socket por parte del cliente.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *element_addr, socklen_t *element_len);

// Función para enviar un mensaje a través del socket.
int send_message(int fd, const char *buffer, int message_len, struct sockaddr_in *actor_addr, socklen_t actor_len, int is_client);

#endif
