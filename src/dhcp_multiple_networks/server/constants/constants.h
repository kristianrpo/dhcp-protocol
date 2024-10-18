#ifndef CONSTANTS_H
#define CONSTANTS_H

// Puerto que se va a utilizar para el servidor y se va a asociar al socket.
#define DHCP_SERVER_PORT 1067

// Tamaño del buffer (lo que puede almacenar).
#define BUFFER_SIZE 576

// Maximo numero de arrendamientos.
#define MAX_LEASES 50

// Dirección IP inicial para asignar a los clientes (pool de direcciones).
#define START_IP "192.168.56.21"

// Duración de un arrendamiento cuando se reserva a un cliente en segundos.
#define LEASE_DURATION_RESERVED 10

// Duración de un arrendamiento cuando se asigna a un cliente en segundos.
#define LEASE_DURATION_OCCUPIED 15

// Valor para representar un error.
#define IP_ERROR 0xFFFFFFFF

// Dirección IP del servidor.
#define IP_SERVER_IDENTIFIER "192.168.57.3" 

#endif
