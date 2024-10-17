#include "utils.h"
// Función para convertir una IP string (ej: "192.168.0.1") a formato entero binario
uint32_t ip_to_int(const char *ip) {
    // Deifinimos la estructura in_addr para manejar direcciones IP.
    struct in_addr addr;

    // Convertimos la dirección ip a un formato binario con la función inet_aton.
    inet_aton(ip, &addr);

    // Retornamos el valor convertido a int, o mejor dicho, en formato host, permitiendo asi realizar operaciones con este valor.
    return ntohl(addr.s_addr);
}

// Función para convertir una IP en formato binario de vuelta a cadena
void int_to_ip(uint32_t ip, char *buffer) {
    // Definimos la estructura in_addr para manejar direcciones IP
    struct in_addr addr;

    // Convertimos de formato host a formato de red. Es necesario hacer esto ya que este formato es requerido para ser enviada la ip posteriormente al cliente a través de la red.
    addr.s_addr = htonl(ip);

    // Convertimos de binario a la cadena de la ip y lo almacenamos en el buffer.
    strcpy(buffer, inet_ntoa(addr));
}