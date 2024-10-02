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
    uint8_t op;             // Define el tipo de operacion del paquete, =1 si es una solicitud proveniente de un cliente, =2 si es una respuesta proveniente del servidor.      
    uint8_t htype;          // Define el tipo de hardware que se está utilizando, =1 si es Ethernet (el mas comun). 
    uint8_t hlen;           // Indica el tamaño de la dirección MAC del cliente, =6 longitud de la dirección MAC en Ethernet en bytes.
    uint8_t hops;           // Indica el número de saltos que ha dado el paquete, es decir la cantidad de veces que ha sido reenviado por un agente de retransmisión en camino al servidor.
    uint32_t xid;           // Identificador de transacción, es un número aleatorio que se genera para identificar la transacción entre el cliente y el servidor.
    uint16_t secs;          // Indica la cantidad de segundos que han pasado desde que el cliente comenzó a buscar una dirección IP.
    uint16_t flags;         // Si está activo, indica que el cliente está esperando la respuesta por broadcast porque no tiene una dirección IP asignada.
    uint32_t ciaddr;        // La dirección IP del cliente, si la tiene o 0.0.0.0 si aún no la tiene.
    uint32_t yiaddr;        // La dirección IP que el servidor le ofrece al cliente.
    uint32_t siaddr;        // La dirección IP del servidor que está ofreciendo la dirección IP al cliente.
    uint32_t giaddr;        // La dirección IP del agente de retransmisión que reenvió el paquete si el cliente y el servidor están en redes diferentes.
    uint8_t chaddr[16];     // Dirección MAC del cliente, generalmente de 6 bytes en Ethernet pero definida con 16 para posibles extensiones.
    uint8_t sname[64];      // Nombre del servidor, generalmente no se usa.
    uint8_t file[128];      // Nombre del archivo de arranque que el cliente deberia usar al iniciar, se usa solo cuando los dispositivos necesitan un arranqye remoto o boot desde una imagen de red.
    uint8_t options[312];   // Opciones que personalizan en comportamiento del protocolo, se incluye información como el tipo de mensaje (opción 53), identificador del servidor (opción 54), parametros de red solicitados por el cliente (opción 55), etc.
};


// Metodo para retornar el tipo de error a la hora de crear un socket.
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

// Metodo para obtener el mensaje DHCP para conocer la opción a realizar.
int get_dhcp_message_type(struct dhcp_message *msg) {
    // Definición de la variable de control para recorrer el campo de options.
    int i = 0;
    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 53, que es la que contiene el mensaje para la acción a realizar. Se recorre hasta 312 porque el campo options como maximo puede medir 312 bytes según estandarización.
    while (i < 312) {
        // Si encontramos el fin de las opciones (0xFF), salimos del ciclo.
        if (msg->options[i] == 255) {
            break;
        }
        
        // Revisar si estamos en la opción 53 (Tipo de mensaje DHCP).
        if (msg->options[i] == 53) {
            // Aseguramos que la longitud del mensaje DHCP denota que el mismo existe.
            if (msg->options[i + 1] == 1) {
                return msg->options[i + 2];  // El valor del tipo de mensaje está en el tercer byte.
            } else {
                return -1;  // Si la longitud del mensaje DHCP no es valida, se retorna 'error'.
            }
        }
        
        // Avanzamos al siguiente campo de opciones, se avanza de esta manera puesto que cada opcion mide diferente, asi que se obtiene la siguiente opción de manera 'dinamica'.
        i += msg->options[i + 1] + 2;  // Saltamos el código de opción, la longitud, y el valor de la opción.
    }
    
    return -1;  // Si no se encuentra la opción 53.
}


int main(){
    struct sockaddr_in server_addr; // Definimos la estructura que va a almacenar la ip y el puerto del servidor DHCP (ESTA ESTRUCTURA ESTA HECHA PARA ALMACENAR DIRECCIONES DE RED EN IPV4 (sockaddr_in)) y le damos de nombre server_addr.
    struct sockaddr_in client_addr;  // Se define la estructura para almacenar la información del cliente, ya que el servidor necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del cliente.
    socklen_t server_len = sizeof(server_addr); // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t client_len = sizeof(client_addr); // Tamaño de la dirección del clinete (para saber cuantos bytes se debe leer o escribir en la estructura).
    char buffer[BUFFER_SIZE]; // Definimos un buffer para almacenar los datos recibidos de manera temporal, para así posteriormente procesarlos. 
    int fd; // Definición de variable que va a almacenar el socker.
    int message_type;
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

        // El mensaje que llega al servidor DHCP es una secuencia de bits, pudiendose considerar que la información está encapsulada, C como tal no es capaz de decodificar esta estructura para acceder a la información, por lo tanto, debemos definir una estructura que permita convertir esos simples bienarios en información accesible y operable para el servidor
        // Convertir el buffer a un mensaje DHCP. Se utiliza el casting de C para tratar bits crudos como una estructura
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        // Se obtiene el tipo de mensaje que se mandó del cliente para saber la acción a realizar.
        message_type = get_dhcp_message_type(msg);

        int message_type = get_dhcp_message_type(msg);
        if (message_type == 1) {
            printf("Mensaje DHCPDISCOVER recibido\n");
        }

    }

    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos
    close(fd);
    return 0;
}