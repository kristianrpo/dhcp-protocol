#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <errno.h>

#define PORT 1069   // Puerto de destino en la Relay VM
#define BROADCAST_IP "192.168.56.255"  // Dirección de broadcast para la red
#define INTERFACE "enp0s3"  // Especificar la interfaz de red

// Estructura para imprimir información del paquete
void print_packet(unsigned char *buffer, int size) {
    struct iphdr *iph = (struct iphdr *)(buffer);
    struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct iphdr));

    printf("Paquete recibido:\n");
    printf("Encabezado IP:\n");
    printf(" - Versión: %d\n", iph->version);
    printf(" - Longitud del encabezado: %d\n", iph->ihl);
    printf(" - Tipo de servicio: %d\n", iph->tos);
    printf(" - Longitud total: %d\n", ntohs(iph->tot_len));
    printf(" - ID del paquete: %d\n", ntohs(iph->id));
    printf(" - TTL: %d\n", iph->ttl);
    printf(" - Protocolo: %d\n", iph->protocol);
    printf(" - Dirección de origen: %s\n", inet_ntoa(*(struct in_addr *)&iph->saddr));
    printf(" - Dirección de destino: %s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));

    if (iph->protocol == IPPROTO_UDP) {
        printf("Encabezado UDP:\n");
        printf(" - Puerto de origen: %d\n", ntohs(udph->source));
        printf(" - Puerto de destino: %d\n", ntohs(udph->dest));
        printf(" - Longitud: %d\n", ntohs(udph->len));
        printf("Mensaje UDP: %s\n", buffer + sizeof(struct iphdr) + sizeof(struct udphdr));
    } else {
        printf("No es un paquete UDP\n");
    }
}

int main() {
    int sockfd_send, sockfd_recv;
    struct sockaddr_in broadcast_addr, recv_addr;
    struct ifreq ifr;
    int broadcast_enable = 1;
    char message[] = "Mensaje desde cliente sin IP";
    unsigned char buffer[4096];  // Buffer para recibir paquetes
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    // Crear el socket para enviar (UDP)
    sockfd_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd_send < 0) {
        perror("Error creando el socket de envío UDP");
        exit(EXIT_FAILURE);
    }

    // Habilitar la opción de broadcast en el socket de envío
    if (setsockopt(sockfd_send, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error configurando broadcast en el socket de envío");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    // Asociar el socket de envío a la interfaz específica sin IP
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ - 1);
    if (setsockopt(sockfd_send, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("Error vinculando el socket de envío a la interfaz");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    // Preparar la dirección de destino (broadcast)
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);  // Puerto 1069
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviar el mensaje de broadcast
    if (sendto(sockfd_send, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("Error enviando el broadcast");
        close(sockfd_send);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado correctamente.\n");

    // Cerrar el socket de envío
    close(sockfd_send);

    // Crear el socket para recibir (UDP)
    sockfd_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd_recv < 0) {
        perror("Error creando el socket de recepción");
        exit(EXIT_FAILURE);
    }

    // Asociar el socket de recepción a la interfaz específica sin IP
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ - 1);
    if (setsockopt(sockfd_recv, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("Error vinculando el socket de recepción a la interfaz");
        close(sockfd_recv);
        exit(EXIT_FAILURE);
    }

    // Escuchar paquetes
    printf("Escuchando paquetes UDP en la interfaz %s\n", INTERFACE);

    // Bucle infinito para escuchar paquetes entrantes
    while (1) {
        int data_size = recvfrom(sockfd_recv, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (data_size < 0) {
            perror("Error recibiendo datos");
            close(sockfd_recv);
            exit(EXIT_FAILURE);
        }

        // Imprimir la información del paquete recibido
        print_packet(buffer, data_size);
    }

    // Cerrar el socket de recepción (nunca llegará aquí en este ejemplo)
    close(sockfd_recv);

    return 0;
}
