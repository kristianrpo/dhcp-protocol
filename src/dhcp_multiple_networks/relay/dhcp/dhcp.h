#ifndef DHCP_H
#define DHCP_H

#include "../structs/structs.h"

// Función para obtener el tipo de mensaje DHCP.
int get_dhcp_message_type(struct dhcp_message *msg);

#endif
