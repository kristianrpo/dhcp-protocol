#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>  // Para la cabecera IP
#include <netinet/udp.h> // Para la cabecera UDP
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> // Protocolo de Ethernet
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 65536

// Función para crear un socket RAW
int create_raw_socket() {
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sock < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    return sock;
}

// Función para recibir un paquete
ssize_t receive_packet(int socket_fd, char *buffer, size_t buffer_size) {
    ssize_t data_size = recvfrom(socket_fd, buffer, buffer_size, 0, NULL, NULL);
    if (data_size < 0) {
        perror("Error al recibir el paquete");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    return data_size;
}

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
char *get_udp_payload(char *buffer, struct iphdr *ip_header, struct udphdr *udp_header, ssize_t data_size, int *data_len) {
    *data_len = data_size - (sizeof(struct ethhdr) + ip_header->ihl * 4 + sizeof(struct udphdr));
    return (char *)(buffer + sizeof(struct ethhdr) + ip_header->ihl * 4 + sizeof(struct udphdr));
}

// Función para procesar el paquete y extraer el payload UDP si es un paquete para el puerto 1069
char* process_packet(char *buffer, ssize_t data_size) {
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    // Verificar si es un paquete IP
    if (is_ip_packet(eth)) {
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        // Verificar si es un paquete UDP
        if (is_udp_packet(ip_header)) {
            struct udphdr *udp_header = get_udp_header(buffer, ip_header);

            // Verificar si el puerto destino es el 1069
            unsigned short dest_port = ntohs(udp_header->dest);
            if (dest_port == 1069) {
                printf("Mensaje UDP broadcast recibido en el puerto 1069\n");

                // Obtener los datos del mensaje (payload)
                int data_len;
                char *data = get_udp_payload(buffer, ip_header, udp_header, data_size, &data_len);
                return data;
            }
        }
    }
}

int main() {
    char buffer[BUF_SIZE];

    // Crear el socket
    int client_socket = create_raw_socket();

    printf("Esperando mensaje DHCP OFFER UDP en broadcast...\n");

    while (1) {
        // Recibir un paquete
        ssize_t data_size = receive_packet(client_socket, buffer, BUF_SIZE);

        // Procesar el paquete recibido
        char *msg = process_packet(buffer, data_size);
        if (msg != NULL) {
            printf("Mensaje UDP: %s\n", msg);
            break;
        }
    }



    // Cerrar el socket
    close(client_socket);

    return 0;
}
