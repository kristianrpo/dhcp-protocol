#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define DHCP_SERVER_PORT 1067  // Puerto en el que el servidor está escuchando
#define DHCP_CLIENT_PORT 1068  // Puerto del cliente DHCP
#define BUFFER_SIZE 576        // Tamaño máximo del mensaje DHCP

// Estructura del mensaje DHCP
struct dhcp_message {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
};

// Función para mostrar errores y salir
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Función para obtener el tipo de mensaje DHCP (de las opciones)
int get_dhcp_message_type(struct dhcp_message *msg) {
    int i = 0;
    while (i < 312) {
        if (msg->options[i] == 53) {  // Opción 53 es el tipo de mensaje DHCP
            return msg->options[i + 2];  // El valor del tipo de mensaje está en el tercer byte
        }
        // Avanzar en las opciones (código de opción + longitud de la opción + valor)
        i += msg->options[i + 1] + 2;
    }
    return -1;  // No se encontró la opción 53
}

void print_network_config(struct dhcp_message *offer_msg) {
    int i = 0;
    struct in_addr addr;
    
    while (i < 312) {
        uint8_t option = offer_msg->options[i];
        uint8_t len = offer_msg->options[i + 1];

        if (option == 1) {  // Máscara de red
            memcpy(&addr, &offer_msg->options[i + 2], 4);
            printf("Máscara de red: %s\n", inet_ntoa(addr));
        } else if (option == 3) {  // Gateway predeterminado
            memcpy(&addr, &offer_msg->options[i + 2], 4);
            printf("Gateway: %s\n", inet_ntoa(addr));
        } else if (option == 6) {  // Servidor DNS
            memcpy(&addr, &offer_msg->options[i + 2], 4);
            printf("Servidor DNS: %s\n", inet_ntoa(addr));
        }

        // Fin de las opciones (255)
        if (option == 255) {
            break;
        }

        // Avanzar al siguiente campo de opciones
        i += len + 2;
    }
}

int main() {
    struct sockaddr_in client_addr, server_addr;
    int sockfd;
    ssize_t sent_len;
    struct dhcp_message discover_msg, request_msg;
    struct dhcp_message *offer_msg;  // Para almacenar el mensaje recibido
    char buffer[BUFFER_SIZE];

    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("Error al crear el socket");
    }

    // Configurar la dirección del cliente (local)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(DHCP_CLIENT_PORT);  // Puerto del cliente
    client_addr.sin_addr.s_addr = INADDR_ANY;        // Cualquier dirección

    // Enlazar el socket a la dirección local
    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        error("Error en bind");
    }

    // Configurar la dirección del servidor (remoto)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DHCP_SERVER_PORT);  // Puerto del servidor
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Dirección IP del servidor

    // Construir mensaje DHCPDISCOVER
    memset(&discover_msg, 0, sizeof(discover_msg));
    discover_msg.op = 1;  // Mensaje de solicitud (cliente a servidor)
    discover_msg.htype = 1;  // Tipo de hardware (Ethernet)
    discover_msg.hlen = 6;  // Longitud de dirección MAC (6 bytes)
    discover_msg.xid = htonl(0x12345678);  // ID de transacción
    discover_msg.flags = htons(0x8000);  // Broadcast
    discover_msg.chaddr[0] = 0x00;
    discover_msg.chaddr[1] = 0x11;
    discover_msg.chaddr[2] = 0x22;
    discover_msg.chaddr[3] = 0x33;
    discover_msg.chaddr[4] = 0x44;
    discover_msg.chaddr[5] = 0x55;

    // Configurar opciones del mensaje DHCP
    discover_msg.options[0] = 53;  // Opción para tipo de mensaje DHCP
    discover_msg.options[1] = 1;   // Longitud de la opción
    discover_msg.options[2] = 1;   // DHCPDISCOVER
    discover_msg.options[3] = 255; // Fin de opciones (opción 255)

    // Enviar mensaje DHCPDISCOVER al servidor
    sent_len = sendto(sockfd, &discover_msg, sizeof(discover_msg), 0, 
                      (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent_len < 0) {
        error("Error al enviar DHCPDISCOVER");
    }

    printf("Mensaje DHCPDISCOVER enviado\n");

    // Esperar respuesta del servidor (DHCPOFFER)
    socklen_t server_len = sizeof(server_addr);
    ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                                (struct sockaddr *)&server_addr, &server_len);
    if (recv_len > 0) {
        printf("Respuesta recibida del servidor DHCP\n");

        // Convertir el buffer recibido en un mensaje DHCP
        offer_msg = (struct dhcp_message *)buffer;

        // Verificar que el mensaje es un DHCPOFFER
        int dhcp_type = get_dhcp_message_type(offer_msg);
        if (dhcp_type == 2) {  // 2 es el valor de DHCPOFFER
            printf("Mensaje DHCPOFFER recibido correctamente\n");
            printf("IP ofrecida: %s\n", inet_ntoa(*(struct in_addr *)&offer_msg->yiaddr));

            print_network_config(offer_msg);

            // Construir mensaje DHCPREQUEST
            memset(&request_msg, 0, sizeof(request_msg));
            request_msg.op = 1;  // Solicitud (cliente a servidor)
            request_msg.htype = 1;  // Tipo de hardware (Ethernet)
            request_msg.hlen = 6;  // Longitud de dirección MAC (6 bytes)
            request_msg.xid = offer_msg->xid;  // Mismo ID de transacción
            request_msg.flags = htons(0x8000);  // Broadcast
            memcpy(request_msg.chaddr, discover_msg.chaddr, 6);  // Misma dirección MAC

            // Configurar opciones del mensaje DHCPREQUEST
            request_msg.options[0] = 53;  // Opción para tipo de mensaje DHCP
            request_msg.options[1] = 1;   // Longitud de la opción
            request_msg.options[2] = 3;   // DHCPREQUEST

            // Opción para IP solicitada (opción 50)
            request_msg.options[3] = 50;
            request_msg.options[4] = 4;  // Longitud de la opción
            memcpy(&request_msg.options[5], &offer_msg->yiaddr, 4);  // IP ofrecida

            // Fin de opciones
            request_msg.options[9] = 255;

            sent_len = sendto(sockfd, &request_msg, sizeof(request_msg), 0, 
                      (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (sent_len < 0) {
                error("Error al enviar DHCPREQUEST");
            }

            printf("Mensaje DHCPREQUEST enviado\n");
        } else {
            printf("No se recibió un DHCPOFFER. Tipo de mensaje recibido: %d\n", dhcp_type);
        }
    } else if (recv_len < 0) {
        error("Error al recibir respuesta");
    }

    // Cerrar el socket
    close(sockfd);

    return 0;
}
