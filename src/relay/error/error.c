#include "error.h"

// Función para manejar errores utilizando fprintf.
void error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}
