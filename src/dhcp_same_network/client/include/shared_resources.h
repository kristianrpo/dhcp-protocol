#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

#include <pthread.h>

// Declarar el mutex externamente
extern pthread_mutex_t flag_mutex;
extern pthread_cond_t cond_var;

#endif
