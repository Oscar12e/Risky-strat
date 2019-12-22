#ifndef NODE_H
#define NODE_H
#include <stdlib.h>
#include "vector.h"
#include "DSMCommon.h"

//Pendiente al tamaño que se ocupa y el que se reserva
//Void * == 8 bytes
//Unsigned char == 1 bytes
typedef struct block{
    long startIndex;
    size_t totalSize;
    size_t occupied;
    void** content;
} block;

typedef struct page {
    long pageNum;
    size_t occupied;
    unsigned char* data;
} page;


void handleRequest(int operation);
page* newPage(long num, long size);
var_ref* newDsmVar(long num, size_t size, size_t offset);

void setupPages();

void allocInMemory();
void readMemory();
void writeInMemory();
#endif

