#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>

// Función para convertir una IP string (ej: "192.168.0.1") a formato entero binario
uint32_t ip_to_int(const char *ip);

// Función para convertir una IP en formato binario de vuelta a cadena
void int_to_ip(uint32_t ip, char *buffer);

#endif