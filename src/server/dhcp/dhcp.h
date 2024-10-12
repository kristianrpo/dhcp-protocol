#ifndef DHCP_H
#define DHCP_H

#include <string.h> 
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include "../structs/dhcp_structs.h"
#include "../error/error.h"

// Función para obtener el mensaje DHCP para conocer la opción a realizar.
int get_dhcp_message_type(struct dhcp_message *msg);

// Función para convertir una IP string (ej: "192.168.0.1") a formato entero binario
uint32_t ip_to_int(const char *ip);

// Función para convertir una IP en formato binario de vuelta a cadena
void int_to_ip(uint32_t ip, char *buffer);

// Función para inicializar los valores de memoria de los arrendamientos (en 0) con el fin de evitar y limpiar los espacios que se pretenden ocupar.
void initialize_leases(struct lease_entry leases[MAX_LEASES]);

// Función para asignar una ip al cliente.
uint32_t reserved_ip_to_client(struct lease_entry leases[MAX_LEASES]);

// Función para asignar una ip al cliente y actualizar la tabla de arrendamientos.
uint32_t assign_ip_to_client(struct lease_entry leases[MAX_LEASES], uint32_t requested_ip, uint8_t *mac_addr);

// Función para obtener la IP solicitada por el cliente en un DHCPREQUEST que fue la que se envió en el DHCPOFFER.
uint32_t get_requested_ip(struct dhcp_message *request_msg);

// Función para configurar los campos principales del mensaje DHCP.
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, uint32_t xid, uint32_t yiaddr, uint8_t *chaddr);

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, uint8_t option_value);

// Función para establecer la máscara de subred en el mensaje DHCP
void set_subnet_mask(uint8_t *options, int *index);

// Función para establecer el gateway en las opciones del mensaje DHCP
void set_gateway(uint8_t *options, int *index);

// Función para establecer el servidor DNS en las opciones del mensaje DHCP.
void set_dns_server(uint8_t *options, int *index);

// Función para establecer el identificador del servidor en las opciones del mensaje DHCP.
void set_server_identifier(uint8_t *options, int *index);

// Función para enviar un DHCPOFFER.
void send_dhcp_offer(int fd, struct sockaddr_in *relay_addr, socklen_t relay_len, struct dhcp_message *discover_msg, struct lease_entry leases[MAX_LEASES]);

// Función para enviar un DHCPNAK.
void send_dhcp_nak(int fd, struct sockaddr_in *relay_addr, socklen_t relay_len, struct dhcp_message *request_msg);

// Función para enviar un DHCPACK.
void send_dhcp_ack(int fd, struct sockaddr_in *relay_addr, socklen_t relay_len, struct dhcp_message *request_msg, struct lease_entry leases[MAX_LEASES]);

// Función para verificar si un arrendamiento ha expirado y liberar la IP.
void check_state_leases(struct lease_entry leases[MAX_LEASES]);

// Función para procesar los mensajes DHCP según el tipo.
void process_dhcp_message(int message_type, int fd, struct sockaddr_in *relay_addr, socklen_t relay_len, struct dhcp_message *msg, struct lease_entry leases[MAX_LEASES]);

// Función para manejar la lógica para procesar una solicitud DHCP de un cliente en un hilo separado.
// El parámetro args es un puntero a una estructura que contiene los argumentos necesarios para procesar la solicitud.
// Basicamente cuando se necesita otro hilo, ese otro hilo ejecuta esta funcion para procesar solicitudes DHCP.
void *handle_client(void *args);

#endif 
