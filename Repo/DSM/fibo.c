#include <stdio.h> 
#include <stdlib.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include "DSMClient.h"

long fibo(long n){
    if (n <= 1) return n;
    return fibo(n-1) + fibo(n-2);
}

int main(){
    init();
    fibo(50);
}

