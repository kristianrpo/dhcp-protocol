#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>      // Define tipos de enteros con tamaños específicos (int8_t, uint32_t, etc.), garantizando el uso de enteros de tamaño fijo, independientemente de la arquitectura del sistema. Es útil cuando se necesita trabajar con datos cuyo tamaño debe ser exacto, como en redes o sistemas embebidos.
#include <time.h>        // Proporciona funciones para la manipulación de fechas y horas, como time(), localtime(), difftime(), y estructuras como struct tm para almacenar fechas y horas de forma estructurada.
#include <netinet/in.h>  // Proporciona definiciones y estructuras para trabajar con direcciones de red y protocolos de Internet (IPv4 e IPv6), como sockaddr_in, sockaddr_in6 y funciones para la conversión de direcciones y puertos entre formatos.

#include "../constants/constants.h" 

// Estructura para representar un arrendamiento
struct lease_entry {
    uint8_t mac_addr[6];  // Dirección MAC del cliente.
    uint32_t ip_addr;     // Dirección IP asignada (en formato binario).
    time_t lease_start;   // Tiempo de inicio del arrendamiento.
    int lease_duration;   // Duración del arrendamiento (en segundos).
    int state;            // Estado de la ip, =1 si está reservada, =0 si está libre, =-1 si está ocupada.
};

// Estructura para representar un mensaje DHCP
struct dhcp_message {
    uint8_t op;             // Tipo de operación del paquete, =1 si es una solicitud proveniente de un cliente, =2 si es una respuesta proveniente del servidor.      
    uint8_t htype;          // Tipo de hardware que se está utilizando, =1 si es Ethernet (el mas común). 
    uint8_t hlen;           // Tamaño de la dirección MAC del cliente, =6 longitud de la dirección MAC en Ethernet en bytes.
    uint8_t hops;           // Número de saltos que ha dado el paquete, es decir la cantidad de veces que ha sido reenviado por un agente de retransmisión en camino al servidor.
    uint32_t xid;           // Identificador de transacción, es un número aleatorio que se genera para identificar la transacción entre el cliente y el servidor.
    uint16_t secs;          // Cantidad de segundos que han pasado desde que el cliente comenzó a buscar una dirección IP.
    uint16_t flags;         // Si está activo, indica que el cliente está esperando la respuesta por broadcast porque no tiene una dirección IP asignada.
    uint32_t ciaddr;        // Dirección IP del cliente, si la tiene o 0.0.0.0 si aún no la tiene.
    uint32_t yiaddr;        // Dirección IP que el servidor le ofrece al cliente.
    uint32_t siaddr;        // Drección IP del servidor que está ofreciendo la dirección IP al cliente.
    uint32_t giaddr;        // Dirección IP del agente de retransmisión que reenvió el paquete si el cliente y el servidor están en redes diferentes.
    uint8_t chaddr[16];     // Dirección MAC del cliente, generalmente de 6 bytes en Ethernet pero definida con 16 para posibles extensiones.
    uint8_t sname[64];      // Nombre del servidor, generalmente no se usa.
    uint8_t file[128];      // Nombre del archivo de arranque que el cliente deberia usar al iniciar, se usa solo cuando los dispositivos necesitan un arranqye remoto o boot desde una imagen de red.
    uint8_t options[312];   // Opciones que personalizan en comportamiento del protocolo, se incluye información como el tipo de mensaje (opción 53), identificador del servidor (opción 54), parametros de red solicitados por el cliente (opción 55), etc.
};

// Estructura para los argumentos de los hilos. Esta está diseñada para contener todos los datos que cada hilo necesitará para procesar una solicitud de cliente DHCP.
struct thread_args {
    int fd;                             // Id del socket.
    struct sockaddr_in client_addr;      // Dirección del cliente.
    socklen_t client_len;                // Longitud de la dirección del cliente.
    char buffer[BUFFER_SIZE];           // Buffer que almacena los datos recibidos de manera temporal.
    struct lease_entry *leases;         // Tabla de arrendamientos.
};

#endif
