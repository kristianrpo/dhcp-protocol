#ifndef DHCP_STRUCTS_H
#define DHCP_STRUCTS_H

#include "dhcp_constants.h" 
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>

// Estructura para representar un arrendamiento
struct lease_entry {
    uint8_t mac_addr[6];  // Dirección MAC del cliente
    uint32_t ip_addr;     // Dirección IP asignada (en formato binario)
    time_t lease_start;   // Tiempo de inicio del arrendamiento
    int lease_duration;   // Duración del arrendamiento (en segundos)
    int state;            // 1 si está reservada, 0 si está libre, -1 ocupada.
};

// Estructura para representar un mensaje DHCP
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

// Estructura para los argumentos de los hilos. Esta está diseñada para contener todos los datos que cada hilo necesitará para procesar una solicitud de cliente DHCP.
struct thread_args {
    int fd;                             // id del socket.
    struct sockaddr_in client_addr;     // dirección del cliente.
    socklen_t client_len;               // longitud de la dirección del cliente.
    char buffer[BUFFER_SIZE];           // buffer que almacena los datos recibidos de manera temporal.
    struct lease_entry *leases;         // tabla de arrendamientos.
};

#endif
