#ifndef DHCP_H
#define DHCP_H

#include <string.h>         // Permite manipular y operar sobre cadenas de caracteres y bloques de memoria (strlen, strcpy, memset, etc.).
#include <stdint.h>         // Proporciona definiciones de tipos de datos enteros con tamaños específicos (int8_t, uint16_t, int32_t, etc.), garantizando el uso de enteros de un tamaño fijo, independientemente de la arquitectura del sistema.
#include <pthread.h>        // Para manejar hilos (threads).
#include <stdio.h>          // Para funciones estándar de entrada/salida como printf, scanf.
#include <stdlib.h>         // Para funciones estándar como malloc, free, exit, atoi.
#include <unistd.h>         // Para funciones POSIX como sleep().
#include <termios.h>        // Para manipular los terminales (control de modos del terminal).
#include <fcntl.h>          // Para manipular descriptores de archivos (control de archivos).
#include <arpa/inet.h>      // Para funciones de conversión de direcciones de red como inet_ntoa.

#include "../structs/structs.h"
#include "../utils/utils.h"
#include "../include/shared_resources.h"

// Función para obtener el tipo de mensaje DHCP.
int get_dhcp_message_type(struct dhcp_message *msg);

// Función para configurar el mensaje DHCP.
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, 
                            uint32_t xid, uint16_t flags, uint8_t *chaddr);

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP.
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, 
                      uint8_t option_value);

// Función para configurar la dirección IP solicitada en las opciones del mensaje DHCP.
void set_requested_ip(uint8_t *options, int *index, uint32_t requested_ip);

// Función para configurar el identificador del servidor en las opciones del mensaje DHCP.
void set_server_identifier(uint8_t *options, int *index, uint32_t server_identifier);

// Función para extraer el tiempo de lease del mensaje DHCP.
uint32_t get_lease_time(struct dhcp_message *msg);

// Función para renovar el lease de la IP actual.
void renew_lease(const char *interface, struct dhcp_message *ack_msg, int send_socket, int recv_socket, struct sockaddr_in *relay_addr);

// Función que ejecuta el hilo que se encarga de renovar el lease de la IP.
void *lease_renewal(void *arg);

void send_dhcp_release(int send_socket, uint32_t assigned_ip, uint32_t server_ip, uint32_t transaction_id, uint8_t *chaddr, struct sockaddr_in *relay_addr, socklen_t relay_len);


#endif