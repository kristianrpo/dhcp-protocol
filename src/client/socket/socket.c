#include "socket.h"

int initialize_DGRAM_socket(struct sockaddr_in *client_addr, socklen_t client_len){
    // Definimos la variable que va a almacenar el identificador socker.
    int fd;

    // Definimos variable para habilitar la opición de broadcast en el socket.
    int broadcast_enable = 1;

    // Asignamos a la variable el socket correspondiente IPv4 (familia de direcciones del socket), UDP (tipo del socket), y este utiliza el mismo protocolo del tipo del socket. Nos da el Id del socket que creamos.
    fd = socket(AF_INET,SOCK_DGRAM,0); 

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
         error("No se pudo crear el socket");
         exit(EXIT_FAILURE);
    }

    // Habilitamos la opción de broadcast en el socket
    int ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    if (ret < 0) {
        perror("Error habilitando la opción de broadcast");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Enlazamos el socket a la interfaz de red del cliente.
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "enp0s3"); 
    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("Error al enlazar a la interfaz");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Definimos al socket su puerto (1068) e ip (0.0.0.0).
    // Definimos la familia de direcciones del socket (IPv4).
    client_addr->sin_family = AF_INET;
    // Configuración del puerto del cliente.
    client_addr->sin_port = htons(DHCP_CLIENT_PORT);
    // Definimos la ip del cliente.
    client_addr->sin_addr.s_addr = inet_addr("0.0.0.0");

    // Enlazamos el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    if (bind(fd, (struct sockaddr *)client_addr, client_len) < 0) {
        error("Error al enlazar el socket con la estructura de red definida");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

// Función para inicializar el socket RAW.
int initialize_RAW_socket(struct sockaddr_in *client_addr, socklen_t client_len){
    // Se inicializa el socket RAW para poder recibir mensajes broadcast sin tener la interfaz ip asignada.
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (fd < 0) {
        error("Error al crear el socket");
        exit(EXIT_FAILURE);
    }
    return fd;
}

// Función para recibir un mensaje del socket.
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *relay_addr, socklen_t *relay_len) {
    printf("Esperando a recibir mensaje en el socket: %d\n", fd);

    // Limpiamos el buffer antes de recibir datos para evitar datos antiguos.
    memset(buffer, 0, BUFFER_SIZE);
    
    // Realizamos la llamada recvfrom para recibir mensajes.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)relay_addr, relay_len);

    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }

    return msg_len;
}
