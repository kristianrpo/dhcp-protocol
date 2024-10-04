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
#define LEASE_DURATION_RESERVED 10 // Segundos.
#define LEASE_DURATION_OCCUPIED 3600 // Segundos.

struct lease_entry {
    uint8_t mac_addr[6];  // Dirección MAC del cliente
    uint32_t ip_addr;     // Dirección IP asignada (en formato binario)
    time_t lease_start;   // Tiempo de inicio del arrendamiento
    int lease_duration;   // Duración del arrendamiento (en segundos)
    int state;            // 1 si está reservada, 0 si está libre, -1 ocupada.
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

// Función para manejar errores utilizando fprintf
void error(const char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
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
        error("Error al enlazar el socket con la estructura de red definida");
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

    // Se retorna el valor convertido a int, o mejor dicho, en formato host, peritiendo asi realizar operaciones con este valor.
    return ntohl(addr.s_addr);
}

// Función para convertir una IP en formato binario de vuelta a cadena
void int_to_ip(uint32_t ip, char *buffer) {
    // Se define estructura in_addr para manejar direcciones IP
    struct in_addr addr;

    // Convierte de formato de host a formato de red. Es necesario hacer esto ya que este formato es requerido para ser enviada la ip posteriormente al cliente a través de la red.
    addr.s_addr = htonl(ip);

    // Convierte de binario a la cadena de la ip y lo almacena en el buffer.
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
        leases[i].ip_addr = start_ip; 
        start_ip++; 

        // Se establece un valor por defecto al tiempo de inicio del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_start = 0;

        // Se establece un valor por defecto a la duración del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_duration = 0;     

        // Se inicializa el estado como disponible.
        leases[i].state = 0;   
    }
}

// Función para asignar una ip al cliente.
uint32_t set_ip_to_client(struct lease_entry leases[MAX_LEASES], uint8_t *mac_addr, int flag_action) {
    // Se empieza a recorrer la tabla de arrendamiento para verificar ips disponibles.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Se verifica que la ip este disponible, esto pasa cuando el arrendamiento no tiene una mac asignada o ya caduco su tiempo.

        if (leases[i].state == 0) {
            // Se obtiene el tiempo actual para definirlo en el arrendamiento para la ip disponible.
            leases[i].lease_start = time(NULL);

            if(flag_action==0) {
            // Se define la duración del arrendamiento.
            leases[i].lease_duration = LEASE_DURATION_RESERVED;
            // Se define que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
            leases[i].state = 1;
            }
            else {
                // Se define la dirección MAC.
                memcpy(leases[i].mac_addr, mac_addr, 6);
                // Se define la duración del arrendamiento.
                leases[i].lease_duration = LEASE_DURATION_OCCUPIED;
                // Se define que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
                leases[i].state = -1;
            }

            // Se retorna la ip asignada.
            return leases[i].ip_addr;
        }
    }
    return -1;
}

// Función para configurar los campos principales del mensaje DHCP
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, uint32_t xid, uint32_t yiaddr, uint8_t *chaddr) {
    // Inicializar el mensaje DHCP con ceros
    memset(msg, 0, sizeof(struct dhcp_message));

    // Configurar los campos principales del mensaje DHCP
    msg->op = op;          // Tipo de operación (1 para solicitud, 2 para respuesta).
    msg->htype = htype;    // Tipo de hardware (1 para Ethernet).
    msg->hlen = hlen;      // Longitud de la dirección MAC (6 bytes para Ethernet).
    msg->xid = xid;        // ID de transacción.
    msg->yiaddr = yiaddr;  // IP asignada al cliente.

    memcpy(msg->chaddr, chaddr, 16); 
}


// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, uint8_t option_value) {
    // Configura el tipo de mensaje en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = option_type;    
    // Asigna la longitud de la opciónn, para indicar la longitud del valor a enviar, en el siguiente campo.
    options[*index + 1] = option_length; 
    // Especifica el valor de la opción.
    options[*index + 2] = option_value;

    // Incrementa el índice para la siguiente opción, se avanzan 3 posiciones, 1 para el tipo, 1 para la longitud, y 1 para el valor.
    *index += 3; 
}

// Función para establecer la máscara de subred en el mensaje DHCP
void set_subnet_mask(uint8_t *options, int *index) {
    // Establece el tipo de opción a "máscara de subred" en el índice actual (opción 1).
    options[*index] = 1; 
    // Establece la longitud de la máscara de subred a 4 bytes (para IPv4).
    options[*index + 1] = 4; 
    // Máscara de subred
    options[*index + 2] = 255;
    options[*index + 3] = 255;
    options[*index + 4] = 255;
    options[*index + 5] = 0;
    
    // Actualiza el índice para la siguiente opción, se avanzan 6 posiciones.
    *index += 6;
}

// Función para establecer el gateway en las opciones del mensaje DHCP
void set_gateway(uint8_t *options, int *index) {
    // Establece el tipo de opción a "gateway" en el índice actual (opción 3).
    options[*index] = 3; 
    // Establece la longitud de la dirección del gateway a 4 bytes (para IPv4).
    options[*index + 1] = 4;  
    // Configura la dirección del gateway (192.168.0.1).
    options[*index + 2] = 192;
    options[*index + 3] = 168;
    options[*index + 4] = 0;
    options[*index + 5] = 1;
    
    // Incrementa el índice para la siguiente opción, avanzando 6 posiciones.
    *index += 6; 
}


// Función para establecer el servidor DNS en las opciones del mensaje DHCP
void set_dns_server(uint8_t *options, int *index) {
    // Establece el tipo de opción a "DNS server" en el índice actual (opción 6).
    options[*index] = 6; 
    // Establece la longitud de la dirección del servidor DNS a 4 bytes (para IPv4).
    options[*index + 1] = 4;
    // Configura la dirección del servidor DNS (8.8.8.8).
    options[*index + 2] = 8;
    options[*index + 3] = 8;
    options[*index + 4] = 8;
    options[*index + 5] = 8;
    
    // Incrementa el índice para la siguiente opción, avanzando 6 posiciones.
    *index += 6;  // Actualiza el índice
}


// Función para enviar un DHCPOFFER.
void send_dhcp_offer(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *discover_msg, struct lease_entry leases[MAX_LEASES]) {
    // Se define la estructura que va a almacenar el mensaje de oferta que se va a enviar al cliente.
    struct dhcp_message offer_msg;

    // Se define la IP que se le va a ofrecer al cliente para que la utilice en la red.
    uint32_t reserved_ip = set_ip_to_client(leases, discover_msg->chaddr,0);

    if(reserved_ip<0){
        error("Error al definir la IP para el cliente: No hay IPs disponibles");
    }

    // Se llama a la función que configura los campos principales del mensaje DHCP
    // Se llena los campos del datagrama con la información pertinente.
    // Se establece el campo con el valor de '2', indicando que el mensaje fué enviado por parte de un servidor, lo que indica una respuesta.
    // Se establece el mismo protocolo utilizado por el cliente para enviar el mensaje, en este caso, utilizandose el protocolo ethernet, permitiendo a dispositivos comunicarse entre si en una red.
    // Se define el tamaño de la dirección de la dirección MAC que es el mismo del DHCPDISCOVER.
    // Se define el identificador aleatorio que establece la comunicación especifica que se esta llevando a cabo entre cliente y servidor, es la misma que DHCPDISCOVER.
    // Se define en la estructura del mensaje que se le va a enviar al cliente la ip ofrecida.
    // Se define la dirección MAC del cliente en la estructura del mensaje.
    configure_dhcp_message(&offer_msg, 2, discover_msg->htype, discover_msg->hlen, discover_msg->xid, htonl(reserved_ip), discover_msg->chaddr);

     // Índice para comenzar a llenar las opciones
    int index = 0;

    // Llama a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 2 es el valor para DHCPOFFER
    set_type_message(offer_msg.options, &index, 53, 1, 2); 

    // Llama a la función que configura la máscara de subred
    set_subnet_mask(offer_msg.options, &index);

    // Llama a la función que configura el gateway
    set_gateway(offer_msg.options, &index);

    // Llama a la función que configura el servidor DNS
    set_dns_server(offer_msg.options, &index);

    // Se define el campo que se especifica la duración del arrendamiento.
    // Se define el codigo de la opción que determina la duración del arrendamiento de la IP.
    offer_msg.options[index] = 51;

    // Se define la longitud que va a tener el campo de la duración del lease (4 bytes).
    offer_msg.options[index+1] = 4;

    // Se cambia el valor en orden de red para ser mandado a través de la misma en el mensaje.
    uint32_t lease_time = htonl(LEASE_DURATION_OCCUPIED);

    // Se establece en los siguientes 4 campos el valor de el tiempo de arrendamiento.
    memcpy(&offer_msg.options[index+2], &lease_time, 4);

    // Se define el campo que especifica que se llegó al final de las opciones.
    offer_msg.options[index+6] = 255; 
    
    // Se utiliza la función sendto para mandar el mensaje al cliente, funcionando de manera practicamente igual que al recibir el mensaje por parte del cliente.
    ssize_t sent_len = sendto(fd, &offer_msg, sizeof(offer_msg), 0, (struct sockaddr *)client_addr, client_len);

    // Se verifica si el valor que retorna la función es menor que 0, significa que el mensaje no se pudo enviar satisfactoriamente.
    if (sent_len < 0) {
        error("Error al enviar DHCPOFFER");
    } else {
        printf("DHCPOFFER enviado a %s\n", inet_ntoa(client_addr->sin_addr));
    }
}

void send_dhcp_ack(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg, struct lease_entry leases[MAX_LEASES]) {
    // Se define la estructura del mensaje que se va a mandar al cliente
    struct dhcp_message ack_msg;
    
    // Obtenemos la IP mandada por el cliente que se le fue reservada en el DHCPOFFER.
    uint32_t requested_ip = ntohl(request_msg->yiaddr);

    // Iteramos para buscar la IP reservada 
    for (int i = 0; i < MAX_LEASES; i++) {
        if (leases[i].ip_addr == requested_ip) {
            if(leases[i].state==1){
                // Actualizamos el estado de la IP como ocupada.
                leases[i].lease_start = time(NULL); 
                leases[i].lease_duration = LEASE_DURATIION_OCCUPIED; 
                leases[i].state = -1;  // -1 significa que la IP está en uso.

                // Configuramos los campos principales del mensaje DHCPACK.
                configure_dhcp_message(&ack_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, htonl(requested_ip), request_msg->chaddr);

                // Configuramos las opciones del mensaje DHCPACK.
                int index = 0;
                set_type_message(ack_msg.options, &index, 53, 1, 5); // Opción 53, valor 5 (DHCPACK)
                set_subnet_mask(ack_msg.options, &index);            // Máscara de subred.
                set_gateway(ack_msg.options, &index);                // Gateway.
                set_dns_server(ack_msg.options, &index);             // Servidor DNS.
                set_renewal_times(ack_msg.options, &index, LEASE_DURATIION_OCCUPIED); // T1 y T2.

                // Establecemos el fin de las opciones.
                ack_msg.options[index] = 255; 

                // Enviamos el mensaje DHCPACK.
                ssize_t sent_len = sendto(fd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)client_addr, client_len);
                if (sent_len < 0) {
                    error("Error al enviar DHCPACK");
                } else {
                    printf("DHCPACK enviado al cliente: IP %s confirmada\n", inet_ntoa((struct in_addr){htonl(requested_ip)}));
                }   
            }
            else{
                send_dhcp_nak(fd, client_addr, client_len, request_msg);
            }
    }
}

void send_dhcp_nak(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg) {
    struct dhcp_message nak_msg;
    
    // Configuramos los campos principales del mensaje DHCPNAK.
    configure_dhcp_message(&nak_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, 0, request_msg->chaddr);

    // Configuramos las opciones del mensaje DHCPNAK.
    int index = 0;
    set_type_message(nak_msg.options, &index, 53, 1, 6); // Opción 53, valor 6 (DHCPNAK).
    
    // Establecemos el fin de las opciones.
    nak_msg.options[index] = 255; 

    // Enviamos el mensaje DHCPNAK.
    ssize_t sent_len = sendto(fd, &nak_msg, sizeof(nak_msg), 0, (struct sockaddr *)client_addr, client_len);
    if (sent_len < 0) {
        error("Error al enviar DHCPNAK");
    } else {
        printf("DHCPNAK enviado: IP solicitada no disponible o inválida\n");
    }
}



void check_state_leases(struct lease_entry leases[MAX_LEASES]) {
    time_t current_time = time(NULL);  // Obtener el tiempo actual
    // Recorrer la tabla de arrendamientos
    for (int i = 0; i < MAX_LEASES; i++) {
        // Si el lease está ocupado o reservado
        if (leases[i].state != 0) {
            // Calcular el tiempo transcurrido desde que el lease fue asignado
            time_t elapsed_time = current_time - leases[i].lease_start;

            // Si el lease ha expirado (el tiempo transcurrido es mayor que la duración del lease)
            if (elapsed_time > leases[i].lease_duration) {
                // Liberar la IP, poniendo el estado del lease como libre (0)
                leases[i].state = 0;

                // Limpiar la MAC asociada para que esté disponible
                memset(leases[i].mac_addr, 0, 6);

                // Resetear el tiempo y la duración del lease
                leases[i].lease_start = 0;
                leases[i].lease_duration = 0;

                printf("Lease para IP %s ha expirado y fue liberado.\n", inet_ntoa((struct in_addr){htonl(leases[i].ip_addr)}));
            }
        }
    }
}


// Función para procesar los mensajes DHCP según el tipo
void process_dhcp_message(int message_type, int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *msg, struct lease_entry leases[MAX_LEASES]) {
    check_state_leases(leases);
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

    // Inicializar la tabla de arrendamientos
    initialize_leases(leases);

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

        if(message_type<0){
            error("Error al identificar el tipo de mensaje");
        }

        // Dependiendo del tipo de mensaje que el cliente mandó, se realiza la acción correspondiente
        process_dhcp_message(message_type, fd, &client_addr, client_len, msg, leases);

    }

    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos
    close(fd);
    return 0;
}