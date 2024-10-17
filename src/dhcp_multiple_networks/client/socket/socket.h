#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>           // Proporciona funciones estándar de entrada y salida como printf() y scanf().
#include <stdlib.h>          // Ofrece funciones para la gestión de memoria dinámica (malloc, free) y control del programa (exit()).
#include <string.h>          // Permite manipular y operar sobre cadenas de caracteres y bloques de memoria (strlen, strcpy, memset, etc.).
#include <arpa/inet.h>       // Proporciona funciones para la manipulación de direcciones IP, como conversión de direcciones entre texto y formato binario (inet_ntoa, inet_addr).
#include <sys/socket.h>      // Define las funciones y estructuras necesarias para la creación y manipulación de sockets (socket, bind, connect, etc.).
#include <netinet/ip.h>      // Proporciona la estructura del encabezado IP y sus campos, útil para manipular paquetes IP a bajo nivel (struct iphdr).
#include <netinet/udp.h>     // Define la estructura del encabezado UDP para la creación y manipulación de paquetes del protocolo UDP (struct udphdr).
#include <net/if.h>          // Contiene definiciones relacionadas con la configuración de interfaces de red, como la estructura ifreq, utilizada con ioctl().
#include <linux/if_ether.h>  // Define estructuras y constantes relacionadas con el protocolo Ethernet. Contiene la definición de la cabecera Ethernet (struct ethhdr), así como otros tipos y tamaños estándar usados en redes Ethernet.
#include <linux/if_packet.h> // Proporciona definiciones para el manejo de sockets de bajo nivel en el modo "packet socket", permitiendo el acceso directo a las tramas a nivel de enlace de datos (capa 2) en interfaces de red.
#include <unistd.h>          // Define funciones estándar del sistema UNIX como read(), write(), close() y otras relacionadas con el control de procesos y la gestión de archivos.
#include <net/ethernet.h>    // Define estructuras para la manipulación de tramas Ethernet a nivel de enlace de datos, como la estructura ether_header para acceder a la cabecera Ethernet.

#include "../error/error.h"
#include "../structs/structs.h"

// Función para inicializar el socket DGRAM.
int initialize_DGRAM_socket(struct sockaddr_in *client_addr, socklen_t client_len);

// Función para inicializar el socket RAW.
int initialize_RAW_socket(struct sockaddr_in *client_addr, socklen_t client_len);

// Función para recibir un mensaje del socket.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *relay_addr, socklen_t *relay_len);

// Función para enviar un mensaje a través del socket.
int send_message(int fd, struct dhcp_message *msg, struct sockaddr_in *relay_addr, socklen_t relay_len);
#endif
