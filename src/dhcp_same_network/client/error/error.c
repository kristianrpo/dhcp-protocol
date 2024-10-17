#include "error.h"

// Función para imprimir un mensaje de error y terminar el programa.
void error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}
