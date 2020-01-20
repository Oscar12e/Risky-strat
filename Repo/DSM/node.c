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
long pageSize, amountOfPages;
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
    long buffer[3] = {0}; // {pageSize, pagesPerNode, 0}; 
    int valread = read(server, buffer, sizeof(long) * 3); 

    pageSize = buffer[0];
    long pagesToAlloc = buffer[1];
    amountOfPages = pagesToAlloc;
    indexStart = buffer[2];
    indexEnd = indexStart + pagesToAlloc;

    #if DEBUG
        printf("\n[Setup]: ");
        printf("Allocando %ld paginas de tamaño %ld en el nodo.\n", pagesToAlloc , pageSize);
    #endif

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
        printf("\n[Alloc]: ");
        printf("Allocando %ld bytes en la pagina: %d.\n", size, pageNum);
    #endif

    //We get the page at that position and the offset of the free space
    page* dataPage = vector_get(&pagesBlock, pageNum % amountOfPages );
    long offset = dataPage->occupied; //Can calculate
    
    //Update the opccupied 
    long newOccupied = dataPage->occupied + size;

    if (newOccupied > pageSize){
        #if DEBUG
            printf("[Alloc - error]: Pagina sin espacio %ld - %ld.\n", newOccupied, pageSize);
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
    //Read the reference of the variable
    var_ref* varRef = malloc(sizeof(var_ref));
    int valread = read(server, varRef, sizeof(var_ref));
    
    #if DEBUG
        printf("\n[Write]: ");
        printf("Escribiendo %ld bytes. (Pagina %ld - offset %ld).\n",
            varRef->size, varRef->pageNum, varRef->offset);
    #endif

    //With it, the have all we need to access the the data 
    page* dataPage = vector_get(&pagesBlock, varRef->pageNum % amountOfPages );
    //Our index to the memory will simply be like this
    long index = varRef->offset;
    //We use a dataBuffer to get the var info
    //unsigned char* data = malloc(sizeof(unsigned char) * varRef->size);
    //We will need to receive the data to store it
    long readBytes = 0;
    unsigned char* data = malloc(sizeof(varRef->size));

    while (readBytes < varRef->size){
        readBytes += read(server, data + readBytes, varRef->size - readBytes);
    }
    //valread = read(server, data, var->size);

    int * readed = (int *) data;
    
    //Set the value byte per byte into the page
    for (int move = 0; move < varRef->size; move++){
        dataPage->data [index + move] = data[move];
    }

}

void readMemory(){
    var_ref* varRef = malloc(sizeof(var_ref));
    int valread = read(server, varRef, sizeof(var_ref));

    #if DEBUG
        printf("\n[Read]: ");
        printf("Leyendo %ld bytes. (Pagina %ld - offset %ld).\n",
            varRef->size, varRef->pageNum,  varRef->offset);
    #endif

    page* dataPage = vector_get(&pagesBlock, varRef->pageNum % amountOfPages );
    long index = varRef->offset;
    unsigned char* data = malloc(sizeof(varRef->size));

    //Set the values
    for (int move = 0; move < varRef->size; move++){
        data[move] = dataPage->data[index + move];
    }

    //To finish we send back the data we read
    //send(server, data, var->size, 0);
    long sentBytes = 0; //Record of how many bytes we sent
    while(sentBytes < varRef->size){ //Keep sending till 
        sentBytes += send(server, data + sentBytes, varRef->size - sentBytes, 0);
    }
}


/*Just to add rows to the log
Tan solo recibe el sring y lo guarda
*/
void addToLog(char* entry){
	char* filename = "Log.txt"; //Wish I could make it a const -> Icould
	FILE* file = fopen(filename, "a");
	fprintf(file, "%s", entry);
	fclose(file);

	printf("\nSaving on log\n");
}

void saveSnapshot(){
    int size = vector_total(&pagesBlock);
    for (int i = 0; i < 1; i++){
        page* currentPage = vector_get(&pagesBlock, i);
        printf("\nPage %ld\n", currentPage->pageNum);
        char filename[100] = 

        for (long i = 0; i < size; i++){
            printf("%c", currentPage->data[i]);
        }
    }

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
            printf("\n[Handler]: Esperando intrucciones\n" ); 
        #endif 
        valread = read( sock , &operation, sizeof(int)); 

        if (valread == 0){ //A failsafe in case the server goes down
            close(sock);
            exit(0);
            return -1;
        }

        #if DEBUG
            printf("[Handler]: Instrucción recibida: %d\n", operation);
        #endif 
        
        handleRequest(operation);

        #if DEBUG
             printf("[Handler]: Operacion terminada\n\n");
        #endif        
    }

    return 0; 
} 
