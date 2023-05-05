#ifndef LINES
#define LINES
#include <unistd.h>

struct mensaje
{   
    char op;
    int clave;
    int clave2;
    char val1[256];
    int val2;
    double val3;

};

int sendMessage(int socket, char *buffer, int len);
int recvMessage(int socket, char *buffer, int len);
ssize_t readLine(int fd, void *buffer, size_t n);
int sendStruct(int socket, struct mensaje *mess);
int recvStruct(int socket, struct mensaje *mess);


#endif