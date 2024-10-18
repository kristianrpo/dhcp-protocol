#include "socket/socket.h"
#include "structs/structs.h"
#include "constants/constants.h"
#include "error/error.h"
#include "dhcp/dhcp.h"

int main() {
    /**************************************************/
    /*      DEFINICIÓN DE VARIABLES A UTILIZAR        */
    /**************************************************/

    // Definimos el buffer para almacenar los mensajes.
    char buffer[BUFFER_SIZE];

    // Definimos la variable que va a almacenar el id del socket.
    int fd;

    // Definimos la variable que va a almacenar la longitud del mensaje recibido.
    ssize_t message_len;

    // Definimos las estructuras para almacenar las direcciones del cliente, del servidor y del relay.
    struct sockaddr_in client_addr, server_addr, relay_addr;

    // Definimos las longitudes de las direcciones del cliente, del servidor y del relay.
    socklen_t client_len = sizeof(client_addr), server_len = sizeof(server_addr), relay_len = sizeof(relay_addr);

    /**************************************************/
    
    // Inicializamos el socket.
    fd = initialize_socket(&relay_addr, relay_len);

    while (1) {

        // Recibimos el mensaje DHCP ya sea del cliente o del servidor.
        message_len = receive_message(fd, buffer, &client_addr, &client_len);

        // Convertimos el buffer a un mensaje DHCP.
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        // Obtenemos el tipo de mensaje DHCP.
        int dhcp_type = get_dhcp_message_type(msg);

        if (dhcp_type == 1) {
            printf("DHCPDISCOVER recibido del cliente.\n");
        }
        else if (dhcp_type == 3) {
            printf("DHCPREQUEST recibido del cliente.\n");
        }
        else if (dhcp_type == 7) {
            printf("DHCPRELEASE recibido del cliente.\n");
        }

        // Asignamos la IP del relay en el campo correspondiente del mensaje DHCP.
        msg->giaddr = relay_addr.sin_addr.s_addr;

        // Configuramos de la estructura de servidor.
        // Definimos la familia de direcciones del socket (IPv4).
        server_addr.sin_family = AF_INET;
        // Configuración del puerto del servidor.
        server_addr.sin_port = htons(DHCP_SERVER_PORT);
        // Definimos la ip del servidor.
        server_addr.sin_addr.s_addr = inet_addr(IP_SERVER_IDENTIFIER);

        /**************************************************/
        /*              CLIENTE->SERVIDOR                 */
        /**************************************************/

        // Reenviamos el mensaje DHCP al servidor.
        if (dhcp_type == 1) {
            printf("Reenviando DHCPDISCOVER al servidor...\n");
        }
        else if (dhcp_type == 3) {
            printf("Reenviando DHCPREQUEST al servidor...\n");
        }
        else if (dhcp_type == 7) {
            printf("Reenviando DHCPRELEASE al servidor...\n");
        }

        // Enviamos el mensaje al servidor.
        if (send_message(fd, buffer, message_len, &server_addr, server_len,0) < 0) {
            exit(EXIT_FAILURE);  
        }

        /**************************************************/

        /**************************************************/
        /*              SERVIDOR->CLIENTE                 */
        /**************************************************/

        // Recibimos el mensaje DHCP del servidor.
        message_len = receive_message(fd, buffer, &server_addr, &server_len);
        if (dhcp_type == 1) {
            printf("DHCPOFFER recibido del servidor.\n");
        }
        else if (dhcp_type == 3) {
            printf("DHCPACK recibido del servidor.\n");
        }

        // Configuramos de la estructura de cliente.
        // Definimos la familia de direcciones del socket (IPv4).
        client_addr.sin_family = AF_INET;
        // Configuración del puerto del cliente.
        client_addr.sin_port = htons(DHCP_CLIENT_PORT);
        // Definimos la ip del cliente (broadcast).
        client_addr.sin_addr.s_addr = inet_addr("192.168.56.255");

        // Enviamos el mensaje como broadcast al puerto 1067 del cliente.
        if (send_message(fd, buffer, message_len, &client_addr, client_len,1) < 0) {
            exit(EXIT_FAILURE);  
        }
        
        if(dhcp_type == 1){
            printf("DHCPOFFER reenviado al cliente.\n");
        }
        else if(dhcp_type == 3){
            printf("DHCPACK reenviado al cliente.\n");
        }

        /**************************************************/

    }

    // Cerramos el socket.
    close(fd);
    return 0;
}
