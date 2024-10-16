#include "socket.h"


// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *relay_addr, socklen_t relay_len){
    // Definición de variable que va a almacenar el socker.
    int fd;
    int broadcast_enable = 1;

    // Asignación a la variable el socket correspondiente IPv4 (familia de direcciones del socket), UDP (tipo del socket), y este utiliza el mismo protocolo del tipo del socket. Nos da el Id del socket que creamos.
    fd = socket(AF_INET,SOCK_DGRAM,0); 

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
         error("No se pudo crear el socket");
         exit(EXIT_FAILURE);
    }

    // Habilitar la opción de broadcast en el socket
    int ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    if (ret < 0) {
        error("Error habilitando la opción de broadcast");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Configuración de la estructura donde se va a almacenar la ip y el puerto.
    // Configuración de que el socket va a almacenar una IPV4.
    relay_addr->sin_family = AF_INET;

    // Configuración de que el socket va a tener asociado el puerto 1067 para DHCP. htons tranforma este numero en decimal a bits de red.
    relay_addr->sin_port = htons(DHCP_RELAY_PORT);

    // Configuración de la dirección IP donde el socket va a estar escuchando conexiones (el servidor para todas sus ips relacionadas con el puerto especificado va a estar escuchando). htonl tambien convierte a formato adecuado.
    relay_addr->sin_addr.s_addr = htonl(INADDR_ANY); 


    // Ahora, se va a enlazar el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    // bind recibe 3 parametros, el identificador del socket creado, la dirección en memoria de la estructura que contiene la ip y el puerto asociado para el socket que deseamos, y, por el utlimo, el tamaño de la estructura que definimos para que se sepa cuanta memoria se debe leer.
    if (bind(fd, (struct sockaddr *)relay_addr, relay_len) < 0) {
        error("Error al enlazar el socket con la estructura de red definida");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mensaje de confirmación de que el socket fue configurado e inicializado correctamente.
    printf("Socket inicializado y escuchando en el puerto %d\n", DHCP_SERVER_PORT);
    return fd;
}

// Función para recibir un mensaje del socket
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *actor_addr, socklen_t *actor_len) {
    
    // Se almacena el numero de bytes recibidos del datagrama/mensaje correspondiente.
    // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer. Además configura la estructura del cliente con la información del cliente que envió el mensaje.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)actor_addr, actor_len);

    // Comprueba si ocurrió un error durante la recepción del mensaje.
    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }
    return msg_len;
}

// Función para enviar un mensaje a través del socket.
int send_message(int fd, const char *buffer, int message_len, struct sockaddr_in *actor_addr, socklen_t actor_len, int is_client) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    if(is_client){
        // Enlazamos el socket a la interfaz de red relacionada con el cliente.
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), CLIENT_ASSOCIATED_INTERFACE); 
        if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
            perror("Error al enlazar a la interfaz");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    else{
        // Enlazamos el socket a la interfaz de red relacionada con el servidor.
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), SERVER_ASSOCIATED_INTERFACE); 
        if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
            perror("Error al enlazar a la interfaz");
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    if (sendto(fd, buffer, message_len, 0, (struct sockaddr *)actor_addr, actor_len) < 0) {
        error("Error al reenviar DHCPOFFER al cliente");
        return -1;
    }
    return 0;
}

