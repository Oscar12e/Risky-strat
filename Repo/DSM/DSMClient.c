#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include "DSMClient.h"

#define PORT 4000 

//Global variables
int server;
long currentPage; //The page we are storing the data currently
vector variables; //All the variables we have in our client

int dsm_malloc(size_t size){
    //First we send the operation
    int op = ALLOC;
    #if DEBUG
        printf("Enviado solicitud para malloc de %ld \n", size);
    #endif 
    send(server, &op, sizeof(int), 0);    

    //We start sharing what eh function waits for
    //We send the page
    send(server, &currentPage, sizeof(long), 0);
    #if DEBUG
        printf("Pagina enviada\n");
    #endif 
    //Then the size
    size_t* sizeBuff = malloc(sizeof(size_t));
    *sizeBuff = size;
    send(server, sizeBuff, sizeof(size_t), 0);
    #if DEBUG
        printf("Tamaño a reservar enviado\n");
    #endif 

    //We wait to read the result
    int result;
    #if DEBUG
        printf("Leyendo resultado\n");
    #endif 
    read(server, &result, sizeof(int));
    #if DEBUG
        printf("Resultado leido\n");
    #endif 

    if (result == OK){
        //If there was no error, then we wait for the reference
        dsm_var *var = malloc(sizeof(dsm_var));
        read(server, var, sizeof(dsm_var));
        vector_add(&variables, var);
        
        return vector_total(&variables) - 1;
    }

    //There was and error
    /*
    The error can be request here and choose to asign other page if that's the case
    */

    return -1;
    
    
}

void* dsm_read(int variable){
    //As always, the operation first
    int op = READ;
    send(server, &op, sizeof(int), 0);
    //Get the internal reference to the var and send it
    dsm_var* var = vector_get(&variables, variable);
    send(server, var, sizeof(dsm_var), 0);
    //Send the data to overwrite the node
    void* data = malloc(sizeof(var->size));
    read(server, data, sizeof(var->size));

    return data;
}

void dsm_overwrite(int variable, void* data){
    //As always, the operation first
    int op = OVERWRITE;
    send(server, &op, sizeof(int), 0);
    //Get the internal reference to the var and send it
    dsm_var* var = vector_get(&variables, variable);
    send(server, var, sizeof(dsm_var), 0);
    //Send the data to overwrite the node
    send(server, data, sizeof(var->size), 0);

    return;
}

int initConnection(){
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
