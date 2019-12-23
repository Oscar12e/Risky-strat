#include <stdio.h> 
#include <stdlib.h>
#include "DSMClient.h"

long fibo(long n){
    if (n <= 1) return n;
    return fibo(n-1) + fibo(n-2);
}

int main(){
    if (init() == 0){
        printf("Darn int\n");
    }
    fibo(50);
}

