#include "dhcp.h"

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

uint32_t ip_to_int(const char *ip) {
    // Se define estructura in_addr para manejar direcciones IP.
    struct in_addr addr;

    // Se convierte la dirección ip a un formato binario con la función inet_aton.
    inet_aton(ip, &addr);

    // Se retorna el valor convertido a int, o mejor dicho, en formato host, peritiendo asi realizar operaciones con este valor.
    return ntohl(addr.s_addr);
}

void int_to_ip(uint32_t ip, char *buffer) {
    // Se define estructura in_addr para manejar direcciones IP
    struct in_addr addr;

    // Convierte de formato de host a formato de red. Es necesario hacer esto ya que este formato es requerido para ser enviada la ip posteriormente al cliente a través de la red.
    addr.s_addr = htonl(ip);

    // Convierte de binario a la cadena de la ip y lo almacena en el buffer.
    strcpy(buffer, inet_ntoa(addr));
}

void initialize_leases(struct lease_entry leases[MAX_LEASES]) {
    // Convertimos ips a enteros para operarlos (+1)
    uint32_t start_ip = ip_to_int(START_IP); 
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

uint32_t reserved_ip_to_client(struct lease_entry leases[MAX_LEASES]) {
    // Se empieza a recorrer la tabla de arrendamiento para verificar ips disponibles.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Se verifica que la ip este disponible, esto pasa cuando el arrendamiento no tiene una mac asignada o ya caduco su tiempo.
        if (leases[i].state == 0) {
            // Se obtiene el tiempo actual para definirlo en el arrendamiento para la ip disponible.
            leases[i].lease_start = time(NULL);
            // Se define la duración del arrendamiento.
            leases[i].lease_duration = LEASE_DURATION_RESERVED;
            // Se define que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
            leases[i].state = 1;
            
            // Se retorna la ip asignada.
            return leases[i].ip_addr;
        }
    }
    return -1;
}

uint32_t assign_ip_to_client(struct lease_entry leases[MAX_LEASES], uint32_t requested_ip, uint8_t *mac_addr) {
    // Se recorre la tabla de arrendamientos para verificar si la ip solicitada por el cliente está disponible.
    for (int i = 0; i < MAX_LEASES; i++) {
        // Se verifica que la ip solicitada por el cliente sea igual a la ip de la iteración actual.
        if (leases[i].ip_addr == requested_ip) {
            // Se verifica que la ip esté reservada.
            if (leases[i].state == 1) {
                // Se define la dirección MAC.
                memcpy(leases[i].mac_addr, mac_addr, 6);\
                // Se define el tiempo de inicio del arrendamiento.
                leases[i].lease_start = time(NULL);
                // Se define la duración del arrendamiento.
                leases[i].lease_duration = LEASE_DURATION_OCCUPIED;
                // Se define que el estado de la ip es reservada para un cliente, reservada significa que es ofrecida para el mismo, pero aun no se confirma su real uso.
                leases[i].state = -1;
                // Se retorna la ip asignada.
                return leases[i].ip_addr;
            }
            else{
                return -1;
            }
        }
    }       
}

void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, uint32_t xid, uint32_t yiaddr, uint8_t *chaddr) {
    // Inicializar el mensaje DHCP con ceros.
    memset(msg, 0, sizeof(struct dhcp_message));

    // Configurar los campos principales del mensaje DHCP.
    msg->op = op;          // Tipo de operación (1 para solicitud, 2 para respuesta).
    msg->htype = htype;    // Tipo de hardware (1 para Ethernet).
    msg->hlen = hlen;      // Longitud de la dirección MAC (6 bytes para Ethernet).
    msg->xid = xid;        // ID de transacción.
    msg->yiaddr = yiaddr;  // IP asignada al cliente.

    memcpy(msg->chaddr, chaddr, 16); 
}


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
    *index += 6;
}


void set_server_identifier(uint8_t *options, int *index) {
    // Se convierte el string de la IP del servidor en un entero de 32 bits que representa la IP en formato de red (IPv4).
    uint32_t server_ip = inet_addr(IP_SERVER_IDENTIFIER);

    // Se establece en las opciones que se va a definir el identificador del servidor, y se le asigna el codigo 54 que lo representa.
    options[*index] = 54;

    // Se establece la longitud de la dirección IP del servidor (asi se conoce cuanto se debe leer o escribir de la ip cuando se necesite).
    options[*index + 1] = 4;

    // Se copia la dirección IP del servidor en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &server_ip, 4);
    
    // Avanza el índice 6 posiciones (tipo + longitud + 4 bytes de la IP).
    *index += 6;
}

void send_dhcp_offer(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *discover_msg, struct lease_entry leases[MAX_LEASES]) {
    // Se define la estructura que va a almacenar el mensaje de oferta que se va a enviar al cliente.
    struct dhcp_message offer_msg;

    // Se define la IP que se le va a ofrecer al cliente para que la utilice en la red.
    uint32_t reserved_ip = reserved_ip_to_client(leases);

    if(reserved_ip==IP_ERROR){
        error("Error al definir la IP para el cliente: No hay IPs disponibles");
    }

    // Se llama a la función que configura los campos principales del mensaje DHCP.
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

    // Llama a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 2 es el valor para DHCPOFFER.
    set_type_message(offer_msg.options, &index, 53, 1, 2); 

    // Llama a la función que configura la máscara de subred.
    set_subnet_mask(offer_msg.options, &index);

    // Llama a la función que configura el gateway.
    set_gateway(offer_msg.options, &index);

    // Llama a la función que configura el servidor DNS.
    set_dns_server(offer_msg.options, &index);

    // Llama a la función que configura el identificador del servidor.
    set_server_identifier(offer_msg.options, &index);


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

void send_dhcp_nak(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg) {
    struct dhcp_message nak_msg;
    
    // Configuramos los campos principales del mensaje DHCPNAK.
    configure_dhcp_message(&nak_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, 0, request_msg->chaddr);

    // Configuramos las opciones del mensaje DHCPNAK.
    int index = 0;
    set_type_message(nak_msg.options, &index, 53, 1, 6); // Opción 53, valor 6 (DHCPNAK).

    // Llama a la función que configura el identificador del servidor.
    set_server_identifier(nak_msg.options, &index);
    
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

uint32_t get_requested_ip(struct dhcp_message *request_msg) {

    // Se define la variable que va a almacenar la IP solicitada por el cliente.
    uint32_t requested_ip = 0;

    // Se define la variable de control para recorrer el campo de options.
    int i = 0;

    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 50, que es la que contiene la IP solicitada.
    while (i < 312) {
        // Si encontramos la opción 50 que denota la IP solicitada por el cliente.
        if (request_msg->options[i] == 50) { 
            // Se obtiene la IP solicitada por el cliente en formato de red.
            memcpy(&requested_ip, &request_msg->options[i + 2], 4);

            // Se convierte la IP solicitada por el cliente de formato de red a formato host para poder hacer operaciones con la misma.
            requested_ip = ntohl(requested_ip);

            // Se sale del ciclo.
            break;
        }
        // Avanzamos a la siguiente opción.
        i += request_msg->options[i + 1] + 2;
    }

    // Si no se encontró una IP solicitada en la opción 50, retornamos 0 como error.
    if (requested_ip == 0) {
        printf("Error: No se encontró la IP solicitada en la opción 50 del DHCPREQUEST.\n");
    }

    return requested_ip;
}

void send_dhcp_ack(int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *request_msg, struct lease_entry leases[MAX_LEASES]) { 
    // Obtenemos la IP solicitada por el cliente desde la opción 50 (Requested IP Address).
    uint32_t requested_ip = get_requested_ip(request_msg);

    // Llamamos a la función que asigna la ip al cliente y actualiza la tabla de arrendamientos.
    uint32_t assigned_ip = assign_ip_to_client(leases, requested_ip, request_msg->chaddr);
    // Si la ip no se pudo asignar, se manda un mensaje DHCPNAK.
    if (assigned_ip == IP_ERROR) {
        send_dhcp_nak(fd, client_addr, client_len, request_msg);
    }
    // Si la ip se pudo asignar, se manda un mensaje DHCPACK.
    else {
        // Se define la estructura del mensaje DHCPACK que se le va a enviar al cliente.
        struct dhcp_message ack_msg;

        // Configuramos los campos principales del mensaje DHCPACK.
        configure_dhcp_message(&ack_msg, 2, request_msg->htype, request_msg->hlen, request_msg->xid, htonl(assigned_ip), request_msg->chaddr);

        // Configuramos las opciones del mensaje DHCPACK.
        int index = 0;

        // Llama a la función que configura el tipo de mensaje, 53 es el tipo de opción, 1 es la longitud, 5 es el valor para DHCPACK.
        set_type_message(ack_msg.options, &index, 53, 1, 5);

        // Llama a la función que configura la máscara de subred
        set_subnet_mask(ack_msg.options, &index);           

        // Llama a la función que configura el gateway
        set_gateway(ack_msg.options, &index);         

        // Llama a la función que configura el servidor DNS       
        set_dns_server(ack_msg.options, &index);

        // Llama a la función que configura el identificador del servidor.
        set_server_identifier(ack_msg.options, &index);           

        // Establecemos el fin de las opciones.
        ack_msg.options[index] = 255; 

        // Enviamos el mensaje DHCPACK.
        ssize_t sent_len = sendto(fd, &ack_msg, sizeof(ack_msg), 0, (struct sockaddr *)client_addr, client_len);
        if (sent_len < 0) {
            error("Error al enviar DHCPACK");
        } else {
            printf("DHCPACK enviado al cliente: IP %s confirmada\n", inet_ntoa((struct in_addr){htonl(assigned_ip)}));
        }   
    }
}

void check_state_leases(struct lease_entry leases[MAX_LEASES]) {
    time_t current_time = time(NULL);  // Obtener el tiempo actual.
    // Recorrer la tabla de arrendamientos
    for (int i = 0; i < MAX_LEASES; i++) {
        // Si el lease está ocupado o reservado.
        if (leases[i].state != 0) {
            // Calcular el tiempo transcurrido desde que el lease fue asignado.
            time_t elapsed_time = current_time - leases[i].lease_start;

            // Si el lease ha expirado (el tiempo transcurrido es mayor que la duración del lease).
            if (elapsed_time > leases[i].lease_duration) {
                // Liberar la IP, poniendo el estado del lease como libre (0).
                leases[i].state = 0;

                // Limpiar la MAC asociada para que esté disponible.
                memset(leases[i].mac_addr, 0, 6);

                // Resetear el tiempo y la duración del lease.
                leases[i].lease_start = 0;
                leases[i].lease_duration = 0;

                // Imprimir mensaje de que el lease ha expirado y la IP ha sido liberada.
                printf("Lease para IP %s ha expirado y fue liberado.\n", inet_ntoa((struct in_addr){htonl(leases[i].ip_addr)}));
            }
        }
    }
}


void process_dhcp_message(int message_type, int fd, struct sockaddr_in *client_addr, socklen_t client_len, struct dhcp_message *msg, struct lease_entry leases[MAX_LEASES]) {
    check_state_leases(leases);
    switch (message_type) {
        case 1: 
            printf("Mensaje DHCPDISCOVER recibido\n");
            send_dhcp_offer(fd, client_addr, client_len, msg, leases); 
            break;
        case 3:
            printf("Mensaje DHCPREQUEST recibido\n");
            send_dhcp_ack(fd, client_addr, client_len, msg, leases);
            break;
        default:
            printf("Mensaje DHCP desconocido o no soportado, tipo: %d\n", message_type);
            break;
    }
}

void *handle_client(void *args) {
    // Se obtiene el contenido para cada uno de los campos definidos en la estructura y se almacena en la estructura thread_data.
    struct thread_args *thread_data = (struct thread_args *)args;

    // Se define la estructura que va a almacenar el mensaje DHCP que se va a recibir del cliente. Este mensaje se obtiene al leer el buffer que se recibe del cliente especificado en los argumentos de la función
    struct dhcp_message *msg = (struct dhcp_message *)thread_data->buffer;
    
    // Obtener el tipo de mensaje DHCP.
    int message_type = get_dhcp_message_type(msg);

    // Si el mensaje es menor a 0, significa que no se pudo identificar el mensaje recibido por parte del cliente.
    if (message_type < 0) {
        error("Error al identificar el tipo de mensaje");
    }

    // Procesar el mensaje según su tipo.
    process_dhcp_message(message_type, thread_data->fd, &thread_data->client_addr, thread_data->client_len, msg, thread_data->leases);

    // Liberar la memoria reservada para los argumentos del hilo.
    free(args);
    pthread_exit(NULL);
}
