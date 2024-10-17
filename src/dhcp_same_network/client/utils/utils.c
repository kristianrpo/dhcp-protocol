#include "utils.h"

// Función para verificar si el paquete es IP.
int is_ip_packet(struct ethhdr *eth) {
    return ntohs(eth->h_proto) == ETH_P_IP;
}

// Función para verificar si el paquete es UDP.
int is_udp_packet(struct iphdr *ip_header) {
    return ip_header->protocol == IPPROTO_UDP;
}

// Función para obtener la cabecera Ethernet.
struct ethhdr *get_eth_header(char *buffer) {
    return (struct ethhdr *)buffer;
}

// Función para obtener la cabecera IP.
struct iphdr *get_ip_header(char *buffer) {
    return (struct iphdr *)(buffer + sizeof(struct ethhdr));
}

// Función para obtener la cabecera UDP.
struct udphdr *get_udp_header(char *buffer, struct iphdr *ip_header) {
    return (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_header->ihl * 4);
}

// Función para obtener el payload (datos) del paquete UDP.
char *get_udp_payload(char *buffer, struct iphdr *ip_header, struct udphdr *udp_header) {
    return (char *)(buffer + sizeof(struct ethhdr) + ip_header->ihl * 4 + sizeof(struct udphdr));
}

// Función para procesar el paquete y extraer el payload UDP si es un paquete para el puerto 1069.
struct dhcp_message* process_msg(char *buffer) {
    // Posicionamos el puntero al inicio del buffer que representa el encabezado Ethernet y lo mapeamos a una estructura ethernet.
    struct ethhdr *eth = get_eth_header(buffer);

    // Verificamos si es un paquete IP.
    if (is_ip_packet(eth)) {
        // Posicionamos el puntero en el encabezado IP y lo mapeamos a una estructura IP.
        struct iphdr *ip_header = get_ip_header(buffer);

        // Verificamos si es un paquete UDP.
        if (is_udp_packet(ip_header)) {

            // Posicionamos el puntero en el encabezado UDP y lo mapeamos a una estructura UDP.
            struct udphdr *udp_header = get_udp_header(buffer, ip_header);

            // Obtenemos los puertos de origen y destino del paquete UDP.
            unsigned short dest_port = ntohs(udp_header->dest);
            unsigned short src_port = ntohs(udp_header->source);

            // Verificamos si el paquete es un mensaje DHCP.
            if (dest_port == 1068 && src_port == 1067) {
                printf("Mensaje UDP broadcast recibido en el puerto 1068\n");

                // Obtenemos los datos del mensaje (payload).
                char *data = get_udp_payload(buffer, ip_header, udp_header);

                // Mapeamos el payload como una estructura de mensaje DHCP.
                struct dhcp_message *msg = (struct dhcp_message *)data;
                return msg;
            }
        }
    }
    // Si el mensaje recibido la interfaz no es un mensaje DHCP, retornamos NULL.
    return NULL;
}

// Función para imprimir la configuración de red.
void print_network_config(struct dhcp_message *msg) {
    int i = 0;
    struct in_addr addr;   
    struct in_addr ip_addr;
    ip_addr.s_addr = msg->yiaddr; 
    printf("Dirección IP: %s\n", inet_ntoa(ip_addr));  
    while (i < 312) {
        uint8_t option = msg->options[i];
        uint8_t len = msg->options[i + 1];

        if (option == 1) {  // Opción máscara de red.
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Máscara de red: %s\n", inet_ntoa(addr));
        } else if (option == 3) {  // Opción gateway predeterminado.
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Gateway: %s\n", inet_ntoa(addr));
        } else if (option == 6) {  // Opción servidor DNS.
            memcpy(&addr, &msg->options[i + 2], 4);
            printf("Servidor DNS: %s\n", inet_ntoa(addr));
        }

        // Verificar si se llegó al final de las opciones
        if (option == 255) {
            break;
        }

        i += len + 2;
    }
}

// Función para obtener la dirección MAC y retornarla como un puntero.
uint8_t *get_mac_address(const char *interface) {

    // Definición de variable que va a almacenar el ID del socket.
    int client_socket;

    // Definición de la estructura ifreq.
    struct ifreq ifr;

    // Definición de la dirección MAC como un arreglo de 6 bytes.
    static uint8_t mac[6];

    // Creamos un socket para la solicitud con el fin de obtener la MAC.
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Error al crear socket");
        return NULL;
    }

    // Copiamos el nombre de la interfaz a la estructura ifreq.
    strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

    // Hacemos la solicitud ioctl para obtener la dirección MAC.
    if (ioctl(client_socket, SIOCGIFHWADDR, &ifr) == -1) {
        perror("Error al obtener dirección MAC");
        close(client_socket);
        return NULL;
    }

    // Copiamos la dirección MAC obtenida a la variable estática.
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

    close(client_socket);
    return mac;  // Retornar el puntero a la dirección MAC.
}

// Funciones para obtener el identificador del servidor.
uint32_t get_server_identifier(struct dhcp_message *msg) {
    // Definimos la variable de control para recorrer el campo de opciones.
    int i = 0;

    // Definimos la variable que va a almacenar la dirección IP del servidor.
    uint32_t server_ip = 0;

    // Recorremos las opciones del mensaje DHCP para encontrar la dirección IP del servidor.
    while (i < 312) {
        uint8_t option = msg->options[i];
        uint8_t len = msg->options[i + 1];

        if (option == 54) {
            memcpy(&server_ip, &msg->options[i + 2], 4);
            break;
        }

        if (option == 255) {
            break;
        }

        i += len + 2;
    }

    // Retornamos la dirección IP del servidor.
    return server_ip;
}

// Función para asignar una dirección IP a una interfaz.
void assign_ip_to_interface(const char *interface, struct dhcp_message *msg) { 

    // Definimos la variable que va a almacenar el comando a ejecutar.
    char command[256];

    // Definimos la estructura que va a almacenar la dirección IP.
    struct in_addr ip_addr;

    // Definimos el prefijo de la máscara de red.
    char subnet_prefix[4] = "24";

    // Extraemos la IP asignada desde el campo yiaddr del mensaje DHCPACK.
    ip_addr.s_addr = msg->yiaddr;

    // Asignamos la nueva dirección IP con el prefijo quemado.
    snprintf(command, sizeof(command), "sudo ip addr add %s/%s dev %s", inet_ntoa(ip_addr), subnet_prefix, interface);
    system(command);

    // Activamos la interfaz.
    snprintf(command, sizeof(command), "sudo ip link set dev %s up", interface);
    system(command);
}

