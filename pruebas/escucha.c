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

#define INTERFACE "enp0s3"  // Especificar la interfaz de red

// Estructura para imprimir información del paquete
void print_packet(unsigned char *buffer, int size) {
    struct iphdr *iph = (struct iphdr *)(buffer);
    struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct iphdr));

    printf("Paquete recibido:\n");
    printf("Encabezado IP:\n");
    printf(" - Version: %d\n", iph->version);
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
    int sockfd;
    struct ifreq ifr;
    unsigned char buffer[4096];  // Buffer para recibir paquetes
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    // Crear el socket RAW para recibir paquetes UDP
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Error creando el socket RAW");
        exit(EXIT_FAILURE);
    }

    // Asociar el socket a la interfaz específica sin IP
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ - 1);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("Error vinculando el socket a la interfaz");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Escuchando paquetes UDP en la interfaz %s\n", INTERFACE);

    // Bucle infinito para escuchar paquetes entrantes
    while (1) {
        int data_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, &saddr, &saddr_len);
        if (data_size < 0) {
            perror("Error recibiendo datos");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        // Imprimir la información del paquete recibido
        print_packet(buffer, data_size);
    }

    // Cerrar el socket
    close(sockfd);
    return 0;
}
