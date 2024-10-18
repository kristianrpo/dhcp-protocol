#include "dhcp.h"

// Función para obtener el tipo de mensaje DHCP.
int get_dhcp_message_type(struct dhcp_message *msg) {
    // Definimos la variable de control para recorrer el campo de options.
    int i = 0;

    // Recorremos el campo de 'options' del datagrama recibido, options puede tener una cantidad de opciones variable, por lo tanto, tenemos que recorrer options hasta encontrar la opción 53, que es la que contiene el mensaje para la acción a realizar.
    while (i < 312) {
        if (msg->options[i] == 53) { 
            return msg->options[i + 2];  
        }
        i += msg->options[i + 1] + 2;
    }
    return -1;  
}

// Función para configurar el mensaje DHCP.
void configure_dhcp_message(struct dhcp_message *msg, uint8_t op, uint8_t htype, uint8_t hlen, 
                            uint32_t xid, uint16_t flags, uint8_t *chaddr) {
    // Inicializamos el mensaje DHCP con ceros.
    memset(msg, 0, sizeof(struct dhcp_message));

    // Configuramos los campos principales del mensaje DHCP.
    msg->op = op;          // Tipo de operación (1 para solicitud, 2 para respuesta).
    msg->htype = htype;    // Tipo de hardware (1 para Ethernet).
    msg->hlen = hlen;      // Longitud de la dirección MAC (6 bytes para Ethernet).
    msg->xid = xid;        // ID de transacción.
    msg->flags = flags;    // Bandera de broadcast.

    // Copiamos la secuencia de bytes de la dirección MAC del cliente en el mensaje DHCP.
    memcpy(msg->chaddr, chaddr, 6);  // Solo los primeros 6 bytes de la MAC se copian.
}

// Función para configurar el tipo de mensaje en las opciones del mensaje DHCP.
void set_type_message(uint8_t *options, int *index, uint8_t option_type, uint8_t option_length, 
                      uint8_t option_value) {
    // Configuramos el tipo de mensaje en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = option_type;

    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = option_length; 

    // Especificamos el valor de la opción.
    options[*index + 2] = option_value;

    // Incrementamos el índice para la siguiente opción (se avanzan 3 posiciones).
    *index += 3; 
}

// Función para configurar la dirección IP solicitada en las opciones del mensaje DHCP.
void set_requested_ip(uint8_t *options, int *index, uint32_t requested_ip) {
    // Configuramos la opción de la IP solicitada en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = 50;  // Opción 50 es la IP solicitada.
    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = 4;  // Longitud de la dirección IP.
    // Copiamos la dirección IP solicitada en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &requested_ip, 4);  // Copiamos 4 bytes de la IP solicitada.
    // Incrementamos el índice para la siguiente opción (se avanzan 6 posiciones).
    *index += 6; 
}

// Función para configurar la dirección IP del servidor en las opciones del mensaje DHCP.
void set_server_identifier(uint8_t *options, int *index, uint32_t server_identifier) {
    // Configuramos la opción de la dirección IP del servidor en las opciones del mensaje DHCP en el índice especificado.
    options[*index] = 54;  // Opción 54 es la dirección IP del servidor.
    // Asignamos la longitud de la opción para indicar la longitud del valor a enviar.
    options[*index + 1] = 4;  // Longitud de la dirección IP.
    // Copiamos la dirección IP del servidor en la estructura del mensaje DHCP.
    memcpy(&options[*index + 2], &server_identifier, 4);  // Copiamos 4 bytes de la dirección IP del servidor.
    // Incrementamos el índice para la siguiente opción (se avanzan 6 posiciones).
    *index += 6; 
}



// Función para extraer el tiempo de lease del mensaje DHCP.
uint32_t get_lease_time(struct dhcp_message *msg) {
    // Definimos varibale de control que nos permitirá recorrer el campo de opciones.
    int i = 0;

    // Recorremos el campo de opciones.
    // 312 es el tamaño máximo de las opciones en DHCP.
    while (i < 312) { 
        // Código de la opción.
        uint8_t option_code = msg->options[i];

        // Si llegamos al final de las opciones, salimos.
        if (option_code == 255) {
            break;
        }

        // Si encontramos la opción de Lease Time (51).
        if (option_code == 51) {
            // Obtenemos la longitud de la opción (que debería ser 4).
            uint8_t option_length = msg->options[i + 1]; 

            if (option_length == 4) {

                // Definimos la variable donde se va a almacenar el tiempo de arrendamiento.
                uint32_t lease_time;

                // Copiamos los 4 bytes del tiempo de lease.
                memcpy(&lease_time, &msg->options[i + 2], 4);

                // Convertimos el valor de network byte order a host byte order.
                return ntohl(lease_time);
            }
        }

        // Avanzamos al siguiente bloque de opciones (opción + longitud + datos).
        i += 2 + msg->options[i + 1];  // Código (1 byte) + longitud (1 byte) + longitud de la opción.
    }

    // Si no encontramos la opción 51, devolvemos 0.
    return 0;
}


// Función para renovar el lease de la IP actual.
void renew_lease(const char *interface, struct dhcp_message *ack_msg, int send_socket, int recv_socket, struct sockaddr_in *relay_addr) {
    // Definimos la estructura que va a almacenar la ip para imprimir mensaje.
    struct in_addr addr;
    addr.s_addr = ack_msg->yiaddr;
    
    // Declaramos relay_len como socklen_t para utilizarla en receive_message
    socklen_t relay_len = sizeof(*relay_addr);

    // Definimos un tamaño de buffer.
    char buffer[BUFFER_SIZE];

    // Definimos la variable que va a almacenar el tamaño del mensaje recibido.
    ssize_t message_len;

    printf("Renovando la IP %s para la interfaz %s...\n", inet_ntoa(addr), interface);

    // Definimos el mensaje DHCPREQUEST para la renovación.
    struct dhcp_message request_msg;
    memset(&request_msg, 0, sizeof(request_msg));

    // Configuramos los parámetros principales del mensaje DHCP.
    // Usamos el mismo XID que el recibido en el mensaje DHCPACK.
    configure_dhcp_message(&request_msg, 1, 1, 6, ack_msg->xid, htons(0x8000), ack_msg->chaddr);

    // Definimos el índice de las opciones.
    int request_index = 0;

    // Tipo de mensaje DHCP (solicitud de renovación - tipo 3).
    set_type_message(request_msg.options, &request_index, 53, 1, 3);

    // Solicitamos la IP ya asignada (que el cliente ya está usando).
    set_requested_ip(request_msg.options, &request_index, ack_msg->yiaddr);

    // Configuramos la dirección IP del servidor DHCP (basada en el ACK).
    uint32_t server_ip = get_server_identifier(ack_msg);
    set_server_identifier(request_msg.options, &request_index, server_ip);

    // Establecemos el final de las opciones.
    request_msg.options[request_index] = 255;

    // Definimos los datos necesarios para enviar el mensaje al server.
    relay_addr->sin_family = AF_INET;
    relay_addr->sin_port = htons(DHCP_RELAY_PORT);
    relay_addr->sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviar el mensaje DHCPREQUEST al servidor DHCP para la renovación.
    if (send_message(send_socket, &request_msg, relay_addr, sizeof(*relay_addr)) < 0) {
        exit(EXIT_FAILURE);
    }

    printf("Mensaje DHCPREQUEST de renovación enviado al servidor.\n");


    // Ahora esperamos el DHCPACK en respuesta.
    while (1) {
        // Recibimos la respuesta del servidor.
        message_len = receive_message(recv_socket, buffer, relay_addr, &relay_len);

        // Procesamos el mensaje recibido y verificamos si es un DHCPACK.
        struct dhcp_message *new_ack_msg = process_msg(buffer);
        if (new_ack_msg != NULL) {
            // Obtenemos el tipo de mensaje DHCP
            int ack_type = get_dhcp_message_type(new_ack_msg);
            if (ack_type == 5) {
                printf("Renovación de lease exitosa:\n");
                print_network_config(new_ack_msg);

                // Asignamos la IP renovada a la interfaz.
                assign_ip_to_interface(interface, new_ack_msg);
                break;
            } else {
                if (ack_type == 6){
                    printf("El servidor ha enviado un DHCPNAK. Esto significa que la IP no se pudo renovar y se debe hacer un DHCPDISCOVER nuevamente para obtener una IP\n");
                    release_ip(interface, ack_msg);
                    exit(0);
                }
            }
        }
    }
}


// Función que ejecuta el hilo que se encarga de renovar el lease de la IP.
void *lease_renewal(void *arg) {
    struct renew_args *args = (struct renew_args *)arg;

    // Obtenemos el tiempo de lease para la ip asignada al cliente.
    int lease_time = get_lease_time(args->ack_msg); 

    // Definimos una variable para almacenar el tiempo que ha transcurrido desde que se asignó la IP.
    int time_elapsed = 0;

    // Definimos el umbral para renovar el lease (70% del tiempo de lease).
    int renew_threshold = lease_time * 0.7;

    // Definimos la variable de control que nos permite determinar cuantas renovaciones queremos hacer.
    int i=0;

    while (i<=3) {
        // Esperamos 1 segundo en cada ciclo para contar correctamente los segundos.
        sleep(1);

        // Incrementamos el tiempo transcurrido.
        time_elapsed += 1;

        // Si hemos alcanzado el 70% del tiempo de lease, renovamos.
        if (time_elapsed >= renew_threshold) {
            printf("El tiempo de lease ha alcanzado el 70%%. Renovando IP...\n");
            sleep(2);

            // Llamamos a la función para renovar el lease de la IP.
            renew_lease(args->interface, args->ack_msg, args->send_socket, args->recv_socket, &args->relay_addr);

            // Reiniciamos el contador de tiempo transcurrido después de renovar el lease.
            time_elapsed = 0;

            // Volvemos a calcular el umbral del 70% basándonos en el nuevo lease_time recibido.
            lease_time = get_lease_time(args->ack_msg);
            renew_threshold = lease_time * 0.7;

            // Bloqueamos el mutex para enviar la señal de renovación
            pthread_mutex_lock(&flag_mutex);

            // Enviamos una señal al hilo de monitoreo
            pthread_cond_signal(&cond_var);

            pthread_mutex_unlock(&flag_mutex);

            // Aumentamos la variable de control para identificar cuantas renovaciones se han hecho.
            i++;
        }
    }
    return NULL;
}


// Función para enviar DHCPRELEASE
void send_dhcp_release(int send_socket, uint32_t assigned_ip, uint32_t server_ip, uint32_t transaction_id, uint8_t *chaddr, struct sockaddr_in *relay_addr, socklen_t relay_len) {
    // Crear el mensaje DHCPRELEASE
    struct dhcp_message release_msg;
    memset(&release_msg, 0, sizeof(release_msg));

    // Configurar los parámetros principales del mensaje DHCP para un release
    configure_dhcp_message(&release_msg, 1, 1, 6, transaction_id, htons(0x0000), chaddr);

    // Asignamos la dirección IP que estamos liberando en el campo 'ciaddr' del mensaje
    release_msg.ciaddr = assigned_ip;

    // Definimos el índice para las opciones del mensaje DHCP
    int release_index = 0;

    // Configuramos el mensaje DHCP para que sea un mensaje de tipo release (opción 53, valor 7)
    set_type_message(release_msg.options, &release_index, 53, 1, 7);

    // Configuramos la dirección IP del servidor en las opciones (opción 54)
    set_server_identifier(release_msg.options, &release_index, server_ip);

    // Establecemos el final de las opciones
    release_msg.options[release_index] = 255; // Fin de las opciones

    relay_addr->sin_family = AF_INET;
    relay_addr->sin_port = htons(DHCP_RELAY_PORT);
    relay_addr->sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Enviamos el mensaje al servidor (no se espera respuesta en DHCPRELEASE)
    if (send_message(send_socket, &release_msg, relay_addr, relay_len) < 0) {
        printf("Error al enviar DHCPRELEASE\n");
        exit(EXIT_FAILURE);
    }

    printf("DHCPRELEASE enviado correctamente\n");
}