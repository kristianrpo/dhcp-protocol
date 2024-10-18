#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>        // Proporciona definiciones de tipos de datos enteros con tamaños específicos (int8_t, uint16_t, int32_t, etc.), garantizando el uso de enteros de un tamaño fijo, independientemente de la arquitectura del sistema.
#include <netinet/in.h>    // Proporciona las definiciones de las constantes y estructuras necesarias para trabajar con direcciones de red, como sockaddr_in.
#include "../constants/constants.h" 

struct dhcp_message {
    uint8_t op;             // Define el tipo de operacion del paquete, =1 si es una solicitud proveniente de un cliente, =2 si es una respuesta proveniente del servidor.      
    uint8_t htype;          // Define el tipo de hardware que se está utilizando, =1 si es Ethernet (el mas comun). 
    uint8_t hlen;           // Indica el tamaño de la dirección MAC del cliente, =6 longitud de la dirección MAC en Ethernet en bytes.
    uint8_t hops;           // Indica el número de saltos que ha dado el paquete, es decir la cantidad de veces que ha sido reenviado por un agente de retransmisión en camino al servidor.
    uint32_t xid;           // Identificador de transacción, es un número aleatorio que se genera para identificar la transacción entre el cliente y el servidor.
    uint16_t secs;          // Indica la cantidad de segundos que han pasado desde que el cliente comenzó a buscar una dirección IP.
    uint16_t flags;         // Si está activo, indica que el cliente está esperando la respuesta por broadcast porque no tiene una dirección IP asignada.
    uint32_t ciaddr;        // La dirección IP del cliente, si la tiene o 0.0.0.0 si aún no la tiene.
    uint32_t yiaddr;        // La dirección IP que el servidor le ofrece al cliente.
    uint32_t siaddr;        // La dirección IP del servidor que está ofreciendo la dirección IP al cliente.
    uint32_t giaddr;        // La dirección IP del agente de retransmisión que reenvió el paquete si el cliente y el servidor están en redes diferentes.
    uint8_t chaddr[16];     // Dirección MAC del cliente, generalmente de 6 bytes en Ethernet pero definida con 16 para posibles extensiones.
    uint8_t sname[64];      // Nombre del servidor, generalmente no se usa.
    uint8_t file[128];      // Nombre del archivo de arranque que el cliente deberia usar al iniciar, se usa solo cuando los dispositivos necesitan un arranqye remoto o boot desde una imagen de red.
    uint8_t options[312];   // Opciones que personalizan en comportamiento del protocolo, se incluye información como el tipo de mensaje (opción 53), identificador del servidor (opción 54), parametros de red solicitados por el cliente (opción 55), etc.
};

// Estructura que define los parametros que se le va a pasar a la función que ejecuta el hilo de salida.
struct exit_args {
    struct dhcp_message *ack_msg;   // Puntero al mensaje DHCP ACK.
    char* interface;                // Nombre de la interfaz para liberar la ip una vez se observe que ya pasó su tiempo de arrendamiento y no se hizo una renovación.
    int send_socket;                // Socket para enviar mensajes de liberación.
    struct sockaddr_in server_addr; // Dirección del servidor (puerto-broadcast).
};

// Estructura que define los parametros que se le va a pasar a la función que ejecuta el hilo de renovación.
struct renew_args {
    struct dhcp_message *ack_msg;   // Puntero al mensaje DHCP ACK.
    const char *interface;          // Nombre de la interfaz.
    int send_socket;                // Socket para enviar mensajes de renovación. 
    int recv_socket;                // Socket para recibir mensajes de renovación.
    struct sockaddr_in server_addr; // Dirección del servidor (puerto-broadcast).
    int *renewed_flag;              // Bandera que indica si se renovó el lease.
};

// Estructura que define los parametros que se le va a pasar a la función que ejecuta el hilo de monitoreo de si una IP se vence.
struct monitor_args {
    struct dhcp_message *ack_msg;   // Puntero al mensaje DHCP ACK.
    char* interface;                // Nombre de la interfaz para liberar la ip una vez se observe que ya pasó su tiempo de arrendamiento y no se hizo una renovación.
    int *renewed_flag;              // Bandera que indica si se renovó el lease.
};

#endif
