#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "socket/socket.h"  
#include "constants/client_constants.h"
#include "dhcp/client_dhcp.h"


// Función para verificar si el paquete es IP
int is_ip_packet(struct ethhdr *eth) {
    return ntohs(eth->h_proto) == ETH_P_IP;
}

// Función para verificar si el paquete es UDP
int is_udp_packet(struct iphdr *ip_header) {
    return ip_header->protocol == IPPROTO_UDP;
}

// Función para obtener la cabecera UDP
struct udphdr *get_udp_header(char *buffer, struct iphdr *ip_header) {
    return (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_header->ihl * 4);
}

// Función para obtener el payload (datos) del paquete UDP
char *get_udp_payload(char *buffer, struct iphdr *ip_header, struct udphdr *udp_header) {
    return (char *)(buffer + sizeof(struct ethhdr) + ip_header->ihl * 4 + sizeof(struct udphdr));
}

// Función para procesar el paquete y extraer el payload UDP si es un paquete para el puerto 1069
struct dhcp_message* process_msg(char *buffer) {
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    // Verificar si es un paquete IP
    if (is_ip_packet(eth)) {
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        // Verificar si es un paquete UDP
        if (is_udp_packet(ip_header)) {
            struct udphdr *udp_header = get_udp_header(buffer, ip_header);

            unsigned short dest_port = ntohs(udp_header->dest);

            unsigned short src_port = ntohs(udp_header->source);

            if (dest_port == 1068 && src_port == 1067) {
                printf("Mensaje UDP broadcast recibido en el puerto 1068\n");

                // Obtener los datos del mensaje (payload)
                char *data = get_udp_payload(buffer, ip_header, udp_header);

                // Mapear el payload como una estructura DHCP
                struct dhcp_message *msg = (struct dhcp_message *)data;
                return msg;
            }
        }
    }
    return NULL;
}


void print_network_config(struct dhcp_message *msg) {
    int i = 0;
    struct in_addr addr;
    printf("Configuración de red recibida:\n");
    printf("-------------------------------\n");    
    struct in_addr ip_addr;
    ip_addr.s_addr = msg->yiaddr; 
    printf("Dirección IP: %s\n", inet_ntoa(ip_addr));  
    while (i < 312) {
        uint8_t option = msg->options[i];
        uint8_t len = msg->options[i + 1];

        if (option == 1) {  // Máscara de red
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Máscara de red: %s\n", inet_ntoa(addr));
        } else if (option == 3) {  // Gateway predeterminado
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Gateway: %s\n", inet_ntoa(addr));
        } else if (option == 6) {  // Servidor DNS
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Servidor DNS: %s\n", inet_ntoa(addr));
        }

        if (option == 255) {
            break;
        }

        i += len + 2;
    }
}


// Función para obtener la dirección MAC y retornarla como un puntero
uint8_t *get_mac_address(const char *interface) {
    int relay_socket;
    struct ifreq ifr;
    static uint8_t mac[6];  // Variable estática para que no se pierda el valor al retornar el puntero

    // Crear un socket para la solicitud
    relay_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (relay_socket < 0) {
        error("Error al crear socket");
        return NULL;
    }

    // Copiar el nombre de la interfaz a la estructura ifreq
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

    // Hacer la solicitud ioctl para obtener la dirección MAC
    if (ioctl(relay_socket, SIOCGIFHWADDR, &ifr) == -1) {
        error("Error al obtener dirección MAC");
        close(relay_socket);
        return NULL;
    }

    // Copiar la dirección MAC obtenida a la variable estática
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

    close(relay_socket);
    return mac;  // Retornar el puntero a la dirección MAC
}

void assign_ip_to_interface(const char *interface, struct dhcp_message *msg) { 
    char command[256];

    struct in_addr ip_addr;
    char subnet_prefix[4] = "24";  // Prefijo quemado /24

    // Extraer la IP asignada desde el campo yiaddr del mensaje DHCPACK.
    ip_addr.s_addr = msg->yiaddr;

    // Asignar la nueva dirección IP con el prefijo quemado
    snprintf(command, sizeof(command), "sudo ip addr add %s/%s dev %s", inet_ntoa(ip_addr), subnet_prefix, interface);
    system(command);

    // Activar la interfaz
    snprintf(command, sizeof(command), "sudo ip link set dev %s up", interface);
    system(command);
}

int main() {
    // Inicializar el socket
    int send_socket;
    int recv_socket;

    // Se define el nombre de la interfaz de red
    char* interface = INTERFACE;

    // Estructuras para almacenar las direcciones del dhcp relay y del cliente.
    struct sockaddr_in relay_addr,  client_addr;

    // Longitud de las direcciones del cliente y del relay.
    socklen_t client_len = sizeof(client_addr);
    socklen_t relay_len = sizeof(relay_addr);

    // Se define la estructura que van a tener los mensajes DHCP.
    struct dhcp_message discover_msg, request_msg;

    // Buffer para almacenar los mensajes.
    char buffer[BUFFER_SIZE];

    // Definimos la estructura para almacenar la longitud del mensaje recibido.
    ssize_t message_len;

    struct dhcp_message *offer_msg, *ack_msg;

    // ----

    // Se inicializa el socket que nos va a permitir hacer broadcast al puerto del relay.
    send_socket = initialize_DGRAM_socket(&client_addr, client_len);

    // Se obtiene la MAC del cliente asociada a la interface enp0s3.
    uint8_t *chaddr = get_mac_address(interface);
    
    // Se configura los parametros principales del mensaje DHCP.
    configure_dhcp_message(&discover_msg, 1, 1, 6,  htonl(0x12345678), htons(0x8000), chaddr);

    // Se define el index que nos va a permitir ubicar en que parte del options del mensaje DHCP vamos.
    int index = 0;
    
    // Se configura el mensaje DHCP para que sea un mensaje de discover.
    set_type_message(discover_msg.options, &index, 53, 1, 1);

    // Se establece el final de las opciones.
    discover_msg.options[index] = 255;

   
    // Se define la dirección IP del relay.
    relay_addr.sin_family = AF_INET;

    // Configuración del puerto del relay.
    relay_addr.sin_port = htons(DHCP_RELAY_PORT);

    // Se define que se le mandara el mensaje de broadcast al puerto del relay.
    relay_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Se envía el mensaje de broadcast al puerto del relay.
    if (sendto(send_socket, &discover_msg, sizeof(discover_msg), 0, (struct sockaddr*)&relay_addr, relay_len) < 0) {
        error("Error al enviar mensaje");
        close(send_socket);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_RELAY_PORT);

    // Crear el socket RAW socket para poder recibir mensajes broadcast sin tener la interfaz ip asignada.
    recv_socket = initialize_RAW_socket(&client_addr, client_len);

    printf("Esperando mensaje DHCP OFFER UDP en broadcast...\n");

    while (1) {

        // Se alamacena el mensaje recibido en el buffer.
        message_len = receive_message(recv_socket, buffer, &relay_addr, &relay_len);

        // Se procesa el mensaje recibido.
        offer_msg = process_msg(buffer);
        if (offer_msg != NULL) {
            printf("Mensaje UDP recibido\n");
            break;
        }
    }

    // Se verifica que el mensaje es un DHCP OFFER
    int dhcp_type = get_dhcp_message_type(offer_msg);

    if (dhcp_type == 2) {
        printf("Mensaje DHCPOFFER recibido correctamente\n");
        print_network_config(offer_msg);
    } else {
        printf("No se recibió un DHCPOFFER. Tipo de mensaje recibido: %d\n", dhcp_type);
    }

    memset(&request_msg, 0, sizeof(request_msg));

    configure_dhcp_message(&request_msg, 1, 1, 6, offer_msg->xid, htons(0x8000), chaddr);

    int request_index = 0;
    
    // Se configura el mensaje DHCP para que sea un mensaje de request.
    set_type_message(request_msg.options, &request_index, 53, 1, 3);

    // Se configura la IP solicitada.
    set_requested_ip(request_msg.options, &request_index, offer_msg->yiaddr);

    uint32_t server_ip = inet_addr(IP_SERVER_IDENTIFIER);

    // Se configura la dirección IP del servidor.
    set_server_identifier(request_msg.options, &request_index, server_ip);

    // Se establece el final de las opciones.
    request_msg.options[request_index] = 255;

    relay_addr.sin_family = AF_INET;

    // Configuración del puerto del relay.
    relay_addr.sin_port = htons(DHCP_RELAY_PORT);

    // Se define que se le mandara el mensaje de broadcast al puerto del relay.
    relay_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    if (sendto(send_socket, &request_msg, sizeof(request_msg), 0, (struct sockaddr*)&relay_addr, relay_len) < 0) {
        perror("Error al enviar mensaje");
        close(send_socket);
        exit(EXIT_FAILURE);
    }

    int send_type = get_dhcp_message_type(&request_msg);
    printf("El tipo de mensaje que se envio fue un: %d\n", send_type);
    printf("La IP solicitada fue: %s\n", inet_ntoa(*(struct in_addr *)&request_msg.options[5]));
    printf("La IP del servidor fue: %s\n", inet_ntoa(*(struct in_addr *)&request_msg.options[11]));

    printf("Mensaje de broadcast enviado al puerto %d desde la interfaz enp0s3\n",DHCP_RELAY_PORT);

    close(send_socket);

    printf("Esperando mensaje DHCP ACK UDP en broadcast...\n");

    // Recibir mensaje DHCP ACK
    while(1) {
        message_len = receive_message(recv_socket, buffer, &relay_addr, &relay_len);

        ack_msg = process_msg(buffer);
        if (ack_msg != NULL) {
            printf("Mensaje UDP recibido\n");
            break;
        }
    }

    int ack_type = get_dhcp_message_type(ack_msg);

    if (ack_type == 5) {
        printf("Mensaje DHCPACK recibido correctamente\n");
        print_network_config(ack_msg);
        struct in_addr ip_addr, subnet_mask, gateway;

        assign_ip_to_interface(interface, ack_msg);
    } else {
        printf("No se recibió un DHCPACK. Tipo de mensaje recibido: %d\n", ack_type);
    }

    close(recv_socket);


    return 0;
}
