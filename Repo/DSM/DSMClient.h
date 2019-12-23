#ifndef DSMCLIENT_H
#define DSMCLIENT_H
#include <stdlib.h>
#include "vector.h"
#include "DSMCommon.h"

//Which pages do i have?
//Offset per variable

//I need to know with pages i own

//I need a reference to the memory i have used
//Only a int of the client but a reference to me



typedef struct dsm_var {
    long pageNum;
    size_t offset;
    size_t size;
} dsm_var;

int dsm_malloc(size_t size);
void* dsm_read();
void dsm_overwrite(int, void*);

#endif