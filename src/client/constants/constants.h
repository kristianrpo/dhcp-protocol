#ifndef CONSTANTS_H
#define CONSTANTS_H

// Definimos el puerto del cliente DHCP. Este es el puerto donde el cliente espera y envia mensajes de respuesta DHCP.
#define DHCP_CLIENT_PORT 1068

// Definimos el puerto del relay DHCP. Este es el puerto donde el relay envía y recibe mensajes de broadcast.
#define DHCP_RELAY_PORT 1067

// Definimos la dirección IP de broadcast.
#define BROADCAST_IP "255.255.255.255"

// Definimos el tamaño del buffer para almacenar los mensajes.
#define BUFFER_SIZE 1024

// Definimos la interfaz de red que se va a utilizar desde el cliente.
#define INTERFACE "enp0s3"

#endif 
