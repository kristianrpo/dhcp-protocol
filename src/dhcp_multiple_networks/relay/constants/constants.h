#ifndef CONSTANTS_H
#define CONSTANTS_H

// Definimos el puerto del servidor DHCP. Este es el puerto donde el servidor espera y envia mensajes de respuesta DHCP.
#define DHCP_SERVER_PORT 1067

// Definimos el puerto del cliente DHCP. Este es el puerto donde el cliente espera y envia mensajes DHCP.
#define DHCP_CLIENT_PORT 1068

// Definimos el puerto del relay DHCP. Este es el puerto donde el relay envía y recibe mensajes de broadcast.
#define DHCP_RELAY_PORT 1067

// Definimos la dirección IP del servidor.
#define IP_SERVER_IDENTIFIER "192.168.57.3"

// Definimos el tamaño del buffer para almacenar los mensajes.
#define BUFFER_SIZE 576

// Definimos la interfaz de red que se va a utilizar para mandar mensajes a el cliente.
#define CLIENT_ASSOCIATED_INTERFACE "enp0s3" 

// Definimos la interfaz de red que se va a utilizar para mandar mensajes a el servidor.
#define SERVER_ASSOCIATED_INTERFACE "enp0s8"

#endif 
