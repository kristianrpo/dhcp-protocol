#ifndef DHCP_H
#define DHCP_H
#include <string.h> 
#include <stdint.h>
#include "../structs/structs.h"

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

#endif