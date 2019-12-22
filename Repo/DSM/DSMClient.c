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
int currentPage; //The page we are storing the data currently
vector variables; //All the variables we have in our client

int dsm_malloc(size_t size){
    int operation = ALLOC;
    
    send(server, operation, sizeof(int), 0);
    send(server, size, sizeof(size_t), 0);

    dsm_var *var = malloc(sizeof(dsm_var));
    read(server, var, sizeof(dsm_var));
    vector_add(&variables, var);
    
    return vector_total(&variables) - 1;
}

void* dsm_read(int variable){
    int operation = READ;
    send(server, operation, sizeof(int), 0);

    dsm_var* var = vector_get(&variables, variable);

    send(server, var, sizeof(dsm_var), 0);

    void* data = malloc(var->size);
    read(server, data, var->size);

    return data;
}

void dsm_overwrite(int variable, void* data){
    int operation = OVERWRITE;
    send(server, operation, sizeof(int), 0);
    
    dsm_var* var = vector_get(&variables, variable);
    send(server, var, sizeof(dsm_var), 0);
    send(server, data, var->size, 0);

    return data;
}

void init(){
    vector_init(&variables);

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
    
    
    int operation = REGISTER_CLIENT;
    send(sock , &operation , sizeof(int) , 0 ); 
    server = sock;


    int *buffer = malloc(sizeof(int));
    read(sock, buffer, sizeof(int));
    if (*buffer == ERROR){
        //In case no pages are free
        printf("No free page\n");
        return 0;
    } else { //In other case, it 
        currentPage = *buffer;
    }

    //Connection is open by now on
    return 1; 
}
