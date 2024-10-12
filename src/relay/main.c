#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket/socket.h"
#include "structs/relay_structs.h"
#include "constants/relay_constants.h"
#include "error/error.h"

int main() {
    // Definimos un buffer para almacenar los datos recibidos de manera temporal, para así posteriormente procesarlos. 
    char buffer[BUFFER_SIZE];

    // Definición de variable fd, que contiene el socket creado.
    int fd;

    // Definici[on de variable message_len, que almacena la longitud del mensaje recibido.
    ssize_t message_len;

    // Se define la estructura para almacenar la información del cliente, ya que el dhcp relay necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del cliente.
    struct sockaddr_in client_addr;

    // Se define la estructura para almacenar la información del servidor, ya que el dhcp relay necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del servidor.
    struct sockaddr_in server_addr;

    // Se define la estructura para almacenar la información del servidor, ya que el dhcp relay necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del servidor.
    struct sockaddr_in relay_addr;

    // Tamaño de la dirección del cliente (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t client_len = sizeof(client_addr);

    // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t server_len = sizeof(server_addr);

    // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t relay_len = sizeof(relay_addr);

    // Inicializar el socket.
    fd = initialize_socket(&relay_addr, relay_len);

    while (1) {
        // Recibir mensaje DHCP ya sea del cliente o del servidor.
        message_len = receive_message_client(fd, buffer, &client_addr, &client_len);

        printf("DHCPDISCOVER recibido del cliente.\n");

        // El mensaje que llega al servidor DHCP es una secuencia de bits, pudiendose considerar que la información está encapsulada, C como tal no es capaz de decodificar esta estructura para acceder a la información, por lo tanto, debemos definir una estructura que permita convertir esos simples bienarios en información accesible y operable para el servidor.
        // Convertir el buffer a un mensaje DHCP. Se utiliza el casting de C para tratar bits crudos como una estructura.
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        // Se asigna la IP del relay en el campo correspondiente del mensaje DHCP.
        msg->giaddr = relay_addr.sin_addr.s_addr; 

        // Configuración de la estructura donde se va a almacenar la ip y el puerto del servidor.
        // Esto es necesario puesto que el dhcp relay debe conocer a que socket se le debe reenviar el mensaje en el servidor.
        // Definición IPv4.
        server_addr.sin_family = AF_INET;

        // Definición del puerto del servidor.
        server_addr.sin_port = htons(DHCP_SERVER_PORT);

        // Definición de la dirección IP del servidor.
        server_addr.sin_addr.s_addr = inet_addr(IP_SERVER_IDENTIFIER); 

        // Reenviar el mensaje modificado al servidor DHCP.
        if (sendto(fd, buffer, message_len, 0,
                   (struct sockaddr *)&server_addr, server_len) < 0) {
            error("Error al reenviar al servidor DHCP");
            continue;
        }

        printf("DHCPDISCOVER reenviado al servidor DHCP.\n");


        // Recibir la respuesta del servidor.
        message_len = receive_message_server(fd, buffer, &server_addr, &server_len);
        printf("DHCPOFFER recibido del servidor.\n");

        // Modificar la dirección del cliente para enviar el mensaje en broadcast.
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(DHCP_CLIENT_PORT); 
        client_addr.sin_addr.s_addr = inet_addr("255.255.255.255");  

        // Reenviar la respuesta al cliente en broadcast.
        if (sendto(fd, buffer, message_len, 0, (struct sockaddr*)&client_addr, client_len) < 0) {
            error("Error al reenviar la respuesta en broadcast al cliente");
            continue;
        }

        printf("DHCPOFFER reenviado al cliente.\n");

    }
    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos.
    close(fd);
    return 0;
}
