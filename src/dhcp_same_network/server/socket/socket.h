#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>           // Proporciona funciones estándar de entrada y salida como printf() y scanf().
#include <stdlib.h>          // Ofrece funciones para la gestión de memoria dinámica (malloc, free) y control del programa (exit()).
#include <string.h>          // Permite manipular y operar sobre cadenas de caracteres y bloques de memoria (strlen, strcpy, memset, etc.).
#include <arpa/inet.h>       // Proporciona funciones para la manipulación de direcciones IP, como conversión de direcciones entre texto y formato binario (inet_ntoa, inet_addr).
#include <sys/socket.h>      // Define las funciones y estructuras necesarias para la creación y manipulación de sockets (socket, bind, connect, etc.).
#include <netinet/in.h>      // Proporciona definiciones y estructuras para trabajar con direcciones de red y la familia de protocolos AF_INET (IPv4) y AF_INET6 (IPv6). Incluye estructuras como sockaddr_in y funciones para la conversión de direcciones de red.
#include <sys/types.h>       // Define tipos de datos básicos usados en las llamadas al sistema, como pid_t, size_t y ssize_t. Estos tipos garantizan portabilidad entre diferentes plataformas.
#include <unistd.h>          // Define funciones estándar del sistema UNIX como read(), write(), close() y otras relacionadas con el control de procesos y la gestión de archivos.
#include <net/if.h>          // Contiene definiciones relacionadas con la configuración de interfaces de red, como la estructura ifreq, utilizada con ioctl().

#include "../error/error.h"
#include "../structs/structs.h"

// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *server_addr, socklen_t server_len);

// Función para recibir un mensaje desde el socket.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *client_addr, socklen_t *client_len);

// Función para enviar un mensaje a través del socket.
int send_message(int fd, struct dhcp_message *msg, struct sockaddr_in *client_addr, socklen_t client_len);

#endif
