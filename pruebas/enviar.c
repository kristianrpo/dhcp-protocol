#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <errno.h>

#define PORT 1069   // Puerto de destino en la Relay VM
#define BROADCAST_IP "192.168.56.255"  // Dirección de broadcast para la red
#define INTERFACE "enp0s3"  // Especificar la interfaz de red

// Suma de verificación para el encabezado IP y UDP
unsigned short checksum(void *b, int len) {    
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

unsigned short udp_checksum(void *b, int len, struct in_addr *src_addr, struct in_addr *dest_addr) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    // Pseudo-encabezado
    sum += (src_addr->s_addr >> 16) & 0xFFFF;
    sum += (src_addr->s_addr) & 0xFFFF;
    sum += (dest_addr->s_addr >> 16) & 0xFFFF;
    sum += (dest_addr->s_addr) & 0xFFFF;
    sum += htons(IPPROTO_UDP);
    sum += htons(len);

    for (; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main() {
    int sockfd;
    char packet[4096];  // Paquete para almacenar el encabezado IP + UDP + datos
    struct iphdr *iph = (struct iphdr *) packet;
    struct udphdr *udph = (struct udphdr *) (packet + sizeof(struct iphdr));
    struct sockaddr_in broadcast_addr;
    struct ifreq ifr;
    int broadcast_enable = 1;
    char message[] = "Mensaje desde cliente sin IP";
    int datalen = strlen(message);

    // Crear el socket RAW
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("Error creando el socket RAW");
        exit(EXIT_FAILURE);
    }

    // Habilitar la opción de broadcast en el socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error configurando broadcast");
        close(sockfd);
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

    // Preparar la dirección de destino (broadcast)
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Limpiar el paquete antes de usarlo
    memset(packet, 0, sizeof(packet));

    // Construir el encabezado IP
    iph->ihl = 5;  // Longitud del encabezado IP (5 palabras de 32 bits)
    iph->version = 4;  // IPv4
    iph->tos = 0;  // Tipo de servicio
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + datalen);  // Longitud total del paquete
    iph->id = htonl(54321);  // ID del paquete
    iph->frag_off = 0;  // Sin fragmentación
    iph->ttl = 64;  // Tiempo de vida
    iph->protocol = IPPROTO_UDP;  // Protocolo UDP
    iph->check = 0;  // Inicialmente 0 antes de calcular la suma de verificación
    iph->saddr = inet_addr("0.0.0.0");  // Dirección de origen (sin IP)
    iph->daddr = broadcast_addr.sin_addr.s_addr;  // Dirección de destino (broadcast)

    // Calcular la suma de verificación del encabezado IP
    iph->check = checksum((unsigned short *) packet, sizeof(struct iphdr));

    // Construir el encabezado UDP
    udph->source = htons(12345);  // Puerto de origen arbitrario
    udph->dest = htons(PORT);  // Puerto de destino
    udph->len = htons(sizeof(struct udphdr) + datalen);  // Longitud del encabezado UDP + datos
    udph->check = 0;  // Inicialmente 0 antes de calcular la suma de verificación

    // Copiar el mensaje en el paquete
    memcpy(packet + sizeof(struct iphdr) + sizeof(struct udphdr), message, datalen);

    // Calcular la suma de verificación del UDP incluyendo el pseudo-encabezado
    udph->check = udp_checksum((unsigned short *)udph, sizeof(struct udphdr) + datalen, (struct in_addr *)&iph->saddr, (struct in_addr *)&iph->daddr);

    // Enviar el paquete al broadcast
    if (sendto(sockfd, packet, ntohs(iph->tot_len), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("Error enviando el broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje de broadcast enviado correctamente.\n");

    // Cerrar el socket
    close(sockfd);

    return 0;
}
