#ifndef _PARSE_DCF_H_
#define _PARSE_DCF_H_

#include "301/CO_ODinterface.h"

int dcf_od_load(const char *file_path, OD_t **od);
void dcf_od_free(OD_t *od);

#endif
