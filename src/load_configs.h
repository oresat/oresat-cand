#ifndef _LOAD_CONFIGS_H_
#define _LOAD_CONFIGS_H_

#include "301/CO_ODinterface.h"
#include <stdint.h>

int get_default_node_config_path(char *path, size_t path_max);
int get_default_od_config_path(char *path, size_t path_max);

int node_config_load(const char *file_path, char *can_interface, uint8_t *node_id, bool *network_manager);
int od_config_load(const char *file_path, OD_t **od, bool extend_internal_od);
void od_config_free(OD_t *od, bool extend_internal_od);

#endif
