#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>         // Proporciona funciones estándar de entrada y salida como printf() y scanf().
#include <stdlib.h>        // Ofrece funciones para la gestión de memoria dinámica (malloc, free) y control del programa (exit()).
#include <string.h>        // Permite manipular y operar sobre cadenas de caracteres y bloques de memoria (strlen, strcpy, memset, etc.).
#include <arpa/inet.h>     // Proporciona funciones para la manipulación de direcciones IP, como conversión de direcciones entre texto y formato binario (inet_ntoa, inet_addr).
#include <sys/socket.h>    // Define las funciones y estructuras necesarias para la creación y manipulación de sockets (socket, bind, connect, etc.).
#include <netinet/in.h>    // Contiene definiciones para trabajar con direcciones de red (IPv4 e IPv6) y estructuras como sockaddr_in.
#include <netinet/ip.h>    // Proporciona la estructura del encabezado IP y sus campos, útil para manipular paquetes IP a bajo nivel (struct iphdr).
#include <netinet/udp.h>   // Define la estructura del encabezado UDP para la creación y manipulación de paquetes del protocolo UDP (struct udphdr).
#include <net/if.h>        // Contiene definiciones relacionadas con la configuración de interfaces de red, como la estructura ifreq, utilizada con ioctl().
#include <sys/ioctl.h>     // Proporciona funciones para manipular parámetros de dispositivos de E/S, como configurar propiedades de interfaces de red (ioctl).
#include <unistd.h>        // Define funciones estándar del sistema UNIX como read(), write(), close() y otras relacionadas con el control de procesos y la gestión de archivos.
#include <net/ethernet.h>  // Define estructuras para la manipulación de tramas Ethernet a nivel de enlace de datos, como la estructura ether_header para acceder a la cabecera Ethernet.

#include "../socket/socket.h"  
#include "../constants/constants.h"
#include "../dhcp/dhcp.h"

// Funciones para verificación y obtención de cabeceras y datos.
int is_ip_packet(struct ethhdr *eth);
int is_udp_packet(struct iphdr *ip_header);
struct ethhdr *get_eth_header(char *buffer);
struct iphdr *get_ip_header(char *buffer);
struct udphdr *get_udp_header(char *buffer, struct iphdr *ip_header);
char *get_udp_payload(char *buffer, struct iphdr *ip_header, struct udphdr *udp_header);

// Función para procesar el mensaje.
struct dhcp_message* process_msg(char *buffer);

// Función para imprimir la configuración de red.
void print_network_config(struct dhcp_message *msg);

// Funciones relacionadas con la MAC y la asignación de IP.
uint8_t *get_mac_address(const char *interface);

// Funciones para obtener el identificador del servidor.
uint32_t get_server_identifier(struct dhcp_message *msg);

// Función para asignar una IP a una interfaz.
void assign_ip_to_interface(const char *interface, struct dhcp_message *msg);

#endif
