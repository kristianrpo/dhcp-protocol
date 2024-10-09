#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include "socket/socket.h"
#include "dhcp/dhcp.h"
#include "structs/dhcp_structs.h"
#include "constants/dhcp_constants.h"
#include "error/error.h"
int main(){
    // Definimos un buffer para almacenar los datos recibidos de manera temporal, para así posteriormente procesarlos. 
    char buffer[BUFFER_SIZE]; 

    // Definición de variable fd, que contiene el socket creado.
    int fd;

    // Definici[on de variable message_len, que almacena la longitud del mensaje recibido.
    ssize_t message_len;

    // Se define la estructura para almacenar la información del servidor.
    struct sockaddr_in server_addr;

    // Se define la estructura para almacenar la información del cliente, ya que el servidor necesita saber quien mandó un mensaje. Es importante reconocer el puerto y la ip desde la cual se envió el mensaje del cliente.
    struct sockaddr_in client_addr;

    // Tamaño de la dirección del servidor (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t server_len = sizeof(server_addr);

    // Tamaño de la dirección del cliente (para saber cuantos bytes se debe leer o escribir en la estructura).
    socklen_t client_len = sizeof(client_addr);

    // Inicializar el socket.
    fd = initialize_socket(&server_addr, server_len);

    // Tabla de arrendamiento. Aquí se encuentran todos las ips que han sido asignadas con la MAC de los clientes e información del tiempo de inicio del arrendamiento y su duración.
    struct lease_entry leases[MAX_LEASES];

    // Inicializar la tabla de arrendamientos.
    initialize_leases(leases);

    
    // Al utilizar UDP, el servidor puede recibir mensajes en cualquier momento a través del puerto especificado, está siempre listo para recibir mensajes, no como TCP, que se debe establecer una conexión con el cliente y no siempre está escuchando.
    printf("Esperando mensajes de clientes DHCP...\n");

    // Este bucle hace que el servidor DHCP este constantemente esperando recibir mensajes.
    while (1) {
        // Reservar memoria para los argumentos del hilo.
        struct thread_args *args = malloc(sizeof(struct thread_args));
        if (args == NULL) {
            error("Error al asignar memoria para los argumentos del hilo");
        }

        // Recibir el mensaje del cliente.

        // Se guarda el identificador del socket en los args.
        args->fd = fd;

        // Se guarda la longitud de la estructura de la diracción del cliente.
        args->client_len = client_len;

        // Se guarda el mensaje del cliente en los args (se guarda en el args->buffer el buffer obtenido en la función).
        message_len = receive_message(fd, args->buffer, &args->client_addr, &args->client_len);

        // se guarda la tabla de arrendamiento en los args
        args->leases = leases;

        // Crear un nuevo hilo para procesar la solicitud.

        // Definimos la variable que va a almacenar el id del hilo.
        pthread_t thread_id;

        // Se crea un hilo para procesar la solicitud del cliente, se le pasa la función handle_client, que es la función que se encarga de procesar las solicitudes DHCP de los clientes.
        // Se le pasa la estructura que va a almacenar el id del hilo para cuado se cree el mismo.
        if (pthread_create(&thread_id, NULL, handle_client, (void *)args) != 0) {
            error("Error al crear el hilo");
        }

        // Detach el hilo para que se limpie automáticamente al finalizar.
        pthread_detach(thread_id);
    }

    // Cerrar el socket cuando ya no se use para evitar mal gastar recursos.
    close(fd);
    return 0;
}