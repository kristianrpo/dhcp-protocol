#include "socket/socket.h"  
#include "constants/constants.h"
#include "dhcp/dhcp.h"
#include "utils/utils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // Para sleep()
#include <termios.h>
#include <fcntl.h>

// Function prototypes
void release_ip();
void print_network_interface();

// Crear la estructura para pasar al hilo de salida
struct exit_args {
    struct dhcp_message *ack_msg;  // Puntero al mensaje DHCP ACK
    const char *interface;         // Nombre de la interfaz
};

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
// Function to release the IP address from the network interface
void release_ip(const char *interface, struct dhcp_message *msg) {
    char command[256];
    struct in_addr addr;
    addr.s_addr = msg->yiaddr;

    snprintf(command, sizeof(command), "sudo ip addr del %s/24 dev %s", inet_ntoa(addr), interface);
    system(command);

}

// Function to print the network interface details using `ip addr show`
void print_network_interface() {
    printf("Información de la interfaz de red actual:\n");
    system("ip addr show enp0s3");
}

// Función para manejar la salida del programa
void *exit_program(void *arg) {
    struct exit_args *args = (struct exit_args *)arg;
    printf("Presionar la tecla Q para finalizar la ejecución del programa y liberar la ip \n");
    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == 'q') {
                printf("\nTecla 'q' presionada. Saliendo del programa...\n");
                // Liberar la IP antes de salir
                release_ip(args->interface, args->ack_msg);
                exit(0);  // Finaliza el programa
            }
        }
        usleep(100000);  // Espera de 100 ms para no consumir muchos recursos
    }
    return NULL;
}

int main() {
    /**************************************************/
    /*      DEFINICIÓN DE VARIABLES A UTILIZAR        */
    /**************************************************/

    // Inicializamos el socket para enviar (SOCK_DGRAM) y recibir (SOCK_RAW) mensajes.
    int send_socket;
    int recv_socket;

    // Definimos la interfaz de red que se va a utilizar desde el cliente (enp0s3).
    char* interface = INTERFACE;

    // Definimos las estructuras para almacenar las direcciones del dhcp server y del cliente.
    struct sockaddr_in server_addr,  client_addr;

    // Definimos la longitud de las direcciones del cliente y del server.
    socklen_t client_len = sizeof(client_addr);
    socklen_t server_len = sizeof(server_addr);

    // Se define la estructura que van a tener los mensajes DHCP.
    struct dhcp_message discover_msg, request_msg;

    // Buffer para almacenar los mensajes.
    char buffer[BUFFER_SIZE];

    // Definimos la estructura para almacenar la longitud del mensaje recibido por parte del servidor.
    ssize_t message_len;

    // Definimos las estructuras para almacenar los mensajes DHCP OFFER y DHCP ACK.
    struct dhcp_message *offer_msg, *ack_msg;

    // Definimos las estructuras que nos van a permitir crear los hilos para manejar la salida del programa y la renovación del lease en simultaneo
    pthread_t thread_exit, thread_renew;

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

    // Definimos los datos necesarios para enviar el mensaje de broadcast al server.
    // Definimos la dirección IP del server.
    server_addr.sin_family = AF_INET;
    // Configuración del puerto del server.
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del server.
    server_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);


    // Enviamos el mensaje como broadcast al puerto 1067.
    if (send_message(send_socket, &discover_msg, &server_addr, server_len) < 0) {
        exit(EXIT_FAILURE);  
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_SERVER_PORT);


    // Creamos el socket RAW socket para poder recibir mensajes broadcast sin tener en la interfaz una ip asignada.
    recv_socket = initialize_RAW_socket(&client_addr, client_len);

    printf("Esperando mensaje DHCP OFFER UDP en broadcast...\n");

    // Recibimos mensajes en la interfaz de red hasta obtener el mensaje correspondiente del servidor DHCP.
    while (1) {

        // Almacenamos el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &server_addr, &server_len);

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
        printf("-------------------------------\n");
        printf("Configuración de red ofrecida: \n");
        printf("-------------------------------\n"); 
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

    // Definimos los datos necesarios para enviar el mensaje de broadcast al server.
    // Definimos los datos necesarios para enviar el mensaje de broadcast al server.
    server_addr.sin_family = AF_INET;
    // Configuración del puerto del server.
    server_addr.sin_port = htons(DHCP_SERVER_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del server.
    server_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviamos el mensaje como broadcast al puerto 1067.
    if (send_message(send_socket, &request_msg, &server_addr, server_len) < 0) {
        exit(EXIT_FAILURE);  
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_SERVER_PORT);


    printf("Esperando mensaje DHCPACK UDP en broadcast...\n");

    // Recibimos mensajes en la interfaz de red hasta obtener el mensaje correspondiente del servidor DHCP.
    while(1) {

        // Almacenamos el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &server_addr, &server_len);

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
        printf("-------------------------------\n");
        printf("Configuración de red recibida: \n");
        printf("-------------------------------\n"); 

        // Imprimimos la configuración de red recibida.
        print_network_config(ack_msg);

        printf("-------------------------------\n"); 

        // Asignamos la IP a la interface del cliente.
        assign_ip_to_interface(interface, ack_msg);
    } else {
        printf("No se recibió un DHCPACK. Tipo de mensaje recibido: %d\n", ack_type);
    }

    // Capture the SIGINT signal (Ctrl+C)

    // Show the network interface details
    print_network_interface();

    struct exit_args exit_data = {
        .ack_msg = ack_msg,
        .interface = INTERFACE
    };


    // Crear el hilo principal que espera la tecla 'q' para finalizar el programa y liberar la IP.
    pthread_create(&thread_exit, NULL, exit_program, (void *)&exit_data);

    // Esperar a que ambos hilos terminen
    pthread_join(thread_exit, NULL);

    
    // Cerramos los sockets.
    close(send_socket);
    close(recv_socket);

    return 0;
}
