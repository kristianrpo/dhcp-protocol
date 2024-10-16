#include "dhcp.h"

// Función para obtener el tipo de mensaje DHCP
int get_dhcp_message_type(struct dhcp_message *msg) {
    // Definimos la variable de control para recorrer el campo de options.
    int i = 0;

    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 53, que es la que contiene el mensaje para la acción a realizar.
    while (i < 312) {
        if (msg->options[i] == 53) { 
            return msg->options[i + 2];  
        }
        i += msg->options[i + 1] + 2;
    }
    return -1;  
}

// Función para configurar el mensaje DHCP
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, 
                            uint32_t xid, uint16_t flags, uint8_t *chaddr) {
    // Inicializamos el mensaje DHCP con ceros.
    memset(msg, 0, sizeof(struct dhcp_message));

    // Configuramos los campos principales del mensaje DHCP.
    msg->op = op;          // Tipo de operación (1 para solicitud, 2 para respuesta).
    msg->htype = htype;    // Tipo de hardware (1 para Ethernet).
    msg->hlen = hlen;      // Longitud de la dirección MAC (6 bytes para Ethernet).
    msg->xid = xid;        // ID de transacción.
    msg->flags = flags;    // Bandera de broadcast.

    // Copiamos la secuencia de bytes de la dirección MAC del cliente en el mensaje DHCP.
    memcpy(msg->chaddr, chaddr, 6);  // Solo los primeros 6 bytes de la MAC se copian.
}

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, 
                      uint8_t option_value) {
    // Configuramos el tipo de mensaje en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = option_type;

    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = option_length; 

    // Especificamos el valor de la opción.
    options[*index + 2] = option_value;

    // Incrementamos el índice para la siguiente opción (se avanzan 3 posiciones).
    *index += 3; 
}

// Función para configurar la dirección IP solicitada en las opciones del mensaje DHCP
void set_requested_ip(uint8_t *options, int *index, uint32_t requested_ip) {
    // Configuramos la opción de la IP solicitada en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = 50;  // Opción 50 es la IP solicitada.
    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = 4;  // Longitud de la dirección IP.
    // Copiamos la dirección IP solicitada en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &requested_ip, 4);  // Copiamos 4 bytes de la IP solicitada.
    // Incrementamos el índice para la siguiente opción (se avanzan 6 posiciones).
    *index += 6; 
}

// Función para configurar la dirección IP del servidor en las opciones del mensaje DHCP
void set_server_identifier(uint8_t *options, int *index, uint32_t server_identifier) {
    // Configuramos la opción de la dirección IP del servidor en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = 54;  // Opción 54 es la dirección IP del servidor.
    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = 4;  // Longitud de la dirección IP.
    // Copiamos la dirección IP del servidor en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &server_identifier, 4);  // Copiamos 4 bytes de la dirección IP del servidor.
    // Incrementamos el índice para la siguiente opción (se avanzan 6 posiciones).
    *index += 6; 
}