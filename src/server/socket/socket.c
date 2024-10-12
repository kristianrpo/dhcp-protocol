#include "socket.h"

// Función para inicializar el socket.
int initialize_socket(struct sockaddr_in *server_addr, socklen_t server_len) {
    // Reservamos un espacio en memoria para almacenar el identificador del socket que se va a utilizar
    int fd;

    // Asignamos al espacio un socket de la familia IPv4, de tipo UDP, y que utilice el protocolo del tipo de socket. Almacenamos el identificador del socket creado.
    fd = socket(AF_INET,SOCK_DGRAM,0); 

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
        error("No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configuramos la estructura donde se va a almacenar la ip y el puerto.

    // Establecemos que el socket va a almacenar una IPV4.
    server_addr->sin_family = AF_INET;

    // Establecemos que el socket va a tener asociado el puerto 1067 para DHCP, usamos htons para tranformar este numero de decimal a bits de red.
    server_addr->sin_port = htons(DHCP_SERVER_PORT);

    // Configuramos la dirección IP donde el socket va a estar escuchando conexiones (todas las ip relacionadas a dicho puerto), usamos htonl para convertir al formato adecuado.
    server_addr->sin_addr.s_addr = inet_addr(IP_SERVER_IDENTIFIER); 

    // Enlazamos el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    // bind recibe 3 parámetros, el identificador del socket creado, la dirección en memoria de la estructura que contiene la ip y el puerto asociado para el socket que deseamos, y, el tamaño de la estructura que definimos para que se sepa cuanta memoria debe leer.
    if (bind(fd, (struct sockaddr *)server_addr, server_len) < 0) {
        perror("Error al enlazar el socket con la estructura de red definida");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mensaje de confirmación de que el socket fue configurado e inicializado correctamente.
    printf("Socket inicializado y escuchando en el puerto %d\n", DHCP_SERVER_PORT);
    return fd;
}

// Función para recibir un mensaje del socket
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *relay_addr, socklen_t *relay_len) {
    
    // Almacenamos el numero de bytes recibidos del datagrama/mensaje correspondiente.
    // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)relay_addr, relay_len);

    // Comprobamos si ocurrió un error durante la recepción del mensaje.
    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }
    return msg_len;
}