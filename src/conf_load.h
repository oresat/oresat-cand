#ifndef _CONF_LOAD_H_
#define _CONF_LOAD_H_

#include "301/CO_ODinterface.h"
#include <stdint.h>

void node_conf_load(const char *file_path, char *can_interface, uint8_t *node_id, bool *network_manager);
int od_conf_load(const char *file_path, OD_t **od, bool extend_internal_od);
void od_conf_free(OD_t *od, bool extend_internal_od);

#endif
