#ifndef DSM_H
#define DSM_H
#include <stdlib.h>
#include "DSMCommon.h"

typedef struct page_ref {
    int frame;
    long pageNumber;
    int validationBit;
} page_ref;

void init();
void setupNodes();
void* listen_requests();
void allocData(int* client);
void readStored(int* client);
void storeData(int* client);



void configure();
void setupNodes();
void initializeClient();
void asignPage(); //To client
void displayContent();
void menu();
void* handleClient(void*);
#endif