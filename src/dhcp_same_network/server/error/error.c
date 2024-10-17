#include "error.h"

// Función para manejar errores utilizando fprintf
void error(const char *msg) {
    // Imprimimos el mensaje de error en la salida de error estándar y terminamos el programa.
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}
