#include <stdio.h>      
#include <stdlib.h>     
#include <errno.h>      
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <unistd.h>  

#define DHCP_SERVER_PORT 1067
#define BUFFER_SIZE 576

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


// Metodo para retornar el tipo de error a la hora de crear un socket
void error(const char *msg) {
    switch (errno) {
        case EACCES:
            fprintf(stderr, "Error: %s. Permiso denegado.\n", msg);
            break;
        case EPROTONOSUPPORT:
            fprintf(stderr, "Error: %s. Protocolo no soportado.\n", msg);
            break;
        default:
            fprintf(stderr, "Error: %s. Código de error desconocido: %d.\n", msg, errno);
            break;
    }
    exit(EXIT_FAILURE);  
}

int main(){
    struct sockaddr_in server_addr; // Definimos la estructura que va a almacenar la ip y el puerto del servidor DHCP (ESTA ESTRUCTURA ESTA HECHA PARA ALMACENAR DIRECCIONES DE RED EN IPV4 (sockaddr_in)) y le damos de nombre server_addr.
    struct sockaddr_in client_addr;  // Se define la estructura para almacenar la información del cliente, ya que el servidor necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del cliente.
    socklen_t server_len = sizeof(server_addr); // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t client_len = sizeof(client_addr); // Tamaño de la dirección del clinete (para saber cuantos bytes se debe leer o escribir en la estructura).
    char buffer[BUFFER_SIZE]; // Definimos un buffer para almacenar los datos recibidos de manera temporal, para así posteriormente procesarlos. 
    int fd; // Definición de variable que va a almacenar el socker.
    fd = socket(AF_INET,SOCK_DGRAM,0); // Asignación a la variable el socket correspondiente IPv4 (familia de direcciones del socket), UDP (tipo del socket), y este utiliza el mismo protocolo del tipo del socket. Nos da el Id del socket que creamos.

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
        error("No se pudo crear el socket");
    }

    // Configuración de la estructura donde se va a almacenar la ip y el puerto.
    server_addr.sin_family = AF_INET; // Configuración de que el socket va a almacenar una IPV4.
    server_addr.sin_port = htons(DHCP_SERVER_PORT); // Configuración de que el socket va a tener asociado el puerto 1067 para DHCP. htons tranforma este numero en decimal a bits de red.
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Configuración de la dirección IP donde el socket va a estar escuchando conexiones (el servidor para todas sus ips relacionadas con el puerto especificado va a estar escuchando). htonl tambien convierte a formato adecuado.


    // Ahora, se va a enlazar el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    // bind recibe 3 parametros, el identificador del socket creado, la dirección en memoria de la estructura que contiene la ip y el puerto asociado para el socket que deseamos, y, por el utlimo, el tamaño de la estructura que definimos para que se sepa cuanta memoria se debe leer.
    if (bind(fd, (struct sockaddr *)&server_addr, server_len) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    // Al utilizar UDP, el servidor puede recibir mensajes en cualquier momento a través del puerto especificado, está siempre listo para recibir mensajes, no como TCP, que se debe establecer una conexión con el cliente y no siempre está escuchando
    // Esperar y recibir datagrama (usamos recvfrom porque estamos en UDP). Este datagrama contiene el mensaje e información relacionada para poder llevar a cabo una acción
    printf("Esperando mensajes de clientes DHCP...\n");
    // Este bucle hace que el servidor DHCP este constantemente esperando recibir mensajes
    while (1) {

        // Se almacena el numero de bytes recibidos del datagrama/mensaje correspondiente
        // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer.
        ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);

        // Comprueba si ocurrió un error durante la recepción del mensaje.
        if (msg_len < 0) {
            error("Error al recibir mensaje");
        }

        printf("Mensaje recibido de cliente: %s\n", buffer);

        // El mensaje que llega al servidor DHCP es una secuencia de bits, pudiendose considerar que la información está encapsulada, c como tal no es capaz de decodificar esta estructura para acceder a la información, por lo tanto, debemos definir una estructura que permita convertir esos simples bienarios en información accesible y operable para el servidor
        // Convertir el buffer a un mensaje DHCP
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        
    }
    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos
    close(fd);
    return 0;
}