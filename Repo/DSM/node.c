#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include "node.h"

#define PORT 4000 


vector pagesBlock;

//Global variables
long pageSize;
long indexStart, indexEnd;
int server;

page* newPage(long num, long size){
    page* _page = malloc(sizeof(page));
    _page->pageNum = num;
    _page->data = malloc(sizeof(unsigned char) * size);

    for (long i = 0; i < size; i++){
        _page->data[i] = '0';
    }

    _page->occupied = 0;
    return _page;
}

var_ref* newDsmVar(long num, size_t size, size_t offset){
    var_ref* _var = malloc(sizeof(var_ref));
    _var->pageNum = num;
    _var->size = size;
    _var->offset = offset;
    return _var;   
}


void handleRequest(int operation){
    printf("\nHandling a request\n");
    int valread;
    
    switch (operation){
        case SET_PAGES:
            vector_init(&pagesBlock);
            setupPages();
            break;
        case ALLOC: //i need info to alloc
            allocInMemory();
            break;
        case READ:
            readMemory();
            break;
        case OVERWRITE: //Pendiente
            writeInMemory();
            break;
        case DISPLAY:
            printf("Displaying\n");
            int size = vector_total(&pagesBlock);
            for (int i = 0; i < 1; i++){
                page* currentPage = vector_get(&pagesBlock, i);
                printf("\nPage %ld\n", currentPage->pageNum);

                for (long i = 0; i < size; i++){
                    printf("%c", currentPage->data[i]);
                }
            }
        break;
        default:
            break;
    }    
}

void setupPages(){
    printf("\nSetting the pages\n");  
    long buffer[3] = {0}; // {pageSize, pagesPerNode, 0}; 
    int valread = read(server, buffer, sizeof(long) * 3); 

    pageSize = buffer[0];
    long pagesToAlloc = buffer[1];
    indexStart = buffer[2];

    indexEnd = indexStart + pagesToAlloc;

    for (long pageIndex = indexStart; pageIndex < indexEnd; pageIndex++){
        page* p = newPage(pageIndex, pageSize);
        vector_add(&pagesBlock, p);
    }
}

void allocInMemory(){
    //First we read the information of pageNum and size
    long buffer[2] = {0};
    int valread = read(server, buffer, sizeof(long) * 2);
    long pageNum = buffer[0];
    size_t size = buffer[1]; //I need this
    
    #if DEBUG
        printf("\nAlloc\n");
        printf("Storing %ld bytes on the page%d\n", size, pageNum);
    #endif

    //We get the page at that position and the offset of the free space
    page* dataPage = vector_get(&pagesBlock, indexStart - pageNum);
    long offset = dataPage->occupied; //Can calculate
    
    //Update the opccupied 
    long newOccupied = dataPage->occupied + size;

    if (newOccupied > pageSize){
        #if DEBUG
            printf("Pagina sin espacio %ld - %ld.\n", newOccupied, pageSize);
        #endif
        int op = ERROR;
        send(server, &op, sizeof(int), 0);
    } else {
        int op = OK;
        send(server, &op, sizeof(int), 0);
        dataPage->occupied += size; //Note we can have a segmentation default here

        //Create a new reference with the data
        var_ref* var = newDsmVar(pageNum, size, offset);
        send(server, var, sizeof(var_ref), 0); //Devuelve el alloc
        
        #if DEBUG
            printf("Operation finised.\n", size, pageNum);
        #endif
    }
    
}

void writeInMemory(){
    var_ref* var = malloc(sizeof(var_ref));
    int valread = read(server, var, sizeof(var_ref));

    //Using the information for the variable, we write on the offset
    //corresponding to the variable
    page* dataPage = vector_get(&pagesBlock, indexStart - var->pageNum);

    //Our index to the memory will simply be like this
    long index = var->offset;
    //We use a dataBuffer to get the var info
    unsigned char* data = malloc(sizeof(unsigned char) * var->size);
    //We will need to receive the data to store it
    valread = read(server, data, var->size);
    //Set the value byte per byte into the page
    for (int move = 0; move < var->size; move++){
        dataPage->data [index + move] = data[move];
    }

}

void readMemory(){
    printf("Reading\n");
    var_ref* var = malloc(sizeof(var_ref));
    int valread = read(server, var, sizeof(var_ref));

    page* dataPage = vector_get(&pagesBlock, indexStart - var->pageNum);
    long index = var->offset;
    unsigned char* data = malloc(sizeof(var->size));

    //Set the values
    for (int move = 0; move < var->size; move++){
        data[move] = dataPage->data[move];
    }

    //To finish we send back the data we read
    printf("Reading done");
    send(server, data, var->size, 0);
}

int main(int argc, char const *argv[]) { 
    //Pasar a otra función

    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
    
    //Conect to localhost 
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 

    while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nConexión con el DSM ha fallado. Reintentando en 5 segundos.\n");
        sleep(5);
    }
    
    
    int operation = ADD_NODE;
    send(sock , &operation , sizeof(int) , 0 ); 
    server = sock;

    while (1){
        int operation;
        #if DEBUG
            printf("Esperando intrucciones\n" ); 
        #endif 
        valread = read( sock , &operation, sizeof(int)); 

        if (valread == 0){ //A failsafe in case the server goes down
            close(sock);
            exit(0);
            return -1;
        }

        #if DEBUG
            printf("\nInstrucción recibida: %d\n", operation);
        #endif 
        
        handleRequest(operation);

        #if DEBUG
             printf("Operacion terminada\n");
        #endif        
    }

    return 0; 
} 
