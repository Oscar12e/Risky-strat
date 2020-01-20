//INCLUDES
#include <stdio.h>//Standard input-output
#include <stdlib.h>//Standard library
#include <unistd.h>//Standard symbolic constants and types
#include <math.h>//Math functionality

#include <stdbool.h>//Boolean type
#include <string.h>//String functionality

#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>//Handle signals during program's execution

#include <dirent.h>//Format of directory entries
#include <sys/stat.h>//Information about files attributes
#include <sys/types.h>

#include <sys/socket.h>//Internet protocol family
#include <arpa/inet.h>//Definitions for internet operations

#include <errno.h>

//#include <openssl/sha.h>//SHA256

#include "server.h"
#include "vector.c"
#include "md5.c"

//DEFINITIONS
#define BUFFER_SIZE_INDEX 60000
#define BUFFER_SIZE_VIDEO 1048576
#define BUFFER_SIZE_IMAGES 10240
#define IPSERVER "192.168.1.3"
#define LOCALHOST "127.0.0.1"
#define PUERTO 8080
#define JPEG 1
#define MP4 2
#define TXT 3

//Variables globales
vector fileInfoVector;
vector clientsVector;
vector MD5_JPEG_InfoVector;
vector MD5_MP4_InfoVector;
vector MD5_TXT_InfoVector;
//static int const JPEG = 1;
//static int const MP4 = 2;
//static int const TXT = 3;
//Variables para el menu
long transferredBytes = 0;
long averageSpeed = 0;
long speedSamples = 0;
struct client* mapClients;
int registeredClients = 0;
char current_ipAddress[30];
int threadsCreated = 0;

//SEMAFOROS PARA LOS HILOS
sem_t sem_ActualizarIndex;
sem_t sem_ActualizarVelocidad;
sem_t sem_ActualizarClientes;
sem_t sem_ActualizarBytes;
sem_t sem_ActualizarThreads;

/*
INFORMACION IMPORTANTE

HTTP/0.9 Y 1.0 Cierran la conexion despues de un request
HTTP/1.1 Puede reutilizar la conexion mas de una vez para requests nuevos

*/





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

void clearLog(){
  char entry = ' ';
  char* filename = "Log.txt"; //Wish I could make it a const
	FILE* file = fopen(filename, "w");
	fprintf(file, "%c", entry);
	fclose(file);

	printf("\nClearing de log \n");
}

void semaphoresInit(){
  sem_init(&sem_ActualizarIndex, 0, 1);
  sem_init(&sem_ActualizarVelocidad, 0, 1);
  sem_init(&sem_ActualizarClientes, 0, 1);
  sem_init(&sem_ActualizarBytes, 0, 1);
  sem_init(&sem_ActualizarThreads, 0, 1);  
}


int main(int argc , char *argv[]) {
  clearLog();
  semaphoresInit(); 

  vector_init(&clientsVector);

  pthread_t menu_thread;
  pthread_create(&menu_thread, NULL, menu, NULL);
  

  //Socket initialization
  int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
  int opt = 1;
	//Create socket file descriptor
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("ERROR: No se ha creado el socket\n");
	}

  // Forcefully attaching socket to the port 8080
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

  server.sin_family = AF_INET;
	//server.sin_addr.s_addr = inet_addr(IPSERVER); //Ip del servidor
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PUERTO); //Puerto del servidor

  //Bind the socket to the corresponding port
	if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		printf("ERROR: No se realizo el bind correspondiente\n");
		return 1;
	}
  if (listen(socket_desc, 3) < 0) {
    printf("ERROR: No se puede escuchar\n");
		return 1;
  }
  printf("El servidor ha iniciado correctamente en el puerto 8080\n");

  createIndex();

  pthread_t update_thread;
  pthread_create(&update_thread, NULL, update_index, NULL);

  while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char *str_ip = malloc(INET_ADDRSTRLEN);
		inet_ntop( AF_INET, &ipAddr, str_ip, INET_ADDRSTRLEN );

    time_t tiempo_conexion;
		struct tm tiempo_inicio_conexion; //Tiempo de inicio del  servidor

		tiempo_conexion = time(NULL);
 		tiempo_inicio_conexion = *localtime(&tiempo_conexion);

 		char datos_fecha_conexion[200];
 		memset(datos_fecha_conexion,0,200);

 		sprintf(datos_fecha_conexion,"Hora de inicio: %d-%d-%d %d:%d:%d", tiempo_inicio_conexion.tm_mday, tiempo_inicio_conexion.tm_mon + 1, tiempo_inicio_conexion.tm_year + 1900, tiempo_inicio_conexion.tm_hour, tiempo_inicio_conexion.tm_min, tiempo_inicio_conexion.tm_sec);

		char peticion_realizada[200];
		memset(peticion_realizada,0,200);
		strcat(peticion_realizada,"Se conectó el usuario ");
		strcat(peticion_realizada,str_ip);
		strcat(peticion_realizada," - ");
		strcat(peticion_realizada,datos_fecha_conexion);

		//printf("%s \n", peticion_realizada);
    addToLog("\n[Nueva conexion]: ");
    addToLog(peticion_realizada);
    
    struct client* newClient = new_client(str_ip);
    strcpy(current_ipAddress, str_ip);

    pthread_t sniffer_thread;
		new_sock = malloc(4);
    *new_sock = client_sock;

		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0) {
			printf("ERROR: No se pudo crear el hilo\n");
			return 1;
		} else {
      sem_wait(&sem_ActualizarThreads);
      threadsCreated++;
      sem_post(&sem_ActualizarThreads);
      
    }

  }
  printf("SALIDA\n");
  sem_destroy(&sem_ActualizarIndex);
  return 0;
}

void* connection_handler (void *socket_desc){
  int sock = *(int*)socket_desc;
	char client_message[2000];

  int numRead = recv(sock , client_message , 2000 , 0);
    if (numRead < 1){
	    if (numRead == 0){
	    	printf("ERROR: El cliente se desconectó.\n");
	    } else {
	    	printf("ERROR: El cliente no está leyendo. errno: %d\n", errno);
	        //perror("The client was not read from");
	    }
	    close(sock);
      return NULL;
    }

  FILE *fileptr;

	char *buffer;
	long filelen;

	//Aqui se obtiene lo que se pide en el request
	int largo_copia = 0;
	for(int i = 4; i < numRead; i++) {
		if(client_message[i] == ' ') {
			break;
		} else {
			largo_copia++;
		}
	}
	char salida[2000];

	strncpy(salida,client_message, largo_copia+4);

	salida[largo_copia+4] = '\0';

  printf("%s \n", salida);

	if( !strncmp(salida, "GET /favicon.ico\n", 16) ) {
		printf("Petición get de /favicon.ico");
    registerPetition("GET de /favicon.ico");
    addToLog("\n[Peticion] ");
    addToLog("get de /favicon.ico");
		fileptr = fopen("Web/Files_icons/favicon.ico", "rb");  // Open the file in binary mode
		char* fstr = "image/ico";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);


    while ((numRead = fread(salida, 1, 2000, fileptr)) > 0) {
        send(sock, salida, numRead, 0);
        updateTransferredBytes((long) numRead);
    }

    fclose(fileptr);
  }else if ( !strncmp(salida, "GET /\0", 6) ){
    char peticion_realizada[80];
		memset(peticion_realizada,0,80);
		strcat(peticion_realizada,"Peticion get de /index.html");
    registerPetition("GET de /index.html");

		printf("%s", peticion_realizada);
    addToLog("\n[Peticion] ");
    addToLog(peticion_realizada);

		fileptr = fopen("Web/Files_HTML/index.html", "rb");
		char* fstr = "text/html";
		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
		send(sock, salida, strlen(salida), 0);


    while ((numRead = fread(salida, 1, 2000, fileptr)) > 0){
        send(sock, salida, numRead, 0);
        updateTransferredBytes((long) numRead);
    }
    fclose(fileptr);
  }else if ( !strncmp(salida, "GET /watch", 10) ){
    if ( !strncmp(salida, "GET /watch/Gallery/Files_Video/", 31) ){
      registerPetition("GET de /watch");

      char peticion_realizada[106];
  		memset(peticion_realizada,0,106);
  		strcat(peticion_realizada,"Peticion get videos");

  		printf("%s", peticion_realizada);
      addToLog("\n[Peticion] ");
      addToLog(peticion_realizada);

      char *fileName= strtok(salida, " ");
      fileName = strtok(NULL, " ");
      //En filename se encuentra el nombre del archivo
      printf("%s", fileName);

      fileptr = fopen(fileName+7, "rb");//Tengo que eliminar /watch/
      if (fileptr == NULL) {
        //No encontre el video, cargo la unknown
        fileptr = fopen("Gallery/Files_Video/unknown.mp4", "rb");
      }
      char* fstr = "video/mp4";
  		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
  		send(sock, salida, strlen(salida), 0);


      while ((numRead = fread(salida, 1, 2000, fileptr)) > 0){
          send(sock, salida, numRead, 0);
          updateTransferredBytes((long) numRead);
      }
      fclose(fileptr);

    } else{
      char peticion_realizada[80];
  		memset(peticion_realizada,0,80);
  		strcat(peticion_realizada,"Peticion get de /watch");

  		printf("%s", peticion_realizada);
      addToLog("\n[Peticion] ");
      addToLog(peticion_realizada);

      //GET /watch/im_a_kitty_cat
      char* fileName = salida + 11;//Quiero solo el nombre
      struct VideoInfo* myVideoInfo = new_videoInfo(fileName);


  		fileptr = fopen("Web/Files_HTML/watchVideo.html", "rb");
  		char* fstr = "text/html";
  		sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
  		send(sock, salida, strlen(salida), 0);

      char* line = NULL;
      size_t salidaBuff = 2000;

      while ((numRead = getline(&line, &salidaBuff, fileptr)) != -1){
        if (!strncmp(line, "T", 1)) {
          //T    Title
          send(sock, myVideoInfo->title, strlen(myVideoInfo->title), 0);
        } else if (!strncmp(line, "S", 1)) {
          //S    filename
          send(sock, myVideoInfo->name, strlen(myVideoInfo->name), 0);
        } else if (!strncmp(line, "D", 1)) {
          //D    Description
          send(sock, myVideoInfo->desc, strlen(myVideoInfo->desc), 0);
        } else if (!strncmp(line, "B", 1)) {
          //B    Bytes
          printf("%s\n", myVideoInfo->videoBytes);
          send(sock, myVideoInfo->videoBytes, strlen(myVideoInfo->videoBytes), 0);
          updateTransferredBytes((long) strlen(myVideoInfo->videoBytes));
        } else{
          send(sock, line, numRead, 0);
          updateTransferredBytes((long) numRead);
        }

      }

      fclose(fileptr);
    }
  } else if ( !strncmp(salida, "GET /Gallery/Files_Image/", 25) ){

    char peticion_realizada[100];
    memset(peticion_realizada,0,100);
    strcat(peticion_realizada,"Peticion get image");

    printf("%s", peticion_realizada);
    addToLog("\n[Peticion] ");
    addToLog(peticion_realizada);

    //free(peticion_realizada);
    char *fileName= strtok(salida, " ");
    fileName = strtok(NULL, " ");
    fileptr = fopen(fileName+1, "rb");//Elimino el primer "/"
    if (fileptr == NULL) {
      //No encontre la imagen, cargo la unknown
      fileptr = fopen("Gallery/Files_Image/unknown.jpeg", "rb");
    }
    char* fstr = "image/jpeg";
    sprintf(salida, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",fstr);
    send(sock, salida, strlen(salida), 0);
    updateTransferredBytes((long) strlen(salida));
    while ((numRead = fread(salida, 1, 2000, fileptr)) > 0){
        send(sock, salida, numRead, 0);
        updateTransferredBytes((long) numRead);
    }
    fclose(fileptr);

  }

  free(socket_desc);
	close(sock);

  addToLog("\n[Desconexion]: Se desconectó el cliente\n");
	printf("Se desconectó el cliente\n");

	return 0;
}

void createIndex (){
  FILE* fp;
  FILE* fp_aux;
  char buffer [2000];
  fp = fopen("Web/Files_HTML/index.html", "w+");
  //Parte 0
  fp_aux = fopen("Web/Files_HTML/index_parts/index_part0.html", "r");
  while (!feof(fp_aux)) {
    fread(buffer, sizeof(buffer), 1, fp_aux);//Leo del archivo
    fputs(buffer, fp);//Se escribe en el archivo
    memset(buffer, 0, strlen(buffer));
  }
  fclose(fp_aux);

  //Carrousels
  int i;
  bool isActive = true;
  getFileNames();
  char* myItem;
  for (i = 0; i < vector_total(&fileInfoVector); i++){
    if (isActive) {
      fputs(createCarrouselItem((struct VideoInfo *) vector_get(&fileInfoVector, i), isActive), fp);//myItem
      isActive = false;
    }else{
      fputs(createCarrouselItem((struct VideoInfo *) vector_get(&fileInfoVector, i), isActive), fp);//myItem
    }

  }

  //Parte 1
  fp_aux = fopen("Web/Files_HTML/index_parts/index_part1.html", "r");
  while (!feof(fp_aux)) {
    fread(buffer, sizeof(buffer), 1, fp_aux);//Leo del archivo
    fputs(buffer, fp);//Se escribe en el archivo
    memset(buffer, 0, strlen(buffer));
  }
  fclose(fp_aux);
  //Indicadores uno por cada
  isActive = true;
  for (i = 0; i < vector_total(&fileInfoVector); i++)
      if (isActive) {
        fputs(createCarrouselIndicator("0", isActive), fp);
        isActive = false;
      }
      fputs(createCarrouselIndicator("1", isActive), fp);

  //Parte 2
  fp_aux = fopen("Web/Files_HTML/index_parts/index_part2.html", "r");
  while (!feof(fp_aux)) {
    fread(buffer, sizeof(buffer), 1, fp_aux);//Leo del archivo
    fputs(buffer, fp);//Se escribe en el archivo
    memset(buffer, 0, strlen(buffer));
  }
  fclose(fp_aux);

  fclose(fp);
}

void* update_index(){
  while (true) {

    if(updateRequired_index()){
      sem_wait(&sem_ActualizarIndex);
      //Seccion critica, se va a actualizar el index
      createIndex ();
      sem_post(&sem_ActualizarIndex);
    }

    sleep(20);//Cada 20 segundos se actualiza el index

  }
}

bool updateRequired_index(){
  //Se revisa entre todos los archivos si el sha generado coincide con el existente
  //Cargo los vectores
  fillVector_md5("Gallery/Files_Image", JPEG);
  fillVector_md5("Gallery/Files_Video", MP4);
  fillVector_md5("Gallery/Files_Info", TXT);
  //Reviso en todos los .jpeg
  checkMD5("Gallery/md5checks/md5_JPEG.txt", JPEG);
  //Reviso en todos los .txt
  checkMD5("Gallery/md5checks/md5_TXT.txt", TXT);
  //Reviso en todos los .mp4
  checkMD5("Gallery/md5checks/md5_MP4.txt", MP4);
  return false;
}

bool checkMD5(char* md5_path, int fileType){
  FILE* fp;
  char buffer[2000];
  printf("%s\n", md5_path);
  //Si el archivo ya existe entonces comparo
  if (access(md5_path, R_OK) != -1) {
    //El archivo existe y se puede leer, voy a comparar
    printf("Checking file\n");
    fp = fopen(md5_path, "r");
    char* line = NULL;
    size_t salidaBuff = 2000;
    int numRead;
    char* fileName;
    char* md5_string;
    bool isChanged = false;
    bool found;
    int i;
    while ((numRead = getline(&line, &salidaBuff, fp)) != -1){
      fileName = strtok(line, "=");
      md5_string = strtok(NULL, "=");
      md5_string = strtok(md5_string, "\n");
      //Busco en el vector el valor correspondiente
      found = false;
      switch (fileType) {
        case JPEG:
          for (i = 0; i < vector_total(&MD5_JPEG_InfoVector); i++){
            struct FileMD5Info* fileMD5Info = vector_get(&MD5_JPEG_InfoVector, i);
            if (!strcmp(fileMD5Info->filename, fileName)) {
              found = true;
              if (strcmp(fileMD5Info->md5_string, md5_string)) {
                //Son diferentes entonces hay que refrescar
                printf("Hay que refrescar\n");
                //Se tiene que volver a hacer el archivo correspondiente

                //Se tiene que volver a hacer el index
                return true;
              }
            }
          }
          break;
        case MP4:
          for (i = 0; i < vector_total(&MD5_MP4_InfoVector); i++){
            struct FileMD5Info* fileMD5Info = vector_get(&MD5_MP4_InfoVector, i);
            if (!strcmp(fileMD5Info->filename, fileName)) {
              found = true;
              if (strcmp(fileMD5Info->md5_string, md5_string)) {
                //Son diferentes entonces hay que refrescar
                printf("Hay que refrescar\n");
                //Se tiene que volver a hacer el archivo correspondiente

                //Se tiene que volver a hacer el index
                return true;
              }
            }
          }
          break;
        case TXT:
          for (i = 0; i < vector_total(&MD5_TXT_InfoVector); i++){
            struct FileMD5Info* fileMD5Info = vector_get(&MD5_TXT_InfoVector, i);
            if (!strcmp(fileMD5Info->filename, fileName)) {
              found = true;
              if (strcmp(fileMD5Info->md5_string, md5_string)) {
                //Son diferentes entonces hay que refrescar
                printf("Hay que refrescar\n");
                //Se tiene que volver a hacer el archivo correspondiente

                //Se tiene que volver a hacer el index
                return true;
              }
            }
          }
          break;
      }

      if (!found) {
        printf("No se encontro el archivo %s\n", fileName);
      }
      //Reseteo
      free(line);
      line = NULL;
    }
    fclose(fp);
    printf("Todo en orden\n");
  } else {
    //Debo crear el archivo
    printf("Creating file\n");
    fp = fopen(md5_path, "w+");
    char buffer[100];
    memset(buffer, 0, 100);
    int i;
    switch (fileType) {
      case JPEG:
        for (i = 0; i < vector_total(&MD5_JPEG_InfoVector); i++){
          struct FileMD5Info* fileMD5Info = vector_get(&MD5_JPEG_InfoVector, i);
          strcat(buffer, fileMD5Info->filename);
          strcat(buffer, "=");
          strcat(buffer, fileMD5Info->md5_string);
          strcat(buffer, "\n");
          fputs(buffer, fp);//Se escribe en el archivo
          memset(buffer, 0, 100);
        }
        break;
      case MP4:
        for (i = 0; i < vector_total(&MD5_MP4_InfoVector); i++){
          struct FileMD5Info* fileMD5Info = vector_get(&MD5_MP4_InfoVector, i);
          strcat(buffer, fileMD5Info->filename);
          strcat(buffer, "=");
          strcat(buffer, fileMD5Info->md5_string);
          strcat(buffer, "\n");
          fputs(buffer, fp);//Se escribe en el archivo
          memset(buffer, 0, 100);
        }
        break;
      case TXT:
        for (i = 0; i < vector_total(&MD5_TXT_InfoVector); i++){
          struct FileMD5Info* fileMD5Info = vector_get(&MD5_TXT_InfoVector, i);
          strcat(buffer, fileMD5Info->filename);
          strcat(buffer, "=");
          strcat(buffer, fileMD5Info->md5_string);
          strcat(buffer, "\n");
          fputs(buffer, fp);//Se escribe en el archivo
          memset(buffer, 0, 100);
        }
        break;
    }
    fclose(fp);
  }
  return false;
}

void fillVector_md5(char* dirPath, int fileType){
  switch (fileType) {
    case JPEG:
      vector_init(&MD5_JPEG_InfoVector);
      break;
    case MP4:
      vector_init(&MD5_MP4_InfoVector);
      break;
    case TXT:
      vector_init(&MD5_TXT_InfoVector);
      break;
  }

  DIR* dirp;
  struct dirent * entry;
  char* token;
  dirp = opendir(dirPath); // 19
  char filepath[119];

  while ((entry = readdir(dirp)) != NULL) {
      if (entry->d_type == DT_REG) {  //If the entry is a regular file
        memset(filepath, 0, 119);
        strcpy(filepath, dirPath);
        strcat(filepath, "/");
        strcat(filepath, entry->d_name);
        md5_file(entry->d_name, filepath, fileType);
      }
  }
  closedir(dirp);
}

//Open a file and generate a MD5
void md5_file(char *filename, char *filepath, int fileType){
  FILE *inFile = fopen (filepath, "rb");
  MD5_CTX mdContext;
  struct FileMD5Info* fileMD5Info = (struct FileMD5Info*)malloc(sizeof(struct FileMD5Info));

  int bytes;
  unsigned char data[1024];

  if (inFile == NULL) {
    printf ("%s can't be opened.\n", filepath);
    return;
  }

  MD5Init (&mdContext);
  while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    MD5Update (&mdContext, data, bytes);
  MD5Final (&mdContext);
  //Guardo en el vector
  strcpy(fileMD5Info->filename, filename);
  char md5_string [33];
  MDPrint (&mdContext, md5_string);
  strcpy(fileMD5Info->md5_string, md5_string);

  fclose (inFile);
  switch (fileType) {
    case JPEG:
      vector_add(&MD5_JPEG_InfoVector, fileMD5Info);
      break;
    case MP4:
      vector_add(&MD5_MP4_InfoVector, fileMD5Info);
      break;
    case TXT:
      vector_add(&MD5_TXT_InfoVector, fileMD5Info);
      break;
  }

}

void MDPrint (MD5_CTX* mdContext, char* md5_string){
  int i;
  char buffer[3];
  memset(md5_string, 0, 33);
  for (i = 0; i < 16; i++){
    sprintf (buffer, "%02X", mdContext->digest[i]);
    strcat(md5_string, buffer);
  }
}

void getFileNames(){
  vector_init(&fileInfoVector);
  DIR* dirp;
  struct dirent * entry;
  char* token;
  dirp = opendir("Gallery/Files_Info"); /* There should be error handling after this */
  while ((entry = readdir(dirp)) != NULL) {
      if (entry->d_type == DT_REG) { /* If the entry is a regular file */
        token = strtok(entry->d_name, ".");
        vector_add(&fileInfoVector, new_videoInfo(token));
      }
  }
  closedir(dirp);
}

char* createCarrouselItem (struct VideoInfo* myVideoInfo, bool isActive){
  /*
  <div class="carousel-item active"><img class="w-100 d-block" src="Gallery/Files_Image/im_a_kitty_cat.jpeg" alt="Slide Image">
      <div class="carousel-caption">
          <h3><a href="/watch">I am a kitty cat</a></h3>
          <p>Description</p>
      </div>
  </div>
  */

  char* carrouselItmStr;
  char* part0_0 = "<div class=\"carousel-item active\"><img class=\"w-100 d-block\"src=\"Gallery/Files_Image/";
  char* part0_1 = "<div class=\"carousel-item\"><img class=\"w-100 d-block\"src=\"Gallery/Files_Image/";
  //HERE: filename
  char* part1 = ".jpeg\" alt=\"Slide Image\"> \n <div class=\"carousel-caption\">\n<h3><a href=\"/watch/";
  //HERE: Route to navigate
  char* part2 = "\">";
  //HERE: Title
  char* part3 = "</a></h3>\n<p>";
  //HERE: Description
  char* part4 = "</p>\n</div>\n</div>";
  //struct VideoInfo* myVideoInfo = new_videoInfo(fileName);
  //Allocate memory
  if (isActive) {
    carrouselItmStr = malloc(strlen(part0_0) + strlen(part1) + strlen(part2) + strlen(part3) + strlen(part4) + strlen(myVideoInfo->name) + strlen(myVideoInfo->name) + strlen(myVideoInfo->title) + strlen(myVideoInfo->desc));
    memset(carrouselItmStr, 0, strlen(carrouselItmStr));//Me aseguro de que se interprete como un string vacio
    strcat(carrouselItmStr, part0_0);
  } else {
    carrouselItmStr = malloc(strlen(part0_0) + strlen(part1) + strlen(part2) + strlen(part3) + strlen(part4) + strlen(myVideoInfo->name) + strlen(myVideoInfo->name) + strlen(myVideoInfo->title) + strlen(myVideoInfo->desc));
    memset(carrouselItmStr, 0, strlen(carrouselItmStr));//Me aseguro de que se interprete como un string vacio
    strcat(carrouselItmStr, part0_1);
  }

  strcat(carrouselItmStr, myVideoInfo->name);
  strcat(carrouselItmStr, part1);
  strcat(carrouselItmStr, myVideoInfo->name);
  strcat(carrouselItmStr, part2);
  strcat(carrouselItmStr, myVideoInfo->title);
  strcat(carrouselItmStr, part3);
  strcat(carrouselItmStr, myVideoInfo->desc);
  strcat(carrouselItmStr, part4);
  return carrouselItmStr;
}

char* createCarrouselIndicator (char* index, bool isActive){
  char* myItem;
  char* part0 = "<li data-target=\"#carousel-1\" data-slide-to=\"";
  //HERE: index
  char* part1_0 = "\" class=\"active\"></li>";
  char* part1_1 = "\"></li>";

  //Allocate memory
  if (isActive) {
    myItem = malloc(strlen(part0) + strlen(part1_0) + strlen(index));
    memset(myItem, 0, strlen(myItem));//Me aseguro de que se interprete como un string vacio
    strcat(myItem, part0);
    strcat(myItem, index);
    strcat(myItem, part1_0);
  } else {
    myItem = malloc(strlen(part0) + strlen(part1_1) + strlen(index));
    memset(myItem, 0, strlen(myItem));//Me aseguro de que se interprete como un string vacio
    strcat(myItem, part0);
    strcat(myItem, index);
    strcat(myItem, part1_1);
  }


  return myItem;
}

struct VideoInfo* new_videoInfo(char* fileName) {
  //I will get the file info
  char filepath[119];
  memset(filepath,0,119);
  strcat(filepath,"Gallery/Files_Info/");
  strcat(filepath,fileName);
  strcat(filepath,".txt");

  struct VideoInfo* myVideoInfo = (struct VideoInfo*)malloc(sizeof(struct VideoInfo));
  char name[119];
  memset(name,0,119);

  char title[119];
  memset(title,0,119);

  char desc[2000];
  memset(desc,0,2000);

  char genre[119];
  memset(genre,0,119);

  //Get video info metadata
  if (!readFileInfo(filepath, name, title, desc, genre)) {
    strcpy(myVideoInfo->name, "unknown");
    strcpy(myVideoInfo->title, "UNKNOWN");
    strcpy(myVideoInfo->desc, "Video Info not found: Enjoy this music clip instead...");
    strcpy(myVideoInfo->genre, "NONE");
    strcpy(myVideoInfo->videoBytes, " ");
  }else{
    //Se encuentra el archivo con los datos del archivo
    strcpy(myVideoInfo->name, name);
    strcpy(myVideoInfo->title, title);
    strcpy(myVideoInfo->desc, desc);
    strcpy(myVideoInfo->genre, genre);

    //Get video info
    memset(filepath,0,119);
    strcat(filepath,"Gallery/Files_Video/");
    strcat(filepath,fileName);
    strcat(filepath,".mp4");

    struct stat attrib;
    if (stat(filepath, &attrib) != -1) {
      int enough = (int)((ceil(log10(attrib.st_size))+1) * sizeof(char));
      char videoBytes[enough];
      sprintf(videoBytes, "%lu", attrib.st_size);

      strcpy(myVideoInfo->videoBytes, videoBytes);
    } else{
      //No encontre el video
      strcpy(myVideoInfo->name, "unknown");
      strcpy(myVideoInfo->title, "UNKNOWN");
      strcpy(myVideoInfo->desc, "Video Info not found: Enjoy this music clip instead...");
      strcpy(myVideoInfo->genre, "NONE");
      strcpy(myVideoInfo->videoBytes, " ");
    }
    /*
    if ((&attrib.st_mtime)!=(&attrib.st_atime)) {
      printf("b\n");
    }else{
      printf("a\n");
    }*/

  }
  return myVideoInfo;
}

bool readFileInfo(char* filepath, char* name, char* title, char* desc, char* genre){

  FILE *file = fopen ( filepath, "r" );
  char *search = "=";
  char *token;
  if ( file != NULL ){
    char* line = NULL;
    size_t salidaBuff = 2000;
    int numRead;
    while ((numRead = getline(&line, &salidaBuff, file)) != -1){
      // Agarro el tipo de token
      token = strtok(line, "=");
      //Identifico el tipo y lo guardo donde corresponde
      if (strcmp(token, "name") == 0){
        token = strtok(NULL, search);
        int idxToDel = strlen(token) - 1;
        memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
        strcpy(name, token);
      } else if (strcmp(token, "title") == 0) {
        token = strtok(NULL, search);
        int idxToDel = strlen(token) - 1;
        memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
        strcpy(title, token);
      } else if (strcmp(token, "description") == 0){
        token = strtok(NULL, search);
        int idxToDel = strlen(token) - 1;
        memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
        strcpy(desc, token);
      } else if (strcmp(token, "genre") == 0){
        token = strtok(NULL, search);
        int idxToDel = strlen(token) - 1;
        memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
        strcpy(genre, token);
      }
      free(line);
      line = NULL;
    }


    salidaBuff = 0;
    fclose ( file );
    return true;
  } else {
    return false;
  }
}

void listPetitions(){

}

void* menu(){
  char option;

  while (true){
    printf("\nMenú para servidor\n");
    printf("1. Listar peticiones\n"); //ignorar
    printf("2. Hora de inicio \n"); //Busqueda en el log [busca cada 'Inicio de conexión' en el log]
    printf("3. Cantidad de byes tranferidos \n"); //Consulta a la variable gloabl
    printf("4. Velocidad promedio \n"); //Consutla a la variable global
    printf("5. Cantidad de clientes \n"); //Consulta a variable gloabl
    printf("6. Cantidad de solicitudes \n"); //Consulta a variable gloabl
    printf("7. Threads creados \n"); //Consulta a variable global

    scanf("%c%*c", &option);

    if (option == '1'){
      //Call listar peticiones
      sem_wait(&sem_ActualizarClientes);

      struct client* aux = NULL;
      int total = 0;

      printf("\n Lista de peticiones: \n", total);
      for (int i = 0; i < registeredClients; i++){
        aux = vector_get(&clientsVector, i);
        printf("%s", aux->requestsList);
      }

      sem_post(&sem_ActualizarClientes);

    } else if (option == '2'){
      //Call horas de inicio
    } else if (option == '3'){
      //Call velocidad
      printTransferredBytes();
    } else if (option == '4'){
      //Call bytes de inicio
    } else if (option == '5'){
      sem_wait(&sem_ActualizarClientes);
      printf("\n Se han registrad %d clientes.\n", registeredClients);
      sem_post(&sem_ActualizarClientes);
      //Call cant de solicitudes
    }else if (option == '6'){
      //Call cant de solicitudes
      sem_wait(&sem_ActualizarClientes);

      struct client* aux = NULL;
      int total = 0;

      for (int i = 0; i < registeredClients; i++){
        aux = vector_get(&clientsVector, i);
        total += aux->requestsMade;
      }
  
      printf("\n Se han registrado %d solicitudes.\n", total);
      sem_post(&sem_ActualizarClientes);
    }  else if (option == '7'){
      //Call threads creados
      sem_wait(&sem_ActualizarThreads);
      printf("\n Se han creado %d threads.\n", threadsCreated);
      sem_post(&sem_ActualizarThreads);
    } else {
      printf("Opción de ");
    }
  }

	
}

/* Esto es un recordatorio de lo terrible que puede ser el manejo de memoria en c >:'c
while ((numRead = getline(&line, &salidaBuff, fileptr)) != -1){
  if (!strncmp(line, "T", 1)) {
    //T    Title
    send(sock, myVideoInfo->title, strlen(myVideoInfo->title), 0);
  } else if (!strncmp(line, "S", 1)) {
    //S    filename
    send(sock, myVideoInfo->name, strlen(myVideoInfo->name), 0);
  } else if (!strncmp(line, "D", 1)) {
    //D    Description
    send(sock, myVideoInfo->desc, strlen(myVideoInfo->desc), 0);
  } else{
    send(sock, line, numRead, 0);
  }

}

char line [ 1000 ];
while ( fgets ( line, sizeof line, file ) != NULL ){
  printf("\n=================LINE=================\n");
  printf("\n%s\n", line);
  printf("\n=================LINE=================\n");
  // Agarro el tipo de token
  token = strtok(line, "=");
  //Identifico el tipo y lo guardo donde corresponde
  if (strcmp(token, "name") == 0){
    token = strtok(NULL, search);
    int idxToDel = strlen(token) - 1;
    memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
  } else if (strcmp(token, "title") == 0) {
    token = strtok(NULL, search);
    int idxToDel = strlen(token) - 1;
    memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
  } else if (strcmp(token, "description") == 0){
    token = strtok(NULL, search);
    int idxToDel = strlen(token) - 1;
    memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
  } else if (strcmp(token, "genre") == 0){
    token = strtok(NULL, search);
    int idxToDel = strlen(token) - 1;
    memmove(&token[idxToDel], &token[idxToDel + 1], strlen(token) - idxToDel);
  }
}
*/
