#include "socket/socket.h"  
#include "constants/constants.h"
#include "dhcp/dhcp.h"
#include "utils/utils.h"
#include "include/shared_resources.h"

// Definir el mutex globalmente
pthread_mutex_t flag_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

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

    // Definimos la estructura que van a tener los mensajes DHCP.
    struct dhcp_message discover_msg, request_msg;

    // Definimos para almacenar los mensajes.
    char buffer[BUFFER_SIZE];

    // Definimos la estructura para almacenar la longitud del mensaje recibido por parte del servidor.
    ssize_t message_len;

    // Definimos las estructuras para almacenar los mensajes DHCP OFFER y DHCP ACK.
    struct dhcp_message *offer_msg, *ack_msg;

     // Definimos las estructuras que nos van a permitir crear los hilos para manejar la salida del programa y la renovación del lease en simultaneo
    pthread_t thread_exit, thread_renew, thread_monitor;

    // Inicializamos el flag de renovación
    int renewed_flag = 0;

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


    // Enviamos el mensaje como broadcast al puerto 1067.
    if (send_message(send_socket, &discover_msg, &relay_addr, relay_len) < 0) {
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
        printf("-------------------------------\n");
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

    // Enviamos el mensaje como broadcast al puerto 1067.
    if (send_message(send_socket, &request_msg, &relay_addr, relay_len) < 0) {
        exit(EXIT_FAILURE);  
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_RELAY_PORT);


    printf("Esperando mensaje DHCPACK UDP en broadcast...\n");

    // Recibimos mensajes en la interfaz de red hasta obtener el mensaje correspondiente del servidor DHCP.
    while(1) {

        // Almacenamos el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &relay_addr, &relay_len);

        // Procesamos el mensaje recibido y se verifica si es el mensaje que esperamos o no.
        ack_msg = process_msg(buffer);
        if (ack_msg != NULL) {
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
        printf("-------------------------------\n"); 

        // Asignamos la IP a la interface del cliente.
        assign_ip_to_interface(interface, ack_msg);
    } else {
        printf("No se recibió un DHCPACK. Tipo de mensaje recibido: %d\n", ack_type);
    }

    /**************************************************/

    /********************************************************************************/
    /*                  RENOVACIÓN IPS Y FINALIZACIÓN DEL PROGRAMA                  */
    /********************************************************************************/

    // Imprimimos la interfaz del cliente para demostrar que la ip fue definida a la interfaz satisfactoriamente.
    print_network_interface(INTERFACE);

    // Definimos una estructura que almacenará los parámetros para el hilo de salida.
    struct exit_args exit_data = {
        .ack_msg = ack_msg,
        .interface = INTERFACE,
        .send_socket = send_socket,
        .relay_addr = relay_addr,
    };

    // Definimos una estructura que almacenará los parámetros para el hilo de renovación.
    struct renew_args renew_data = {
        .ack_msg = ack_msg,
        .interface = INTERFACE,
        .send_socket = send_socket,
        .recv_socket = recv_socket,
        .relay_addr = relay_addr,
        .renewed_flag = &renewed_flag
    };

    // Definimos una estructura que almacenará los parámetros para el hilo de monitoreo.
    struct monitor_args monitor_data = {
        .ack_msg = ack_msg,
        .interface = INTERFACE,
        .renewed_flag = &renewed_flag
    };

    // Creamos hilos para que en simultaneo que se van haciendo las renovaciones de la ip, se pueda cancelar el programa con la tecla 'q'.
    // Creamos un hilo que se encargará de renovar el lease de la IP. Este cada que lease time llegue al 70% se renovará (4 veces lo hace).
    pthread_create(&thread_renew, NULL, lease_renewal, (void *)&renew_data);
    // Creamos un hilo que espera la tecla 'q' para finalizar el programa y liberar la IP.
    pthread_create(&thread_exit, NULL, exit_program, (void *)&exit_data);
    // Creamos un hilo que monitorea si la IP se venció y no se hizo una renovación.
    pthread_create(&thread_monitor, NULL, monitor_ip, (void *)&monitor_data);

    // Esperamos a que el hilo de renovación termine para que el programa no finalice antes de esto.
    pthread_join(thread_renew, NULL);
    // Esperamos a que el hilo de salida termine para que el programa no finalice antes de esto.
    pthread_join(thread_exit, NULL);
    // Esperamos a que el hilo de monitoreo termine para que el programa no finalice antes de esto.
    pthread_join(thread_monitor, NULL);

    // Cerramos los sockets.
    close(send_socket);
    close(recv_socket);

    return 0;
}