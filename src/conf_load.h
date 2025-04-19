#ifndef _CONF_LOAD_H_
#define _CONF_LOAD_H_

#include "301/CO_ODinterface.h"
#include <stdint.h>

int od_conf_load(const char *file_path, OD_t **od, bool extend_internal_od);
void od_conf_free(OD_t *od, bool extend_internal_od);

#endif
