#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "../socket/socket.h"  
#include "../constants/constants.h"
#include "../dhcp/dhcp.h"

// Funciones para verificación y obtención de cabeceras y datos
int is_ip_packet(struct ethhdr *eth);
int is_udp_packet(struct iphdr *ip_header);
struct ethhdr *get_eth_header(char *buffer);
struct iphdr *get_ip_header(char *buffer);
struct udphdr *get_udp_header(char *buffer, struct iphdr *ip_header);
char *get_udp_payload(char *buffer, struct iphdr *ip_header, struct udphdr *udp_header);

// Función para procesar el mensaje
struct dhcp_message* process_msg(char *buffer);

// Función para imprimir la configuración de red
void print_network_config(struct dhcp_message *msg);

// Funciones relacionadas con la MAC y la asignación de IP
uint8_t *get_mac_address(const char *interface);

// Funciones para obtener el identificador del servidor.
uint32_t get_server_identifier(struct dhcp_message *msg);

// Función para asignar una IP a una interfaz
void assign_ip_to_interface(const char *interface, struct dhcp_message *msg);

#endif
