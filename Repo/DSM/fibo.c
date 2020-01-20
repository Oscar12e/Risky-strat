#include <stdio.h> 
#include <stdlib.h>
#include "DSMClient.h"

typedef struct agent{
    int posX, posY;
    char flag;
}agent;

long fibo(long n){
    if (n <= 1) return n;
    return fibo(n-1) + fibo(n-2);
}

int main(){
    if (initConnection() == 0){
        printf("Darn int\n");
    }

    printf("Start \n");

    agent *a = malloc(sizeof(agent));
    a->posX = 12;
    a->posY = 15;
    a->flag = 'd';
    int aRef = dsm_malloc(sizeof(agent));
    dsm_overwrite(aRef, a);
    agent *b = dsm_read(aRef);

    printf("Agente:\n");
    printf("%d-%d %c", b->posX, b->posY, b->flag); 

    printf("Mensaje:\n");
    char msg[] = "Hola mundo";
    int msgRef = dsm_malloc(sizeof(char) * 11);
    dsm_overwrite(msgRef, msg);
    sleep(1);
    char *msgRead = (char *) dsm_read(msgRef);
    printf("%s\n", msg);
    printf("%s\n", msgRead);
    
    //fibo(50);

    float longNumbers[4] = {12.0, 87.0, 28.0};
    printf("%ld ", sizeof(longNumbers));
    int longNVar = dsm_malloc(sizeof(float) * 3);
    dsm_overwrite(longNVar, longNumbers);
    //sleep(1);
    float * readed = (float *) dsm_read(longNVar);    
    printf("Read\n");
    //sleep(1);
    printf("Readed %f\n",readed[0]);
    printf("Readed %f\n",readed[1]);
    printf("Readed %f\n",readed[2]);
    

    float longN[4] = {21.0, 837.0, 8.0, 164.0};
    longNumbers[0] = 0;
    longNumbers[2] = 345;

    int lnRef = dsm_malloc( sizeof(longN));
    dsm_overwrite(lnRef, longN);


    readed = (float *) dsm_read(lnRef);
    printf("Readw\n");
    //sleep(1);
    printf("Readed %f\n",readed[0]);
    printf("Readed %f\n",readed[1]);
    printf("Readed %f\n",readed[2]);
    printf("Readed %f\n",readed[3]);





    return 1;
}

