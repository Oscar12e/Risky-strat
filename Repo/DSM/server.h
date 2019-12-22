#include "vector.h"
#include "md5.h"
//STRUCTS
struct VideoInfo {
  char  name[50];
  char  title[100];
  char  desc[600];
  char  genre[50];
  char videoBytes[50];
};
struct client {
	char ipAddress[30];
	int requestsMade;
  char requestsList[1000];
};

struct FileMD5Info{
  char filename[50];
  char md5_string[33];
};

//Funciones
void* connection_handler(void *);
void createIndex ();
void* update_index();
bool updateRequired_index();
bool checkMD5(char* md5_path, int fileType);
void fillVector_md5(char* dirPath, int fileType);
void md5_file(char *filename, char *filepath, int fileType);
void MDPrint (MD5_CTX* mdContext, char* md5_string);
char* createCarrouselItem (struct VideoInfo* myVideoInfo, bool isActive);
char* createCarrouselIndicator (char* index, bool isActive);
struct VideoInfo* new_videoInfo(char* fileName);
void getFileNames();
bool readFileInfo(char* filepath, char* name, char* title, char* desc, char* genre);
void removeIndx(char* token, int idxToDel);
//log y MENU
void updateAvgSpeed(long pSpeedSample);
void updateTransferredBytes(long pTransferred);
struct client* new_client(char* _ipAddress);
struct client* getClient(char* _ipAddress);
void addToLog(char* entry);
void* menu();
