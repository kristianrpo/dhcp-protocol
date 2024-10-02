#include <string.h> 
#include <stdio.h>      
#include <stdlib.h>     
#include <errno.h>      
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <unistd.h>  
#include <time.h>   

#define DHCP_SERVER_PORT 1067
#define BUFFER_SIZE 576

#define MAX_LEASES 50
#define START_IP "192.168.0.21"
#define END_IP "192.168.0.71"

struct lease_entry {
    uint8_t mac_addr[6];  // Dirección MAC del cliente
    uint32_t ip_addr;     // Dirección IP asignada (en formato binario)
    time_t lease_start;   // Tiempo de inicio del arrendamiento
    int lease_duration;   // Duración del arrendamiento (en segundos)
};


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

// Función para retornar el tipo de error a la hora de crear un socket.
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

// Función para inicializar el socket.
int initialize_socket() {
    // Definición de variable que va a almacenar el socker.
    int fd;

    // Definimos la estructura que va a almacenar la ip y el puerto del servidor DHCP (ESTA ESTRUCTURA ESTA HECHA PARA ALMACENAR DIRECCIONES DE RED EN IPV4 (sockaddr_in)) y le damos de nombre server_addr.
    struct sockaddr_in server_addr; 

    // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t server_len = sizeof(server_addr);

    // Asignación a la variable el socket correspondiente IPv4 (familia de direcciones del socket), UDP (tipo del socket), y este utiliza el mismo protocolo del tipo del socket. Nos da el Id del socket que creamos.
    fd = socket(AF_INET,SOCK_DGRAM,0); 

    // Si el socket no se pudo crear satisfactoriamente, llamamos a la función de error para recibir el mensaje completo de fallo al crear un socket.
    if(fd<0){
        error("No se pudo crear el socket");
    }

    // Configuración de la estructura donde se va a almacenar la ip y el puerto.
    // Configuración de que el socket va a almacenar una IPV4.
    server_addr.sin_family = AF_INET;

    // Configuración de que el socket va a tener asociado el puerto 1067 para DHCP. htons tranforma este numero en decimal a bits de red.
    server_addr.sin_port = htons(DHCP_SERVER_PORT);

    // Configuración de la dirección IP donde el socket va a estar escuchando conexiones (el servidor para todas sus ips relacionadas con el puerto especificado va a estar escuchando). htonl tambien convierte a formato adecuado.
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 


    // Ahora, se va a enlazar el socket a la estructura de red definida y configurada anteriormente a través de la función bind().
    // bind recibe 3 parametros, el identificador del socket creado, la dirección en memoria de la estructura que contiene la ip y el puerto asociado para el socket que deseamos, y, por el utlimo, el tamaño de la estructura que definimos para que se sepa cuanta memoria se debe leer.
    if (bind(fd, (struct sockaddr *)&server_addr, server_len) < 0) {
        perror("Error en bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mensaje de confirmación de que el socket fue configurado e inicializado correctamente 
    printf("Socket inicializado y escuchando en el puerto %d\n", DHCP_SERVER_PORT);
    return fd;
}

// Función para recibir un mensaje del socket
ssize_t receive_message(int fd, char *buffer, struct sockaddr_in *client_addr, socklen_t *client_len) {
    
    // Se almacena el numero de bytes recibidos del datagrama/mensaje correspondiente
    // La función recvfrom() recibe información desde el socket correspondiente, especialmente para sockets UDP. Este obtiene el datagrama y lo almacena en el buffer.
    ssize_t msg_len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, client_len);

    // Comprueba si ocurrió un error durante la recepción del mensaje.
    if (msg_len < 0) {
        error("Error al recibir mensaje");
    }
    return msg_len;
}

// Función para obtener el mensaje DHCP para conocer la opción a realizar.
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
                // El valor del tipo de mensaje está en el tercer byte.
                return msg->options[i + 2]; 
                
            } else {
                // Si la longitud del mensaje DHCP no es valida, se retorna 'error'.
                return -1;
            }
        }
        
        // Avanzamos al siguiente campo de opciones, se avanza de esta manera puesto que cada opcion mide diferente, asi que se obtiene la siguiente opción de manera 'dinamica'.
        i += msg->options[i + 1] + 2;
    }
    // Si no se encuentra la opción 53, se devuelve un error.
    return -1;  
}

// Función para convertir una IP string (ej: "192.168.0.1") a formato entero binario
uint32_t ip_to_int(const char *ip) {
    // Se define estructura in_addr para manejar direcciones IP.
    struct in_addr addr;

    // Se convierte la dirección ip a un formato binario con la función inet_aton
    inet_aton(ip, &addr);

    // Se retorna el valor convertido a int.
    return ntohl(addr.s_addr);
}

// Función para convertir una IP en formato binario de vuelta a cadena
void int_to_ip(uint32_t ip, char *buffer) {
    // Se define estructura in_addr para manejar direcciones IP
    struct in_addr addr;

    // Convierte de formato de host a formato de red (binario)
    addr.s_addr = htonl(ip);

    // Convierte de binario a la cadena de chats
    strcpy(buffer, inet_ntoa(addr));
}

// Función para inicializar los valores de memoria de los arrendamientos (en 0) con el fin de evitar y limpiar los espacios que se pretenden ocupar.
void initialize_leases(struct lease_entry leases[MAX_LEASES]) {
    // Convertimos ips a enteros para operarlos (+1)
    uint32_t start_ip = ip_to_int(START_IP); 
    uint32_t end_ip = ip_to_int(END_IP);
    
    // Se recorre cada uno de los posibles arrendamientos y se establece en 0
    for (int i = 0; i < MAX_LEASES; i++) {
        // Iniciar con MACs vacías (para los 6 bits de la mac en 0) para todos los posibles arrendamientos que se puedan crear.
        memset(leases[i].mac_addr, 0, 6);
        
        // Se inicializa cada una de las ips definidas en el rango para todos los posibles arrendamientos que se puedan crear.
        leases[i].ip_addr = htonl(start_ip); 
        start_ip++; 

        // Se establece un valor por defecto al tiempo de inicio del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_start = 0;

        // Se establece un valor por defecto a la duración del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_duration = 0;     
    }
}

// Función para asignar una ip al cliente.
uint32_t assign_ip_to_client(struct lease_entry leases[MAX_LEASES], uint8_t *mac_addr) {
    // Se empieza a recorrer la tabla de arrendamiento para verificar ips disponibles.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Se verifica que la ip este disponible, esto pasa cuando el arrendamiento no tiene una mac asignada o ya caduco su tiempo.
        if (leases[i].mac_addr == 0 || (time(NULL) - leases[i].lease_start) > leases[i].lease_duration) {
            // Se asigna al arrendamiento con la ip disponible la MAC del cliente.
            memcpy(leases[i].mac_addr, mac_addr, 6);

            // Se obtiene el tiempo actual para definirlo en el arrendamiento para la ip disponible.
            leases[i].lease_start = time(NULL);

            // Se define la duración del arrendamiento.
            leases[i].lease_duration = 3600;

            // Se retorna la ip asignada para verificaciónes.
            return leases[i].ip_addr;
        }
    }
    return -1;
}

// Función para imprimir la tabla de arrendamientos
void print_leases(struct lease_entry leases[MAX_LEASES]) {
    printf("\nTabla de Arrendamientos:\n");
    printf("---------------------------------------------------------\n");
    printf("|    MAC Address     |         IP Address         |  Lease Start  | Lease Duration |\n");
    printf("---------------------------------------------------------\n");
    
    char ip_str[INET_ADDRSTRLEN];
    
    for (int i = 0; i < MAX_LEASES; i++) {
        // Si la dirección MAC no es vacía, imprimimos la entrada
        if (leases[i].mac_addr[0] != 0) {
            int_to_ip(ntohl(leases[i].ip_addr), ip_str);  // Convertimos la IP a cadena legible

            // Imprimir MAC en formato legible
            printf("| %02x:%02x:%02x:%02x:%02x:%02x | %s | %ld | %d |\n",
                   leases[i].mac_addr[0], leases[i].mac_addr[1], leases[i].mac_addr[2],
                   leases[i].mac_addr[3], leases[i].mac_addr[4], leases[i].mac_addr[5],
                   ip_str, leases[i].lease_start, leases[i].lease_duration);
        }
    }
    printf("---------------------------------------------------------\n");
}




// Función para enviar un DHCPOFFER.
void send_dhcp_offer(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *discover_msg, struct lease_entry leases[MAX_LEASES]) {
    // Se define la estructura que va a almacenar el mensaje de oferta que se va a enviar al cliente.
    struct dhcp_message offer_msg;

    // Llenamos la estructura que contiene el mensaje a enviar de 0's con el fin de reservar/limpiar el espacio de memoria que va a tener la información relacionada a cada campo del datagrama
    memset(&offer_msg, 0, sizeof(offer_msg));
    
    // Se llena los campos del datagrama con la información pertinente.
    // Se establece el campo con el valor de '2', indicando que el mensaje fué enviado por parte de un servidor, lo que indica una respuesta.
    offer_msg.op = 2;

    // Se establece el mismo protocolo utilizado por el cliente para enviar el mensaje, en este caso, utilizandose el protocolo ethernet, permitiendo a dispositivos comunicarse entre si en una red.
    offer_msg.htype = discover_msg->htype;

    // Se define el tamaño de la dirección de la dirección MAC que es el mismo del DHCPDISCOVER.
    offer_msg.hlen = discover_msg->hlen;

    // Identificador aleatorio que define la comunicación especifica que se esta llevando a cabo entre cliente y servidor, es la misma que DHCPDISCOVER.
    offer_msg.xid = discover_msg->xid;

    // Imprimir la tabla de arrendamientos antes de asignar la IP
    print_leases(leases);

    // Se define la IP que se le va a ofrecer al cliente para que la utilice en la red.
    uint32_t assigned_ip = assign_ip_to_client(leases, discover_msg->chaddr);

    if(assigned_ip<0){
        perror("Error al enviar DHCPOFFER");
    }


    // Se define en la estructura del mensaje que se le va a enviar al cliente la ip ofrecida
    offer_msg.yiaddr = htonl(assigned_ip);

    // Se define la dirección MAC del cliente.
    memcpy(offer_msg.chaddr, discover_msg->chaddr, 16); 
    
    // Configuramos el campo de opciones donde se especifica los parametros del mensaje que se le está mandando al cliente.
    // Se define la opción 53 que permite ubicar el lugar donde se describe el tipo del mensaje
    offer_msg.options[0] = 53;

    // Se define la logitud del tipo de mensaje
    offer_msg.options[1] = 1;

    // Se establece el ID del tipo de mensaje que se está realizando, que en este caso es un DHCPOFFER.
    offer_msg.options[2] = 2;

    // Se define el campo que especifica que se llegó al final de las opciones.
    offer_msg.options[3] = 255; 
    
    // Se utiliza la función sendto para mandar el mensaje al cliente, funcionando de manera practicamente igual que al recibir el mensaje por parte del cliente.
    ssize_t sent_len = sendto(fd, &offer_msg, sizeof(offer_msg), 0, (struct sockaddr *)client_addr, client_len);

    // Se verifica si el valor que retorna la función es menor que 0, significa que el mensaje no se pudo enviar satisfactoriamente.
    if (sent_len < 0) {
        perror("Error al enviar DHCPOFFER");
    } else {
        printf("DHCPOFFER enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}



// Función para procesar los mensajes DHCP según el tipo
void process_dhcp_message(int message_type, int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *msg, struct lease_entry leases[MAX_LEASES]) {
    switch (message_type) {
        case 1: 
            printf("Mensaje DHCPDISCOVER recibido\n");
            send_dhcp_offer(fd, client_addr, client_len, msg, leases); 
            break;
        default:
            printf("Mensaje DHCP desconocido o no soportado, tipo: %d\n", message_type);
            break;
    }
}

int main(){

    // Definimos un buffer para almacenar los datos recibidos de manera temporal, para así posteriormente procesarlos. 
    char buffer[BUFFER_SIZE]; 

    // Definición de variables: fd, que contiene el socket creado y el message_type, que almacena el tipo de mensaje recibido enviado desde el cliente
    int fd, message_type;

    // Se define la estructura para almacenar la información del cliente, ya que el servidor necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del cliente.
    struct sockaddr_in client_addr;

    // Tamaño de la dirección del clinete (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t client_len = sizeof(client_addr);

    // Tabla de arrendamiento. Aquí se encuentran todos las ips que han sido asignadas con la MAC de los clientes e información del tiempo de inicio del arrendamiento y su duración.
    struct lease_entry leases[MAX_LEASES];

    // Inicializar el socket
    fd = initialize_socket();

    // Al utilizar UDP, el servidor puede recibir mensajes en cualquier momento a través del puerto especificado, está siempre listo para recibir mensajes, no como TCP, que se debe establecer una conexión con el cliente y no siempre está escuchando
    printf("Esperando mensajes de clientes DHCP...\n");

    // Este bucle hace que el servidor DHCP este constantemente esperando recibir mensajes
    while (1) {
        // Recibir mensaje del cliente
        ssize_t msg_len = receive_message(fd, buffer, &client_addr, &client_len);

        // El mensaje que llega al servidor DHCP es una secuencia de bits, pudiendose considerar que la información está encapsulada, C como tal no es capaz de decodificar esta estructura para acceder a la información, por lo tanto, debemos definir una estructura que permita convertir esos simples bienarios en información accesible y operable para el servidor
        // Convertir el buffer a un mensaje DHCP. Se utiliza el casting de C para tratar bits crudos como una estructura
        struct dhcp_message *msg = (struct dhcp_message *)buffer;

        // Se obtiene el tipo de mensaje que se mandó del cliente para saber la acción a realizar.
        message_type = get_dhcp_message_type(msg);

        // Dependiendo del tipo de mensaje que el cliente mandó, se realiza la acción correspondiente
        process_dhcp_message(message_type, fd, &client_addr, client_len, msg, leases);

    }

    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos
    close(fd);
    return 0;
}