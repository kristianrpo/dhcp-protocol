#ifndef DHCP_CONSTANTS_H
#define DHCP_CONSTANTS_H

#define DHCP_SERVER_PORT 1067 // Puerto que se va a utilizar para el servidor y se va a asociar al socket.
#define BUFFER_SIZE 576 // Tamaño del buffer (lo que puede almacenar).

#define MAX_LEASES 50 // Maximo numero de arrendamientos.
#define START_IP "192.168.0.21" // Dirección IP inicial para asignar a los clientes.
#define LEASE_DURATION_RESERVED 10 // Duración de un arrendamiento cuando se reserva a un cliente en segundos.
#define LEASE_DURATION_OCCUPIED 3600 // Duración de un arrendamiento cuando se asigna a un cliente en segundos.
#define IP_ERROR 0xFFFFFFFF // 255.255.255.255 para representar error.
#define IP_SERVER_IDENTIFIER "192.168.1.100" // Dirección IP del servidor.

#endif
