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
