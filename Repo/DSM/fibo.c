#include <stdio.h> 
#include <stdlib.h>
#include "DSMClient.h"

long fibo(long n){
    if (n <= 1) return n;
    return fibo(n-1) + fibo(n-2);
}

int main(){
    if (initConnection() == 0){
        printf("Darn int\n");
    }

    printf("Start \n");

    int longNumbers[] = {1, 2, 4, 5};
    printf("Malloc\n");
    int longNVar = dsm_malloc(sizeof(int) * 4);
    printf("Malloc done\n");
    dsm_overwrite(longNVar, longNumbers);
    printf("Write done\n");
    printf("Read start\n");
    sleep(1);
    printf("Read start2\n");
    int * readed = dsm_read(longNVar);
    printf("read done");
    printf("REad\n");
    sleep(1);
    printf("%d\n",readed[2]);

    //fibo(50);

    return 1;
}

