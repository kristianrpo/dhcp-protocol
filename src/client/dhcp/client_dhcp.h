#include <string.h> 
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include "../structs/client_structs.h"

// Función para obtener el tipo de mensaje DHCP
int get_dhcp_message_type(struct dhcp_message *msg);

// Función para configurar el mensaje DHCP
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, 
                            uint32_t xid, uint16_t flags, uint8_t *chaddr);

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, 
                      uint8_t option_value);
