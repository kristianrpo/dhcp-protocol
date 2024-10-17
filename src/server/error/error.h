#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>         // Proporciona funciones estándar de entrada y salida como printf() y scanf().
#include <stdlib.h>        // Ofrece funciones para la gestión de memoria dinámica (malloc, free) y control del programa (exit()).

// Función para manejar errores utilizando fprintf
void error(const char *msg);

#endif 