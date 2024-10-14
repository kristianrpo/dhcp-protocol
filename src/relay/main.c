#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <net/if.h>
#include "socket/socket.h"
#include "structs/relay_structs.h"
#include "constants/relay_constants.h"
#include "error/error.h"

#define INTERFACE "enp0s3"  // Especificar la interfaz de red


int get_dhcp_message_type(struct dhcp_message *msg) {
    // Definimos la variable de control para recorrer el campo de options.
    int i = 0;
    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 53, que es la que contiene el mensaje para la acción a realizar. Se recorre hasta 312 porque el campo options como maximo puede medir 312 bytes según estandarización.
    while (i < 312) {
        // Verificamos si la opción es 255, que indica el final de las opciones y salimos del ciclo.
        if (msg->options[i] == 255) {
            break;
        }
        
        // Revisamos si estamos en la opción 53 (Tipo de mensaje DHCP).
        if (msg->options[i] == 53) {
            // Aseguramos que la longitud del mensaje DHCP denota que el mismo existe.
            if (msg->options[i + 1] == 1) {
                // El valor del tipo de mensaje está en el tercer byte.
                return msg->options[i + 2]; 
                
            } else {
                // Retornamos 'error' si la longitud del mensaje DHCP no es la esperada.
                return -1;
            }
        }
        
        // Avanzamos al siguiente campo de opciones, se avanza de esta manera puesto que cada opcion mide diferente, asi que se obtiene la siguiente opción de manera 'dinamica'.
        i += msg->options[i + 1] + 2;
    }
    // Devolvemos 'error' si no encontramos la opción 53.
    return -1;  
}

int main() {
    char buffer[BUFFER_SIZE];
    int fd;
    ssize_t message_len;
    struct sockaddr_in client_addr, server_addr, relay_addr;
    socklen_t client_len = sizeof(client_addr), server_len = sizeof(server_addr), relay_len = sizeof(relay_addr);

    // Inicializar el socket
    fd = initialize_socket(&relay_addr, relay_len);

    int broadcast_enable = 1;
    struct ifreq ifr;

    while (1) {
        // Recibir mensaje DHCP ya sea del cliente o del servidor.
        message_len = receive_message_client(fd, buffer, &client_addr, &client_len);
        printf("DHCP recibido del cliente.\n");

        // Convertir el buffer a un mensaje DHCP
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        // Obtener el tipo de mensaje DHCP
        int dhcp_type = get_dhcp_message_type(msg);

        // Se asigna la IP del relay en el campo correspondiente del mensaje DHCP
        msg->giaddr = relay_addr.sin_addr.s_addr;

        // Configuración de la estructura de servidor
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(DHCP_SERVER_PORT);
        server_addr.sin_addr.s_addr = inet_addr(IP_SERVER_IDENTIFIER);

        if (dhcp_type == 1) {  // DHCPDISCOVER
            printf("Reenviando DHCPDISCOVER al servidor...\n");

            if (sendto(fd, buffer, message_len, 0, (struct sockaddr *)&server_addr, server_len) < 0) {
                error("Error al reenviar DHCPDISCOVER al servidor");
                continue;
            }

            // Recibir DHCPOFFER del servidor
            message_len = receive_message_server(fd, buffer, &server_addr, &server_len);
            printf("DHCPOFFER recibido del servidor.\n");

            // Reenviar la respuesta DHCPOFFER al cliente
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(DHCP_CLIENT_PORT);
            client_addr.sin_addr.s_addr = inet_addr("192.168.56.255");

            if (sendto(fd, buffer, message_len, 0, (struct sockaddr*)&client_addr, client_len) < 0) {
                error("Error al reenviar DHCPOFFER al cliente");
                continue;
            }

            printf("DHCPOFFER reenviado al cliente.\n");

        } else if (dhcp_type == 3) {  // DHCPREQUEST
            printf("Reenviando DHCPREQUEST al servidor...\n");

            if (sendto(fd, buffer, message_len, 0, (struct sockaddr *)&server_addr, server_len) < 0) {
                error("Error al reenviar DHCPREQUEST al servidor");
                continue;
            }

            // Recibir DHCPACK del servidor
            message_len = receive_message_server(fd, buffer, &server_addr, &server_len);
            printf("DHCPACK recibido del servidor.\n");

            // Reenviar la respuesta DHCPOFFER al cliente
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(DHCP_CLIENT_PORT);
            client_addr.sin_addr.s_addr = inet_addr("192.168.56.255");

            if (sendto(fd, buffer, message_len, 0, (struct sockaddr*)&client_addr, client_len) < 0) {
                error("Error al reenviar DHCPOFFER al cliente");
                continue;
            }

            printf("DHCPACK reenviado al cliente.\n");
        }
    }

    close(fd);
    return 0;
}
