#ifndef DSM_H
#define DSM_H
#include <stdlib.h>
#include "DSMCommon.h"

typedef struct page_ref {
    int frame;
    long pageNumber;
    int validationBit;
} page_ref;

typedef struct dsm_var {
    long pageNum;
    size_t offset;
    size_t size;
} dsm_var;

void init();
void setupNodes();
void* listen_requests();
void dsm_malloc(int* client);
void overwrite(int*client);
void* read_value();



void configure();
void setupNodes();
void initializeClient();
void asignPage(); //To client
void displayContent();
void menu();
void handleClient();
#endif