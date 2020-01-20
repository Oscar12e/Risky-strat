#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <semaphore.h>
#include "DSMClient.h"

#define PORT 4000 

//Global variables
sem_t peticiones;
int server;
long currentPage; //The page we are storing the data currently
vector variables; //All the variables we have in our client

int dsm_malloc(size_t size){
    #if DEBUG
        printf(KGRN "[ALLOC]" KNRM ": Inicio.\n");
        printf("Solicitando un espacio de %ld en la pagina %ld\n", size, currentPage );
    #endif 
    sem_wait(&peticiones);
    //Everything is send in the order the server expects it
    //First we send the operation
    int op = ALLOC;
    send(server, &op, sizeof(int), 0);    
    //The page we are using
    send(server, &currentPage, sizeof(long), 0);
    //Then the size we need to alloc
    send(server, &size, sizeof(size_t), 0);
    //Then we wait to read the result
    int result;
    read(server, &result, sizeof(int));
    if (result == OK){ //The scope of the project doesn't cover
        //If there was no error, then we wait for the reference
        var_ref *varRef = malloc(sizeof(var_ref));
        read(server, varRef, sizeof(var_ref));
        vector_add(&variables, varRef);
        int index  = vector_total(&variables) - 1;
        sem_post(&peticiones);
        #if DEBUG
            printf( KGRN "[ALLOC]" KNRM ": Creado con exito.\n");
            printf(KWHT "\t-> " KNRM "index: %d\n", index);
            printf(KWHT "\t-> " KNRM "size: %ld\n", varRef->size);
            printf(KWHT "\t-> " KNRM "page: %ld\n", varRef->pageNum);
            printf(KWHT "\t-> " KNRM "offset: %ld\n", varRef->offset);
        #endif 

        return index;
    } else { //Here ww can have a switch to manage the errors 
        #if DEBUG
            printf("[ALLOC]: Error %d \n", result);
        #endif 
    }

    //There was and error
    /*
    The error can be request here and choose to asign other page if that's the case
    */
    sem_post(&peticiones);
    return -1;
    
    
}

void* dsm_read(int variable){
    sem_wait(&peticiones);
    var_ref* varRef = vector_get(&variables, variable);
    #if DEBUG
        printf(KGRN "[READ]" KNRM ": Inicio.\n");
        printf("Leyendo del server, datos de la referencia:\n");
        printf(KWHT "\t-> " KNRM "size: %ld\n", varRef->size);
        printf(KWHT "\t-> " KNRM "page: %ld\n", varRef->pageNum);
        printf(KWHT "\t-> " KNRM "offset: %ld\n", varRef->offset);
    #endif 
    //As always, the operation first
    int op = READ;
    send(server, &op, sizeof(int), 0);
    //Get the internal reference to the var and send it
    send(server, varRef, sizeof(var_ref), 0);
    
    //Send the data to overwrite the node
    long readBytes = 0;
    unsigned char* data = malloc(sizeof(varRef->size));
    while (readBytes < varRef->size){
        readBytes += read(server, data + readBytes, varRef->size - readBytes);
    }
    sem_post(&peticiones);
    return data;
}

//NOTE: C doesnt know the size of the data pointer
void dsm_overwrite(int variable, void* data){
    sem_wait(&peticiones); 
    //Set the operation 
    int op = OVERWRITE;
    //Get the reference to the var and send it
    var_ref* varRef = vector_get(&variables, variable);
    #if DEBUG
        printf("\n" KGRN "[WRITE]" KNRM ": Inicio.\n");
        printf("Escribiendo datos en el server, datos de la referencia:\n");
        printf(KWHT "\t-> " KNRM "size: %ld\n", varRef->size);
        printf(KWHT "\t-> " KNRM "page: %ld\n", varRef->pageNum);
        printf(KWHT "\t-> " KNRM "offset: %ld\n", varRef->offset);
    #endif 
    //As always, the operation first
    send(server, &op, sizeof(int), 0);
    send(server, varRef, sizeof(var_ref), 0);
    //Send the data to overwrite the node
    long sentBytes = 0; //Record of how many bytes we sent
    while(sentBytes < varRef->size){ //Keep sending till all is done
        sentBytes += send(server, data + sentBytes, varRef->size - sentBytes, 0);
    }
    sem_post(&peticiones);
    return;
}

int initConnection(){
    sem_init(&peticiones, 0, 1);
    vector_init(&variables);

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
    
    
    int operation = REGISTER_CLIENT;
    printf("Registrando\n");
    send(sock , &operation , sizeof(int) , 0 ); 
    server = sock;

    printf("Obteniendo pagina libre\n");
    long *buffer = malloc(sizeof(long));
    read(sock, buffer, sizeof(long));
    
    /*if (*buffer == ERROR){
        //In case no pages are free
        printf("No free page\n");
        return 0;
    } else { //In other case, it */
        printf("Free page it seems %ld\n", *buffer);
        currentPage = *buffer;


//    }

    //Connection is open by now on
    return 1; 
}
