#ifndef _PARSE_DCF_H_
#define _PARSE_DCF_H_

#include <stdint.h>
#include "301/CO_ODinterface.h"

int dcf_od_load(const char *file_path, OD_t **od, uint8_t *node_id);
void dcf_od_free(OD_t *od);

#endif
