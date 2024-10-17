#include "dhcp.h"

// Función para obtener el tipo de mensaje DHCP para conocer la acción a realizar.
int get_dhcp_message_type(struct dhcp_message *msg) {
    // Definimos la variable de control para recorrer el campo de options.
    int i = 0;
    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 53, que es la que contiene el mensaje para la acción a realizar. Se recorre hasta 312 porque el campo options como maximo puede medir 312 bytes según estandarización.
    while (i < 312) {
        // Verificamos si la opción es 255, que indica el final de las opciones y salimos del ciclo.
        if (msg->options[i] == 255) {
            break;
        }
        
        // Revisamos si estamos en la opción 53 (Tipo de mensaje DHCP).
        if (msg->options[i] == 53) {
            // Aseguramos que la longitud del mensaje DHCP denota que el mismo existe.
            if (msg->options[i + 1] == 1) {
                // El valor del tipo de mensaje está en el tercer byte.
                return msg->options[i + 2]; 
                
            } else {
                // Retornamos 'error' si la longitud del mensaje DHCP no es la esperada.
                return -1;
            }
        }
        
        // Avanzamos al siguiente campo de opciones, se avanza de esta manera puesto que cada opcion mide diferente, asi que se obtiene la siguiente opción de manera 'dinamica'.
        i += msg->options[i + 1] + 2;
    }
    // Devolvemos 'error' si no encontramos la opción 53.
    return -1;  
}

// Función para inicializar los valores de memoria de los arrendamientos (en 0) con el fin de evitar y limpiar los espacios que se pretenden ocupar.
// Además se define en la tabla todas las ips disponibles para arrendar.
void initialize_leases(struct lease_entry leases[MAX_LEASES]) {
    // Convertimos ips a enteros para operarlos (+1)
    uint32_t start_ip = ip_to_int(START_IP); 
    // Recorremos cada uno de los posibles arrendamientos y los establecemos en 0.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Iniciamos con MACs vacías (para los 6 bits de la mac en 0) para todos los posibles arrendamientos que se puedan crear.
        memset(leases[i].mac_addr, 0, 6);
        
        // Inicializamos cada una de las ips definidas en el rango para todos los posibles arrendamientos que se puedan crear.
        leases[i].ip_addr = start_ip; 
        start_ip++; 

        // Establecemos un valor por defecto al tiempo de inicio del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_start = 0;

        // Establecemos un valor por defecto a la duración del arrendamiento de 0 para todos los posibles arrendamientos que se puedan crear.
        leases[i].lease_duration = 0;     

        // Inicializamos el estado como disponible.
        leases[i].state = 0;   
    }
}

// Función para asignar una ip al cliente.
uint32_t reserved_ip_to_client(struct lease_entry leases[MAX_LEASES]) {
    // Empezamos a recorrer la tabla de arrendamiento para verificar ips disponibles.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Verificamos que la ip este disponible, esto pasa cuando el arrendamiento no tiene una mac asignada o ya caduco su tiempo.
        if (leases[i].state == 0) {
            // Obtenemos el tiempo actual para definirlo en el arrendamiento para la ip disponible.
            leases[i].lease_start = time(NULL);
            // Definimos la duración del arrendamiento.
            leases[i].lease_duration = LEASE_DURATION_RESERVED;
            // Definimos que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
            leases[i].state = 1;
            
            // Retornamos la ip asignada.
            return leases[i].ip_addr;
        }
    }
    // Retornamos error si no hay ips disponibles.
    return -1;
}

// Función para asignar una ip al cliente y actualizar la tabla de arrendamientos.
uint32_t assign_ip_to_client(struct lease_entry leases[MAX_LEASES], uint32_t requested_ip, uint8_t *mac_addr) {
    // Recorremos la tabla de arrendamientos para verificar si la ip solicitada por el cliente está disponible.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Verificamos que la ip solicitada por el cliente sea igual a la ip de la iteración actual.
        if (leases[i].ip_addr == requested_ip) {
            // Verificamos que la ip esté reservada.
            if (leases[i].state == 1) {
                // Definimos la dirección MAC.
                memcpy(leases[i].mac_addr, mac_addr, 6);\
                // Definimos el tiempo de inicio del arrendamiento.
                leases[i].lease_start = time(NULL);
                // Definimos la duración del arrendamiento.
                leases[i].lease_duration = LEASE_DURATION_OCCUPIED;
                // Definimos que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
                leases[i].state = -1;
                // Retornamos la ip asignada.
                return leases[i].ip_addr;
            }
            else{
                // Retornamos error si la ip no está reservada.
                return -1;
            }
        }
    }       
}

// Función para obtener la IP solicitada por el cliente en un DHCPREQUEST que fue la que se envió en el DHCPOFFER.
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, uint32_t xid, uint32_t yiaddr, uint8_t *chaddr) {
    // Inicializamos el mensaje DHCP con ceros.
    memset(msg, 0, sizeof(struct dhcp_message));

    // Configuramos los campos principales del mensaje DHCP.
    msg->op = op;          // Tipo de operación (1 para solicitud, 2 para respuesta).
    msg->htype = htype;    // Tipo de hardware (1 para Ethernet).
    msg->hlen = hlen;      // Longitud de la dirección MAC (6 bytes para Ethernet).
    msg->xid = xid;        // ID de transacción.
    msg->yiaddr = yiaddr;  // IP asignada al cliente.

    // Copiamos la secuencia de bytes de la dirección MAC del cliente en el mensaje DHCP.
    memcpy(msg->chaddr, chaddr, 16); 
}

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP.
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, uint8_t option_value) {
    // Configuramos el tipo de mensaje en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = option_type;    
    // Asignamos la longitud de la opciónn, para indicar la longitud del valor a enviar, en el siguiente campo.
    options[*index + 1] = option_length; 
    // Especificamos el valor de la opción.
    options[*index + 2] = option_value;

    // Incrementamos el índice para la siguiente opción, se avanzan 3 posiciones, 1 para el tipo, 1 para la longitud, y 1 para el valor.
    *index += 3; 
}

// Función para establecer la máscara de subred en el mensaje DHCP.
void set_subnet_mask(uint8_t *options, int *index) {
    // Establecemos el tipo de opción a "máscara de subred" en el índice actual (opción 1).
    options[*index] = 1; 
    // Establecemos la longitud de la máscara de subred a 4 bytes (para IPv4).
    options[*index + 1] = 4; 
    // Máscara de subred
    options[*index + 2] = 255;
    options[*index + 3] = 255;
    options[*index + 4] = 255;
    options[*index + 5] = 0;
    
    // Actualizamos el índice para la siguiente opción, se avanzan 6 posiciones.
    *index += 6;
}

// Función para establecer el gateway en las opciones del mensaje DHCP.
void set_gateway(uint8_t *options, int *index) {
    // Establecemos el tipo de opción a "gateway" en el índice actual (opción 3).
    options[*index] = 3; 
    // Establecemos la longitud de la dirección del gateway a 4 bytes (para IPv4).
    options[*index + 1] = 4;  
    // Configuramos la dirección del gateway (192.168.0.1).
    options[*index + 2] = 192;
    options[*index + 3] = 168;
    options[*index + 4] = 0;
    options[*index + 5] = 1;
    
    // Incrementamos el índice para la siguiente opción, avanzando 6 posiciones.
    *index += 6; 
}

// Función para establecer el servidor DNS en las opciones del mensaje DHCP.
void set_dns_server(uint8_t *options, int *index) {
    // Establecemos el tipo de opción a "DNS server" en el índice actual (opción 6).
    options[*index] = 6; 
    // Establecemos la longitud de la dirección del servidor DNS a 4 bytes (para IPv4).
    options[*index + 1] = 4;
    // Configuramos la dirección del servidor DNS (8.8.8.8).
    options[*index + 2] = 8;
    options[*index + 3] = 8;
    options[*index + 4] = 8;
    options[*index + 5] = 8;
    
    // Incrementamos el índice para la siguiente opción, avanzando 6 posiciones.
    *index += 6;
}

// Función para establecer el identificador del servidor en las opciones del mensaje DHCP.
void set_server_identifier(uint8_t *options, int *index) {
    // Convertimos el string de la IP del servidor en un entero de 32 bits que representa la IP en formato de red (IPv4).
    uint32_t server_ip = inet_addr(IP_SERVER_IDENTIFIER);

    // Establecemos en las opciones que se va a definir el identificador del servidor, y le asignamos el codigo 54 que lo representa.
    options[*index] = 54;

    // Establecemos la longitud de la dirección IP del servidor (asi se conoce cuanto se debe leer o escribir de la ip cuando se necesite).
    options[*index + 1] = 4;

    // Copiamos la dirección IP del servidor en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &server_ip, 4);
    
    // Avanzamos el índice 6 posiciones (tipo + longitud + 4 bytes de la IP).
    *index += 6;
}

// Función para enviar un DHCPOFFER.
void send_dhcp_offer(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *discover_msg, struct lease_entry leases[MAX_LEASES]) {
    // Definimos la estructura que va a almacenar el mensaje de oferta que se va a enviar al cliente.
    struct dhcp_message offer_msg;

    // Definimos la IP que se le va a ofrecer al cliente para que la utilice en la red.
    uint32_t reserved_ip = reserved_ip_to_client(leases);

    // Verificamos si la IP que se le va a ofrecer al cliente es válida.
    if(reserved_ip==IP_ERROR){
        error("Error al definir la IP para el cliente: No hay IPs disponibles");
    }

    // LLamamos a la función que configura los campos principales del mensaje DHCP.
    // LLenamos los campos del datagrama con la información pertinente.
    // Establecemos el campo con el valor de '2', indicando que el mensaje fué enviado por parte de un servidor, lo que indica una respuesta.
    // Establecemos el mismo protocolo utilizado por el cliente para enviar el mensaje, en este caso, utilizandose el protocolo ethernet, permitiendo a dispositivos comunicarse entre si en una red.
    // Definimos el tamaño de la dirección de la dirección MAC que es el mismo del DHCPDISCOVER.
    // Definimos el identificador aleatorio que establece la comunicación especifica que se esta llevando a cabo entre cliente y servidor, es la misma que DHCPDISCOVER.
    // Definimos en la estructura del mensaje que se le va a enviar al cliente la ip ofrecida.
    // Definimos la dirección MAC del cliente en la estructura del mensaje.
    configure_dhcp_message(&offer_msg, 2, discover_msg->htype, discover_msg->hlen, discover_msg->xid, htonl(reserved_ip), discover_msg->chaddr);

    // Índice para comenzar a llenar las opciones
    int index = 0;

    // Llamamos a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 2 es el valor para DHCPOFFER.
    set_type_message(offer_msg.options, &index, 53, 1, 2); 

    // Llamamos a la función que configura la máscara de subred.
    set_subnet_mask(offer_msg.options, &index);

    // Llamamos a la función que configura el gateway.
    set_gateway(offer_msg.options, &index);

    // Llamamos a la función que configura el servidor DNS.
    set_dns_server(offer_msg.options, &index);

    // Llamamos a la función que configura el identificador del servidor.
    set_server_identifier(offer_msg.options, &index);


    // Definimos el campo que se especifica la duración del arrendamiento.
    // Definimos el codigo de la opción que determina la duración del arrendamiento de la IP.
    offer_msg.options[index] = 51;

    // Definimos la longitud que va a tener el campo de la duración del lease (4 bytes).
    offer_msg.options[index+1] = 4;

    // Cambios el valor en orden de red para ser mandado a través de la misma en el mensaje.
    uint32_t lease_time = htonl(LEASE_DURATION_OCCUPIED);

    // Establecemos en los siguientes 4 campos el valor de el tiempo de arrendamiento.
    memcpy(&offer_msg.options[index+2], &lease_time, 4);

    // Definimos el campo que especifica que se llegó al final de las opciones.
    offer_msg.options[index+6] = 255; 

    // Definimos los datos necesarios para enviar el mensaje de broadcast al cliente.
    // Definimos la dirección IP del cliente.
    client_addr->sin_family = AF_INET;
    // Configuración del puerto del cliente.
    client_addr->sin_port = htons(DHCP_CLIENT_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del cliente.
    client_addr->sin_addr.s_addr = inet_addr(BROADCAST_IP);
    
    // Utilizamos la función para mandar el mensaje al cliente, funcionando de manera practicamente igual que al recibir el mensaje por parte del cliente.
    if (send_message(fd, &offer_msg, client_addr, client_len) < 0) {
        exit(EXIT_FAILURE);
    }

    printf("DHCPOFFER enviado a %s\n", inet_ntoa(client_addr->sin_addr));

}

// Función para enviar un DHCPNAK.
void send_dhcp_nak(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg) {
    // Definimos la estructura que va a almacenar el mensaje que se va a enviar al cliente.
    struct dhcp_message nak_msg;
    
    // LLamamos a la función que configura los campos principales del mensaje DHCP.
    // LLenamos los campos del datagrama con la información pertinente.
    // Establecemos el campo con el valor de '2', indicando que el mensaje fué enviado por parte de un servidor, lo que indica una respuesta.
    // Establecemos el mismo protocolo utilizado por el cliente para enviar el mensaje, en este caso, utilizandose el protocolo ethernet, permitiendo a dispositivos comunicarse entre si en una red.
    // Definimos el tamaño de la dirección de la dirección MAC que es el mismo del DHCPREQUEST.
    // Definimos el identificador que establece la comunicación especifica que se esta llevando a cabo entre cliente y servidor, es la misma que DHCPREQUEST.
    // Establecemos el campo de IP asignada en 0, ya que no se asignará ninguna IP al cliente.
    // Definimos la dirección MAC del cliente en la estructura del mensaje.
    configure_dhcp_message(&nak_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, 0, request_msg->chaddr);

    // Índice para comenzar a llenar las opciones
    int index = 0;

    // LLamamos a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 6 es el valor para DHCPNAK.
    set_type_message(nak_msg.options, &index, 53, 1, 6); // Opción 53, valor 6 (DHCPNAK).

    // Llamamos a la función que configura el identificador del servidor.
    set_server_identifier(nak_msg.options, &index);
    
    // Establecemos el fin de las opciones.
    nak_msg.options[index] = 255;

    // Definimos los datos necesarios para enviar el mensaje de broadcast al cliente.
    // Definimos la dirección IP del cliente.
    client_addr->sin_family = AF_INET;
    // Configuración del puerto del cliente.
    client_addr->sin_port = htons(DHCP_CLIENT_PORT);
    // Definimos que se le mandará el mensaje de broadcast al puerto del cliente.
    client_addr->sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviamos el mensaje DHCPNAK.
    if (send_message(fd, &nak_msg, client_addr, client_len) < 0) {
        exit(EXIT_FAILURE);
    }

    printf("DHCPNAK enviado: IP solicitada no disponible o inválida\n");
} 

// Función para obtener la IP solicitada por el cliente en un DHCPREQUEST que fue la que se envió en el DHCPOFFER.
uint32_t get_requested_ip(struct dhcp_message *request_msg) {
    // Definimos la variable que va a almacenar la IP solicitada por el cliente.
    uint32_t requested_ip = 0;

    // Definimos la variable de control para recorrer el campo de options.
    int i = 0;

    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 50, que es la que contiene la IP solicitada.
    while (i < 312) {
        // Encontramos la opción 50 que denota la IP solicitada por el cliente.
        if (request_msg->options[i] == 50) { 
            // Obtenemos la IP solicitada por el cliente en formato de red.
            memcpy(&requested_ip, &request_msg->options[i + 2], 4);

            // Convertimos la IP solicitada por el cliente de formato de red a formato host para poder hacer operaciones con la misma.
            requested_ip = ntohl(requested_ip);

            // Salimos del ciclo.
            break;
        }
        // Avanzamos a la siguiente opción.
        i += request_msg->options[i + 1] + 2;
    }

    // Verificamos si la IP solicitada por el cliente es 0, lo que significa que no se encontró la IP solicitada en la opción 50 del DHCPREQUEST.
    if (requested_ip == 0) {
        printf("Error: No se encontró la IP solicitada en la opción 50 del DHCPREQUEST.\n");
    }

    // Retornamos la IP solicitada por el cliente.
    return requested_ip;
}

// Función para enviar un DHCPACK.
void send_dhcp_ack(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg, struct lease_entry leases[MAX_LEASES]) { 
    // Obtenemos la IP solicitada por el cliente desde la opción 50 (Requested IP Address).
    uint32_t requested_ip = get_requested_ip(request_msg);

    // Llamamos a la función que asigna la ip al cliente y actualiza la tabla de arrendamientos.
    uint32_t assigned_ip = assign_ip_to_client(leases, requested_ip, request_msg->chaddr);
    // Verificamos si la ip no se pudo asignar y mandamos un mensaje DHCPNAK.
    if (assigned_ip == IP_ERROR) {
        send_dhcp_nak(fd, client_addr, client_len, request_msg);
    }
    // Verificamos si la ip se pudo asignar y mandamos un mensaje DHCPACK.
    else {
        // Definimos la estructura del mensaje DHCPACK que se le va a enviar al cliente.
        struct dhcp_message ack_msg;

        // LLamamos a la función que configura los campos principales del mensaje DHCP.
        // LLenamos los campos del datagrama con la información pertinente.
        // Establecemos el campo con el valor de '2', indicando que el mensaje fué enviado por parte de un servidor, lo que indica una respuesta.
        // Establecemos el mismo protocolo utilizado por el cliente para enviar el mensaje, en este caso, utilizandose el protocolo ethernet, permitiendo a dispositivos comunicarse entre si en una red.
        // Definimos el tamaño de la dirección de la dirección MAC que es el mismo del DHCPREQUEST.
        // Definimos el identificador que establece la comunicación especifica que se esta llevando a cabo entre cliente y servidor, es la misma que DHCPREQUEST.
        // Establecemos el campo de IP asignada en la IP que se le asignó al cliente.
        // Definimos la dirección MAC del cliente en la estructura del mensaje.
        configure_dhcp_message(&ack_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, htonl(assigned_ip), request_msg->chaddr);

        // Índice para comenzar a llenar las opciones
        int index = 0;

        // Llamamos a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 5 es el valor para DHCPACK.
        set_type_message(ack_msg.options, &index, 53, 1, 5);

        // Llamamos a la función que configura la máscara de subred
        set_subnet_mask(ack_msg.options, &index);           

        // Llamamos a la función que configura el gateway
        set_gateway(ack_msg.options, &index);         

        // Llamamos a la función que configura el servidor DNS       
        set_dns_server(ack_msg.options, &index);

        // Llamamos a la función que configura el identificador del servidor.
        set_server_identifier(ack_msg.options, &index);           

        // Establecemos el fin de las opciones.
        ack_msg.options[index] = 255; 

        // Definimos los datos necesarios para enviar el mensaje de broadcast al cliente.
        // Definimos la dirección IP del cliente.
        client_addr->sin_family = AF_INET;
        // Configuración del puerto del cliente.
        client_addr->sin_port = htons(DHCP_CLIENT_PORT);
        // Definimos que se le mandará el mensaje de broadcast al puerto del cliente.
        client_addr->sin_addr.s_addr = inet_addr(BROADCAST_IP);

        // Enviamos el mensaje DHCPACK.
        if (send_message(fd, &ack_msg, client_addr, client_len) < 0) {
            exit(EXIT_FAILURE);
        }

        // Verificamos si el valor que retorna la función es menor que 0, significa que el mensaje no se pudo enviar satisfactoriamente.
        printf("DHCPACK enviado al cliente: IP %s confirmada\n", inet_ntoa((struct in_addr){htonl(assigned_ip)})); 
    }
}

// Función para verificar si un arrendamiento ha expirado y liberar la IP.
void check_state_leases(struct lease_entry leases[MAX_LEASES]) {
    // Obtenemos el tiempo actual.
    time_t current_time = time(NULL);
    // Recorremos la tabla de arrendamientos.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Verificamos si la IP está asignada.
        if (leases[i].state != 0) {
            // Calculamos el tiempo transcurrido desde que el lease fue asignado.
            time_t elapsed_time = current_time - leases[i].lease_start;

            // Verificamos si el lease ha expirado (el tiempo transcurrido es mayor que la duración del lease).
            if (elapsed_time > leases[i].lease_duration) {
                // Liberamos la IP, poniendo el estado del lease como libre (0).
                leases[i].state = 0;

                // Limpiamos la MAC asociada para que esté disponible.
                memset(leases[i].mac_addr, 0, 6);

                // Reseteamos el tiempo y la duración del lease.
                leases[i].lease_start = 0;
                leases[i].lease_duration = 0;

                // Imprimimos el mensaje de que el lease ha expirado y la IP ha sido liberada.
                printf("Lease para IP %s ha expirado y fue liberado.\n", inet_ntoa((struct in_addr){htonl(leases[i].ip_addr)}));
            }
        }
    }
}

// Función para procesar el mensaje DHCP según su tipo.
void process_dhcp_message(int message_type, int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *msg, struct lease_entry leases[MAX_LEASES]) {
    // Verificamos si un arrendamiento ha expirado y liberamos la IP.
    check_state_leases(leases);
    // Procesamos el mensaje según su tipo.
    switch (message_type) {
        case 1: 
            printf("Mensaje DHCPDISCOVER recibido\n");
            // Luego de recibir el mensaje DHCPDISCOVER, se envía un DHCPOFFER al cliente.
            send_dhcp_offer(fd, client_addr, client_len, msg, leases); 
            break;
        case 3:
            printf("Mensaje DHCPREQUEST recibido\n");
            // Luego de recibir el mensaje DHCPREQUEST, se envía un DHCPACK al cliente.
            send_dhcp_ack(fd, client_addr, client_len, msg, leases);
            break;
        default:
            printf("Mensaje DHCP desconocido o no soportado, tipo: %d\n", message_type);
            break;
    }
}

// Función para manejar la lógica para procesar una solicitud DHCP de un cliente en un hilo separado.
void *handle_client(void *args) {
    // Obtenemos el contenido para cada uno de los campos definidos en la estructura y lo almacenamos en la estructura thread_data.
    struct thread_args *thread_data = (struct thread_args *)args;

    // Definimos la estructura que va a almacenar el mensaje DHCP que se va a recibir del cliente. Este mensaje se obtiene al leer el buffer que se recibe del cliente especificado en los argumentos de la función
    struct dhcp_message *msg = (struct dhcp_message *)thread_data->buffer;
    
    // Obtenemos el tipo de mensaje DHCP.
    int message_type = get_dhcp_message_type(msg);

    // Verificamos si el mensaje es menor a 0, lo que significa que no se pudo identificar el mensaje recibido por parte del cliente.
    if (message_type < 0) {
        error("Error al identificar el tipo de mensaje");
    }

    // Procesamos el mensaje según su tipo.
    process_dhcp_message(message_type, thread_data->fd, &thread_data->client_addr, thread_data->client_len, msg, thread_data->leases);

    // Liberamos la memoria reservada para los argumentos del hilo.
    free(args);

    // Salimos del hilo.
    pthread_exit(NULL);
}
