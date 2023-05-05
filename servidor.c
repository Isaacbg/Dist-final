//#include "estructuras.h"
#include "sll.h"
#include "lines.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> //for close
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <ifaddrs.h>


pthread_mutex_t mutex_mensaje;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje;

int tratar_mensaje(int *socket){
	printf("s> tratar_mensaje\n");
    int sc_client; //información del resultado de la operación

    pthread_mutex_lock(&mutex_mensaje);

	sc_client = *socket;
	
	/* ya se puede despertar al servidor*/
	mensaje_no_copiado = false;
	pthread_cond_signal(&cond_mensaje);
	pthread_mutex_unlock(&mutex_mensaje);

	char mensaje[256];
	
	if (readLine(sc_client, mensaje, 256) < 0){
		//intentamos comunicar el error al cliente
		close(sc_client);
		perror("Error al recibir el mensaje");
	}
	printf("s> mensaje recibido: %s\n", mensaje);


}
	
int main(int argc, char *argv[]){
	struct hostent *hp;
	//struct in_addr ip_servidor;
	struct sockaddr_in server_addr, client_addr;
	int sc_server, sc_client;
	socklen_t size;

	//check if the number of arguments is correct
	if (argc != 2){
		perror("Error: el número de argumentos es incorrecto \n");
	}
	//get the first argument and check if it is a number
	int port = atoi(argv[1]);
	
	if (port == 0){
		printf("Error en el puerto \n");
		return -1;
	}
	
	struct ifaddrs *ifaddr;
	int family, s;
    char ipLocal[NI_MAXHOST];
	if (getifaddrs(&ifaddr) == -1) {
		printf("error al obtener la ip local\n");
		return -1;
    }
	for(int i; i<4; i++){
		ifaddr = ifaddr->ifa_next;
	}
	if(getnameinfo(ifaddr->ifa_addr, sizeof(struct sockaddr_in),ipLocal, NI_MAXHOST, NULL, 0, NI_NUMERICHOST)!= 0){
		printf("error en getanmeinfo()\n");
		return -1;
	}

	printf("s> init server %s:%d\n",ipLocal, port);

	if ((sc_server = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Error al crear el socket del servidor");
	}

	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port); 
	server_addr.sin_addr.s_addr = INADDR_ANY; //luego la pasaremos por variable de entorno
	setsockopt(sc_server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	
	/*bind socket del servidor*/
	if(bind(sc_server,(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		close(sc_server);
		perror("Error al bindear el socket del servidor");
	}
	
	listen(sc_server, SOMAXCONN);

	pthread_attr_t t_attr;		// atributos de los threads 
   	pthread_t thid;

	//pthread_mutex_init(&mutex_mensaje, NULL);
	//pthread_cond_init(&cond_mensaje, NULL);
	pthread_attr_init(&t_attr);
	// atributos de los threads, threads independientes
	pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

	for(;;) {
		sc_client = accept(sc_server,(struct sockaddr *)&client_addr, &size);
		if (sc_client < 0){
			close(sc_client);
			perror("Error al aceptar el socket del cliente");
			break;
		}
		if (pthread_create(&thid, &t_attr, (void *)tratar_mensaje, (void *)&sc_client)== 0) {
				// se espera a que el thread copie el mensaje 
				pthread_mutex_lock(&mutex_mensaje);
				while (mensaje_no_copiado)
					pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
				mensaje_no_copiado = true;
				pthread_mutex_unlock(&mutex_mensaje);
	 	}
	} 		
	if (close(sc_server) == -1){
		perror("Error al cerrar el socket del servidor");
	}
	return 0;







	

}