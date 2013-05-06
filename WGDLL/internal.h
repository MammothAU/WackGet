
#ifndef WGDLL_INTERNAL_H
#define WGDLL_INTERNAL_H

#include "main.h"

extern int dll_ref_cnt;  /* the dll reference count */

/* wgbho.c */
int create_wgbho_instance(void **out_ifc);

#endif
