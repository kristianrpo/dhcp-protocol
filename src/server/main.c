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

    // Reservamos un espacio en memoria para almacenar el identificador del socket que se va a utilizar.
    int fd;

    // Definimos la estructura para almacenar la longitud del mensaje recibido.
    ssize_t message_len;

    // Definimos la estructura para almacenar la informacion de la dirección del servidor
    struct sockaddr_in server_addr;

    // Definimos la estructura para almacenar la información de la dirección del cliente, 
    // incluyendo el puerto y la IP desde la cual se envió el mensaje, para poder responderle.
    struct sockaddr_in client_addr;

    // Almacenamos el tamaño en bytes de la dirección del servidor (para saber cuanta memoria se debe leer o escribir para obtener la información de la dirección).
    socklen_t server_len = sizeof(server_addr);

    // Almacenamos el tamaño en bytes de la dirección del cliente (para saber cuanta memoria se debe leer o escribir para obtener la información de la dirección).
    socklen_t client_len = sizeof(client_addr);

    // Inicializamos el socket y almacenamos su identificador.
    fd = initialize_socket(&server_addr, server_len);

    // Definimos la estructura que va a almacenar los arrendamientos de las direcciones IP,
    // almacenará todos las IPs que han sido asignadas, junto a la respectiva MAC de los clientes, el tiempo de inicio del arrendamiento y su duración.
    struct lease_entry leases[MAX_LEASES];

    // Inicializamos la tabla de arrendamientos.
    initialize_leases(leases);

    
    // Con UDP, el servidor puede recibir mensajes en cualquier momento a través del puerto especificado, 
    // ya que siempre está listo para recibirlos. A diferencia de TCP, donde es necesario establecer 
    // una conexión con el cliente antes de la comunicación, en UDP no se requiere esta conexión previa.
    printf("Esperando mensajes de clientes DHCP...\n");

    // Utilizamos este bucle para que el servidor DHCP constantemente a la espera de mensajes de los clientes.
    while (1) {
        // Reservamos un espacio en memoria para almacenar los argumentos que se le van a pasar al hilo.
        struct thread_args *args = malloc(sizeof(struct thread_args));

        // Verificamos si se pudo reservar memoria para los argumentos del hilo.
        if (args == NULL) {
            error("Error al asignar memoria para los argumentos del hilo");
        }

        // Al recibir un mensaje del cliente

        // Guardamos el identificador del socket en los argumentos.
        args->fd = fd;

        // Guardamos la longitud de la dirección del cliente en los argumentos.
        args->client_len = client_len;

        // Guardamos el mensaje del cliente en los argumentos,
        // se guarda el buffer obtenido en la función en el buffer de los argumentos (args->buffer).
        message_len = receive_message(fd, args->buffer, &args->client_addr, &args->client_len);

        // Guardamos la tabla de arrendamientos en los argumentos.
        args->leases = leases;

        // Para procesar la solicitud del cliente.

        // Definimos la variable que va a almacenar el id del hilo.
        pthread_t thread_id;

        // Creamos un hilo para procesar la solicitud del cliente, a este, 
        // se le pasa la función handle_client, que es la encargada de procesar las solicitudes DHCP de los clientes,
        // y se le pasa la estructura que va a almacenar el id del hilo para cuando se cree.
        if (pthread_create(&thread_id, NULL, handle_client, (void *)args) != 0) {
            error("Error al crear el hilo");
        }

        // Realizamos un detach del hilo, para que se libere la memoria una vez que el hilo termine su ejecución.
        pthread_detach(thread_id);
    }

    // Cerramos el socket una vez que el servidor termine su ejecución, para liberar los recursos utilizados.
    close(fd);
    return 0;
}