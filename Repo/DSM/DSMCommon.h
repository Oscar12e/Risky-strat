#ifndef DSMCOMMON_H
#define DSMCOMMON_H
#include "vector.h"

#define PORT 4000
#define ALLOC 1     //Alloc data
#define READ 2      //Read data
#define OVERWRITE 3 //Overwrite data

#define ADD_NODE 4          //Add node to the server
#define REGISTER_CLIENT 5   //Register node to the server

#define DISPLAY 11   //Display content
#define SET_PAGES 12 //Configure node

//In case we need to knoe if there was an error in the operation
#define ERROR -2
#define OK -1


//Also we will have the common structures
typedef struct var_ref {
    long pageNum;   //Page number
    size_t offset;  //Internal offset
    size_t size;    //Size reserved
} var_ref;


#endif