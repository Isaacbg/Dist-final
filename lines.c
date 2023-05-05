#include <unistd.h>
#include <errno.h>
#include "lines.h"
//#include "estructuras.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int sendMessage(int socket, char * buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = write(socket, buffer, l);
		l = l -r;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* full length has been sent */
}

int recvMessage(int socket, char *buffer, int len)
{
	int r;
	int l = len;
		

	do {	
		r = read(socket, buffer, l);
		l = l -r ;
		buffer = buffer + r;
	} while ((l>0) && (r>=0));
	
	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* full length has been receive */
}


ssize_t readLine(int fd, void *buffer, size_t n)
{
	ssize_t numRead;  /* num of bytes fetched by last read() */
	size_t totRead;	  /* total bytes read so far */
	char *buf;
	char ch;


	if (n <= 0 || buffer == NULL) { 
		errno = EINVAL;
		return -1; 
	}

	buf = buffer;
	totRead = 0;
	
	for (;;) {
        	numRead = read(fd, &ch, 1);	/* read a byte */

        	if (numRead == -1) {	
            		if (errno == EINTR)	/* interrupted -> restart read() */
                		continue;
            	else
			return -1;		/* some other error */
        	} else if (numRead == 0) {	/* EOF */
            		if (totRead == 0)	/* no byres read; return 0 */
                		return 0;
			else
                		break;
        	} else {			/* numRead must be 1 if we get here*/
            		if (ch == '\n')
                		break;
            		if (ch == '\0')
                		break;
            		if (totRead < n - 1) {		/* discard > (n-1) bytes */
				totRead++;
				*buf++ = ch; 
			}
		} 
	}
	
	*buf = '\0';
    	return totRead;
}


int sendStruct(int socket, struct mensaje *mess)
{	
	__int32_t key, key2, val2, op;
	//send op code
	op = htonl(mess->op);
	if (write(socket, &op, 4) < 0){
		return -1;
	}
	//sendMessage(socket, &op, sizeof(char));
	//send key

	key = htonl(mess->clave);
	if (write(socket, &key, 4) < 0){
		return -1;
	}
	//sendMessage(socket, &key, sizeof(int));
	//send key2
	key2 = htonl(mess->clave2);
	if (write(socket, &key2, 4) < 0){
		return -1;
	}
	//sendMessage(socket, &key2, sizeof(int));
	//send val1
	if (sendMessage(socket, mess->val1, sizeof(char)*256) < 0){
		return -1;
	}
	//send val2
	val2 = htonl(mess->val2);
	if (write(socket, &val2, 4) < 0){
		return -1;
	}
	//sendMessage(socket, &val2, sizeof(int));

	char val3[8];	
	// printf("Enviando double: %f\n", mess->val3);
	memcpy(val3, &mess->val3, sizeof(char)*8);
	if (sendMessage(socket, val3, sizeof(char)*8) < 0){
		return -1;
	}
	return 0;


}

int recvStruct(int socket, struct mensaje *mess)
{
	u_int32_t key, key2, val2, op;
	//recv op code
	//recvMessage(socket, &op, sizeof(char));
	if (read(socket, &op, 4) < 0){
		return -1;
	}
	mess->op = ntohl(op);
	//recv key
	//recvMessage(socket, key, sizeof(int));
	if (read(socket, &key, 4) < 0){
		return -1;
	}
	mess->clave = ntohl(key);
	//recv key2
	//recvMessage(socket, key2, sizeof(int));
	if (read(socket, &key2, 4) < 0){
		return -1;
	}
	mess->clave2 = ntohl(key2);
	//recv val1

	if (recvMessage(socket, mess->val1, sizeof(char)*256) < 0){
		return -1;
	}
	//recv val2
	//recvMessage(socket, val2, sizeof(int));
	if (read(socket, &val2, 4) < 0){
		return -1;
	}
	mess->val2 = ntohl(val2);
	//recv val3
	char val3[8];

	if (recvMessage(socket, val3, sizeof(char)*8) < 0){
		return -1;
	}
	memcpy(&mess->val3, val3,  sizeof(char)*8);
	return 0;
}
