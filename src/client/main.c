#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "socket/socket.h"  
#include "constants/constants.h"
#include "dhcp/dhcp.h"
#include "utils/utils.h"

int main() {
    /**************************************************/
    /*      DEFINICIÓN DE VARIABLES A UTILIZAR        */
    /**************************************************/

    // Inicializamos el socket para enviar (SOCK_DGRAM) y recibir (SOCK_RAW) mensajes.
    int send_socket;
    int recv_socket;

    // Definimos la interfaz de red que se va a utilizar desde el cliente (enp0s3).
    char* interface = INTERFACE;

    // Definimos las estructuras para almacenar las direcciones del dhcp relay y del cliente.
    struct sockaddr_in relay_addr,  client_addr;

    // Definimos la longitud de las direcciones del cliente y del relay.
    socklen_t client_len = sizeof(client_addr);
    socklen_t relay_len = sizeof(relay_addr);

    // Se define la estructura que van a tener los mensajes DHCP.
    struct dhcp_message discover_msg, request_msg;

    // Buffer para almacenar los mensajes.
    char buffer[BUFFER_SIZE];

    // Definimos la estructura para almacenar la longitud del mensaje recibido por parte del servidor.
    ssize_t message_len;

    // Definimos las estructuras para almacenar los mensajes DHCP OFFER y DHCP ACK.
    struct dhcp_message *offer_msg, *ack_msg;

    /**************************************************/


    /**************************************************/
    /*                  DHCP DISCOVER                 */
    /**************************************************/

    // Inicializamos el socket udp que nos va a permitir hacer mensajes broadcast.
    send_socket = initialize_DGRAM_socket(&client_addr, client_len);

    // Obtenemos la MAC del cliente asociada a la interface enp0s3.
    uint8_t *chaddr = get_mac_address(interface);
    
    // Configuramos los parametros principales del mensaje DHCP.
    configure_dhcp_message(&discover_msg, 1, 1, 6,  htonl(0x12345678), htons(0x8000), chaddr);

    // Definimos el index que nos va a permitir ubicar en que parte del options del mensaje DHCP vamos.
    int discover_index = 0;
    
    // Configuramos los campos del mensaje DHCP para que sea un mensaje de discover.
    set_type_message(discover_msg.options, &discover_index, 53, 1, 1);

    // Definimos el final de las opciones.
    discover_msg.options[discover_index] = 255;

    // Definimos los datos necesarios para enviar el mensaje de broadcast al relay.
    // Definimos la dirección IP del relay.
    relay_addr.sin_family = AF_INET;
    // Configuración del puerto del relay.
    relay_addr.sin_port = htons(DHCP_RELAY_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del relay.
    relay_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);


    // Enviamos el mensaje de broadcast al puerto del relay.
    if (sendto(send_socket, &discover_msg, sizeof(discover_msg), 0, (struct sockaddr*)&relay_addr, relay_len) < 0) {
        error("Error al enviar mensaje");
        close(send_socket);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_RELAY_PORT);


    // Creamos el socket RAW socket para poder recibir mensajes broadcast sin tener en la interfaz una ip asignada.
    recv_socket = initialize_RAW_socket(&client_addr, client_len);

    printf("Esperando mensaje DHCP OFFER UDP en broadcast...\n");

    // Recibimos mensajes en la interfaz de red hasta obtener el mensaje correspondiente del servidor DHCP.
    while (1) {

        // Almacenamos el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &relay_addr, &relay_len);

        // Procesamos el mensaje recibido y se verifica si es el mensaje que esperamos o no.
        offer_msg = process_msg(buffer);
        if (offer_msg != NULL) {
            printf("Mensaje UDP recibido\n");
            break;
        }
    }

    /**************************************************/

    /**************************************************/
    /*                  DHCP REQUEST                  */
    /**************************************************/

    // Obtenemos el tipo de mensaje DHCP que recibimos.
    int dhcp_type = get_dhcp_message_type(offer_msg);

    // Verificamos que el mensaje recibido del servidor DHCP sea un DHCPOFFER.
    if (dhcp_type == 2) {
        printf("Mensaje DHCPOFFER recibido correctamente\n");
        print_network_config(offer_msg);
    } else {
        printf("No se recibió un DHCPOFFER. Tipo de mensaje recibido: %d\n", dhcp_type);
    }

    // Limpiamos el buffer para poder enviar un nuevo mensaje request.
    memset(&request_msg, 0, sizeof(request_msg));

    // Configuramos los parametros principales del mensaje DHCP.
    configure_dhcp_message(&request_msg, 1, 1, 6, offer_msg->xid, htons(0x8000), chaddr);

    // Definimos el index que nos va a permitir ubicar en que parte del options del mensaje DHCP vamos.
    int request_index = 0;
    
    // Configuramos el mensaje DHCP para que sea un mensaje de tipo request.
    set_type_message(request_msg.options, &request_index, 53, 1, 3);

    // Configuramos la IP solicitada en el mensaje DHCP.
    set_requested_ip(request_msg.options, &request_index, offer_msg->yiaddr);

    // Configuramos la dirección IP del servidor en el mensaje DHCP.
    uint32_t server_ip = get_server_identifier(offer_msg);
    set_server_identifier(request_msg.options, &request_index, server_ip);

    
    // Establecemos el final de las opciones.
    request_msg.options[request_index] = 255;

    // Definimos los datos necesarios para enviar el mensaje de broadcast al relay.
    // Definimos los datos necesarios para enviar el mensaje de broadcast al relay.
    relay_addr.sin_family = AF_INET;
    // Configuración del puerto del relay.
    relay_addr.sin_port = htons(DHCP_RELAY_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del relay.
    relay_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviamos el mensaje de broadcast al puerto del relay.
    if (sendto(send_socket, &request_msg, sizeof(request_msg), 0, (struct sockaddr*)&relay_addr, relay_len) < 0) {
        perror("Error al enviar mensaje");
        close(send_socket);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_RELAY_PORT);


    printf("Esperando mensaje DHCP ACK UDP en broadcast...\n");

    // Recibimos mensajes en la interfaz de red hasta obtener el mensaje correspondiente del servidor DHCP.
    while(1) {

        // Almacenamos el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &relay_addr, &relay_len);

        // Procesamos el mensaje recibido y se verifica si es el mensaje que esperamos o no.
        ack_msg = process_msg(buffer);
        if (ack_msg != NULL) {
            printf("Mensaje UDP recibido\n");
            break;
        }
    }

    // Obtenemos el tipo de mensaje DHCP que recibimos.
    int ack_type = get_dhcp_message_type(ack_msg);

    // Verificamos que el mensaje recibido del servidor DHCP sea un DHCPACK.
    if (ack_type == 5) {
        printf("Mensaje DHCPACK recibido correctamente\n");

        // Imprimimos la configuración de red recibida.
        print_network_config(ack_msg);

        // Asignamos la IP a la interface del cliente.
        assign_ip_to_interface(interface, ack_msg);
    } else {
        printf("No se recibió un DHCPACK. Tipo de mensaje recibido: %d\n", ack_type);
    }

    // Cerramos los sockets.
    close(send_socket);
    close(recv_socket);

    return 0;
}
