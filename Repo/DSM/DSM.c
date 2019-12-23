#include <stdio.h>//Standard input-output
#include <stdlib.h>//Standard library
#include <unistd.h>//Standard symbolic constants and types
#include <math.h>//Math functionality
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>//String functionality

#include <semaphore.h>

#include "DSM.h"
#include "vector.h"

sem_t sem_peticiones;
vector tablePages;
vector dsmNodes;
vector dsmClients;

size_t pageSize;
long totalPages, pagesPerNode;

int aceptingNodes = 1;
int aceptingClients = 1;

//Auxiliar function to know if the node has been added
int containsNode(int node){
    for (int i = 0; i < vector_total(&dsmNodes); i++){
        int* content = (int*) vector_get(&dsmNodes, i);
        if (*content == node) return 1;
    }

    return 0;
}

int containsClient(int node){

    for (int i = 0; i < vector_total(&dsmNodes); i++){
        int* content = (int*) vector_get(&dsmNodes, i);
        if (*content == node) return 1;
    }

    return 0;
}

void* handleClient(void* socket){
    int *clientSock = (int *) socket;
    int *in = malloc(sizeof(int));
    int valread;

    #if DEBUG
            printf("Inicializado el cliente %d\n", *clientSock);
    #endif 

    do {
        valread = read(*clientSock, in, sizeof(int));

        #if DEBUG
            printf("Instruccion recibida %d del cliente %d\n", *in, *clientSock);
        #endif 
        //sem_wait(&sem_peticiones);

        #if DEBUG
            printf("For pete's sake\n");
        #endif 

        switch (*in){
            case ALLOC:
                allocData(*clientSock);
                break;
            case OVERWRITE:
                storeData(clientSock);
                /* code */
                break;
            case READ:
                readStored(clientSock);
                /* code */
                break;
            default:
                break;
        }

        //sem_post(&sem_peticiones);

    } while (valread > 0);
 
    //El cliente se ha descontectado
}


//Nodes code
int addNode(int sender){
    if (containsNode(sender) == 1) return 0;

    int* socket = malloc(sizeof(int));
    *socket = sender;
    vector_add(&dsmNodes, socket);
    printf("\nNuevo nodo añadido: %d - %d\n", sender, *socket);     
}

page_ref* getFreePage(){
    for (int i = 0; i < vector_total(&tablePages); i++){
        page_ref* currentPage = (page_ref*) vector_get(&tablePages, i);

        if (currentPage->validationBit == 0) {
            currentPage->validationBit = 1;
            return currentPage;
        }    
    }
    return NULL;
}

void initializeClient(int clientSock){
    page_ref* freePage = getFreePage();
    //We don't use enougth process to cover all pages, so there's not validation

    //Send a page for use
    long * pageNum = malloc(sizeof(long)); 
    *pageNum = freePage->pageNumber;

    #if DEBUG
        printf("Nuevo cliente\n");
        printf("PAgina libre %ld asignada al cliente %d \n", *pageNum, clientSock);
    #endif 


    send(clientSock, pageNum, sizeof( long ), 0);
    
    //Run the handler
    int *client = malloc(sizeof(int));
    *client = clientSock;
    pthread_t client_thread;

    if( pthread_create( &client_thread , NULL ,  handleClient , (void *) client ) < 0) {
        printf("ERROR: No se pudo crear el hilo\n");
        return;
	}

    #if DEBUG
        printf("Cliente inicializado\n");
    #endif 
}

/*
*/
void allocData(int client){
    //First, tell me the page
    #if DEBUG
        printf("Allocando para %d \n", client);
    #endif 
    long* pageNum = malloc(sizeof(long));

    read(client, pageNum, sizeof(long));
    #if DEBUG
        printf("PageNUm %ld \n", *pageNum);
    #endif 

    size_t* size = malloc(sizeof(size_t));
     #if DEBUG
        printf("Size %ld \n", *size);
    #endif 
    read(client, size, sizeof(size_t));

    int frame = *pageNum / pagesPerNode;
    int* node = vector_get(&dsmNodes, frame);
    
    #if DEBUG
        printf("Solicitada la opción de Malloc para un tamaño de \n", size);
        printf("\nSe guarda en el frame %d\n", frame);
    #endif 

    //We send the operation we want to perfom
    int op = ALLOC;
    send(*node, &op, sizeof( int ), 0);

    //send(*node, pageNum, sizeof(long), 0);
    //send(*node, &size, sizeof(size_t), 0);
    
    long out[2] = {*pageNum, (long) size};
    //Then we send the information to realiaze the operation
    send(*node, out, sizeof(long)*2, 0);

    //And we read the result struct
    int result;
    read(*node, &result, sizeof(int));
    send(client, &result, sizeof(int), 0);
    if (result == OK){
        var_ref* reference = malloc(sizeof(var_ref));
        read(*node, reference, sizeof(var_ref));
        send(client, reference, sizeof(var_ref), 0);
        
        #if DEBUG
            printf("Allocado sin problemas \n", size);
        #endif 
    } else {
        #if DEBUG
            printf("Allocado sin problemas \n", size);
        #endif 
    }

}

void storeData(int* client){
    //Get the actual variable info
    //Use that info to access the page
    var_ref* varRef = malloc(sizeof(var_ref));
    read(*client, varRef, sizeof(var_ref));

    //Get the data from the client
    void* dataBuffer = malloc(sizeof(varRef->size));
    read(*client, dataBuffer, varRef->size);

    //Check the buffer to know if the size sent is right
    
    int frame = varRef->pageNum / pagesPerNode;
    int* node = vector_get(&dsmNodes, frame);

    //Send the operation to the node
    int op = OVERWRITE;
    send(*node, &op, sizeof( int ), 0);

    //We transfere the data to the node
    send(*node, varRef, sizeof( var_ref ), 0);

    //And then send the data to the node to overwrite the space in the node
    send(*node, dataBuffer, varRef->size, 0);
}


void readStored(int* client){
    //Get the actual variable info
    //Use that info to access the page
    var_ref* varRef = malloc(sizeof(var_ref));
    read(*client, varRef, sizeof(var_ref));

    int frame = varRef->pageNum / pagesPerNode;
    int* node = vector_get(&dsmNodes, frame);

    //Send the operation to the node
    int op = READ;
    send(*node, &op, sizeof( int ), 0);
    //We transfere the reference to the node
    send(*node, varRef, sizeof( var_ref ), 0);
    //Get the data from the node
    void* dataBuffer = malloc(sizeof(varRef->size));
    read(*node, dataBuffer, varRef->size);
    
    //And then send the data to the client
    send(*client, dataBuffer, varRef->size, 0);
}

//source code taken from: geeksforgeeks
void* listen_requests(){
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    
    int option = -1;
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) { 
        perror("setsockopt"); exit(EXIT_FAILURE); 
    } 

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) { 
        perror("bind failed"); exit(EXIT_FAILURE); 
    } 

    if (listen(server_fd, 3) < 0) { 
        perror("listen"); exit(EXIT_FAILURE); 
    } 

    //Queremos escuchar cada nueva conexión
    while (1){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); exit(EXIT_FAILURE); 
        } 

        printf("New conection to the server\n" ); 
        valread = read( new_socket , &option, sizeof(int)); 

        if (option == ADD_NODE){
            if (aceptingNodes) addNode(new_socket);
            else {
                printf("La configuración de los nodos ya fue realizada. "
                "No se aceptarán más nodos.");
            }
        } else if (option == REGISTER_CLIENT) {
            printf("Client connected\n");
            initializeClient(new_socket);
        }
    }
    
    return 0; 
}


//Initialize the nodes with the pages needs for the 
/* Function: configure
 * Request the user to insert the values for the global variables of
 * totalPages and pageSize.
*/
void configure(){
    printf("Inicialización del servidor\n");

    printf("Ingrese cuantas paginas desea: \n");
    scanf("%ld", &totalPages);

    printf("Ingrese el tamaño de ñas paginas: \n");
    scanf("%ld", &pageSize);

    printf("Se esta reservado %ld bytes de memoria separados en paginas de %ld\n",
        totalPages * pageSize);

    setupNodes();
}

void setupNodes(){
    //Asumamos que la rawPageSize > 0 && totalMemory > rawSizePage
    int operation = SET_PAGES;
    int totalNodes = vector_total(&dsmNodes);

    if (totalNodes == 0) {
        //terminates all threads
        exit(0);
    }

    pagesPerNode = totalPages / totalNodes;

    long params[3] = {pageSize, pagesPerNode, 0}; 
                    //Size, amount and index start 
    //Last one for print porpouses

    #if DEBUG
        printf("Total of nodes %d\n", vector_total(&dsmNodes));
    #endif
    
    for (int i = 0; i < vector_total(&dsmNodes); i++) {
        int* socket = (int*) vector_get(&dsmNodes, i);
        //Sends the operation first
        send(*socket, &operation, sizeof(int), 0 );
        //Then sends the params for the setup
        send(*socket, params, sizeof(long) * 3, 0);
        //Add info to the table
        params[2] += pageSize; //Modifies de index of start

        #if DEBUG
            printf("Enviado a %d\n", *socket);
        #endif
    }

    int frameCounter = 0;
    long limit = pagesPerNode;
    for (long pageNumber = 0; pageNumber < totalPages; pageNumber++){
        if (pageNumber == limit){
            frameCounter++;
            limit += pagesPerNode;
        }
        
        page_ref* newPage = malloc(sizeof(page_ref));
        newPage->frame = frameCounter;
        newPage->pageNumber = pageNumber;
        newPage->validationBit = 0;
        vector_add(&tablePages , newPage);
    }
}


void menu(){
    //Menu oara funcionar mientras se tenga todo en accion
    //Opcion de salida, lo normal

    int input;
    while(1){
        sleep(1);
        printf("Menu del servidor\n");
        printf("1) Ver estadisticas\n");
        printf("2) Ver contenido\n");
        printf("3) Ejecutar pruebas\n");
        printf("4) Ver contenidos\n");

        scanf("%d", &input);
        
    }
}



/* Main
 * Triggers the start of the DSM. 
 * Initilize: vectors, threads, semaphores and global variables.
 * After that, runs the setup of the nodes and runs the menu.
*/
int main(){
    //Inicializamos el vector que funcionara como nuestra tabla de paginas
    vector_init(&tablePages);
    vector_init(&dsmNodes); //Y el listado de nodos
    pthread_t listener_thread;

    #if DEBUG
        printf("Hey\n");
    #endif

    
    printf("DSM version simple 0.1 \n");
    printf("Inicie los nodos en este momento.\n");
    printf("Al terminar de ingresar los nodos ");
    printf("ingrese 'y' para iniciar o 'a' para usar los datos default.\n");
    if( pthread_create( &listener_thread , NULL ,  listen_requests , 0 ) < 0) {
        printf("ERROR: No se pudo crear el hilo\n");
        return 1;
	}

    char input = 'n';
    scanf("%c%*c", &input);
    
    //We give a chance to add the nodes
    while (input != 'Y' && input != 'y' && input != 'A' && input != 'a'){
        sleep(5);
        printf("Opción invalida.\n");
        scanf("%c%*c", &input);
    }

    //By this point we setup the nodes, so we won't accept more
    aceptingNodes = 0;
    if (input == 'Y' || input == 'y' ){
        configure();
    } else {
        pageSize = 2 * 1024;//2Kb por pagina
        totalPages = 2000;  //2000 paginas en total
        setupNodes();
    }

    menu();
}