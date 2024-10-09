#include "socket.h"

int initialize_socket(struct sockaddr_in *relay_addr, socklen_t relay_len){
    // Definición de variable que va a almacenar el socker.
    int fd;

    // Asignación a la variable el socket correspondiente IPv4 (familia de direcciones del socket), UDP (tipo del socket), y este utiliza el mismo protocolo del tipo del socket. Nos da el Id del socket que creamos.
    fd = socket(AF_INET,SOCK_DGRAM,0); 

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
         error("No se pudo crear el socket");
         exit(EXIT_FAILURE);
    }

    // Configuración de la estructura donde se va a almacenar la ip y el puerto.
    // Configuración de que el socket va a almacenar una IPV4.
    relay_addr->sin_family = AF_INET;

    // Configuración de que el socket va a tener asociado el puerto 1067 para DHCP. htons tranforma este numero en decimal a bits de red.
    relay_addr->sin_port = htons(DHCP_SERVER_PORT);

    // Configuración de la dirección IP donde el socket va a estar escuchando conexiones (el servidor para todas sus ips relacionadas con el puerto especificado va a estar escuchando). htonl tambien convierte a formato adecuado.
    relay_addr->sin_addr.s_addr = htonl(INADDR_ANY); 


    // Ahora, se va a enlazar el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    // bind recibe 3 parametros, el identificador del socket creado, la dirección en memoria de la estructura que contiene la ip y el puerto asociado para el socket que deseamos, y, por el utlimo, el tamaño de la estructura que definimos para que se sepa cuanta memoria se debe leer.
    if (bind(fd, (struct sockaddr *)&relay_addr, relay_len) < 0) {
        error("Error al enlazar el socket con la estructura de red definida");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mensaje de confirmación de que el socket fue configurado e inicializado correctamente.
    printf("Socket inicializado y escuchando en el puerto %d\n", DHCP_SERVER_PORT);
    return fd;
}

// Función para recibir un mensaje del socket
ssize_t receive_message_client(int fd, char *buffer, struct sockaddr_in *client_addr, socklen_t *client_len) {
    
    // Se almacena el numero de bytes recibidos del datagrama/mensaje correspondiente.
    // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer. Además configura la estructura del cliente con la información del cliente que envió el mensaje.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, client_len);

    // Comprueba si ocurrió un error durante la recepción del mensaje.
    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }
    return msg_len;
}

// Función para recibir un mensaje del socket
ssize_t receive_message_server(int fd, char *buffer, struct sockaddr_in *server_addr, socklen_t *server_len) {
    
    // Se almacena el numero de bytes recibidos del datagrama/mensaje correspondiente.
    // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer. Además configura la estructura del cliente con la información del cliente que envió el mensaje.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, server_len);

    // Comprueba si ocurrió un error durante la recepción del mensaje.
    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }
    return msg_len;
}