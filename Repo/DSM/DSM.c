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

void handleClient(int clientSock){
    int *in = malloc(sizeof(int));

    int valread;

    do {
        valread = read(clientSock, in, sizeof(int));
        
        sem_wait(&sem_peticiones);
        switch (*in){
        case ALLOC:
            /* code */
            
            break;
        case OVERWRITE:
            /* code */
            break;
        case READ:
            /* code */
            break;
        default:
            break;
        }

        sem_post(&sem_peticiones);

    } while (valread > 0);
 

}

/* handleRequest
 * @param request: a packet with the data of the request
*/
void handleRequest(int operation){
    sem_wait(&sem_peticiones);
    
    switch (operation){
        case ALLOC:
            //I need the client page, check if it has space
            //If client has not page, then alloc him one
            break;
        case READ:
            break;
        case OVERWRITE:
            break;
        default:
            break;
    }    

    sem_post(&sem_peticiones);
}


//Nodes code
int addNode(int sender){
    if (containsNode(sender) == 1) return 0;

    int* socket = malloc(sizeof(int));
    *socket = sender;
    vector_add(&dsmNodes, socket);
    printf("\nNuevo nodo añadido: %d - %d\n", sender, *socket);     
}


//Clients code
int addClient(int sender){
    if (containsClient(sender) == 0) return 0;

    int* socket = malloc(sizeof(int));
    *socket = sender;
    vector_add(&dsmNodes, socket);
    #if DEBUG
        printf("\nNuevo nodo añadido: %d - %d\n", sender, *socket); 
    #endif    
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
    send(clientSock, freePage->pageNumber, sizeof( int ), 0);
    
    //Run the handler
    int *client = malloc(sizeof(int));
    *client = clientSock;
    pthread_t client_thread;
    if( pthread_create( &client_thread , NULL ,  handleClient , (void *) client ) < 0) {
        printf("ERROR: No se pudo crear el hilo\n");
        return 1;
	}
    handleClient();
}

void dsm_malloc(int* client){
    //First, tell me the page
    long* pageNum = malloc(sizeof(long));
    read(*client, pageNum, sizeof(long));

    size_t* size = malloc(sizeof(size_t));
    read(*client, size, sizeof(size_t));

    int frame = *pageNum / pagesPerNode;
    int* socket = vector_get(&dsmNodes, frame);
    int out = ALLOC;

    #if DEBUG
        printf("Solicitada la opción de Malloc para un tamaño de \n", size);
        printf("\nSe guarda en el frame %d\n", frame);
    #endif 

    //We send the operation we want to perfom
    send(*socket, &out, sizeof( int ), 0);
    long out[2] = {*pageNum, size};
    //Then we send the information to realiaze the operation
    send(*socket, out, sizeof(long)*2, 0);

    //And we read the result struct
    var_ref* in = malloc(sizeof(var_ref));
    read(*socket, in, sizeof(var_ref));
    send(*client, in, sizeof(var_ref), 0);
    
    /*
  

    //Debugin
    out = DISPLAY;
    send(*socket, &out, sizeof( int ), 0);

    out = OVERWRITE;
    send(*socket, &out, sizeof( int ), 0);


    send(*socket, in, sizeof( var_ref ), 0);

    int* test = malloc(sizeof(int));
    *test = 52;

    
    printf("Sobreescribiendo1.\n");
    send(*socket, test, sizeof( int ), 0);
    printf("Sobreescribiendo2.\n");

    //sleep(1);

    out = DISPLAY;
    printf("print1\n");
    send(*socket, &out, sizeof( int ), 0);
    printf("print2\n");


    //send(*socket, ALLOC, 0);
    //send(*socket, index, 0)
    //send(*socket, size, 0);
    //read(*socket, allocBuffer, 1024);
    //start, size
    */
}

void storeData(int variable, int* client){
    //Get the actual variable info
    //Use that info to access the page

    long* pageNum = malloc(sizeof(long));
    //First, tell me the page
    read(*client, pageNum, sizeof(long));

    int frame = *pageNum / pagesPerNode;
    int* node = vector_get(&dsmNodes, frame);
    int out = OVERWRITE;

    //Send the operation to the node
    send(*node, &out, sizeof( int ), 0);

    //Get the reference from the client
    dsm_var* in = malloc(sizeof(dsm_var));
    read(*client, &in, sizeof(var_ref));

    //We transfere the data to the node
    send(*node, in, sizeof( var_ref ), 0);

    //Get the data from the client
    void* dataBuffer = malloc(sizeof(in->size));
    read(*client, dataBuffer, in->size);
    
    //And then send the data to the node to overwrite the space in the node
    send(*socket, dataBuffer, in->size, 0);

    //read if ok or error
}


void* readStored(int* client){
    //Get variable data
    //Use the data to access the node
    //And access the 

    long* pageNum = malloc(sizeof(long));
    //First, tell me the page
    read(*client, pageNum, sizeof(long));

    int frame = *pageNum / pagesPerNode;
    int* node = vector_get(&dsmNodes, frame);
    int op = READ;

    //Send the operation to the node
    send(*node, &op, sizeof( int ), 0);

    //Get the reference from the client
    dsm_var* in = malloc(sizeof(dsm_var));
    read(*client, &in, sizeof(var_ref));

    //We transfere the reference to the node
    send(*node, in, sizeof( var_ref ), 0);

    //Get the data from the node
    void* dataBuffer = malloc(sizeof(in->size));
    read(*node, dataBuffer, in->size);
    
    //And then send the data to the client
    send(*client, dataBuffer, in->size, 0);

    //read if ok or error
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
            initializeClient(new_socket);
            //addClient(new_socket);
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
        printf("Enviado a %d\n", *socket);
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
    DSM_malloc(4);

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
    printf("Tamaños %d - %d - %d \n", sizeof(unsigned char), sizeof(void*), sizeof(int));
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