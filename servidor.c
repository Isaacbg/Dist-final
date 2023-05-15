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

struct UserList *lista_usuarios;

void enviar_mensaje(struct usuario *user ,struct mensaje *msg){
	struct sockaddr_in client_listen;
	int sc_client;
	printf("Enviando mensaje a %s:%d\n", inet_ntoa(user->ip), user->puerto);
	//conectamos con el cliente
	if ((sc_client = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error al crear el socket");
		exit(1);
	}
	client_listen.sin_family = AF_INET;
	client_listen.sin_port = htons(user->puerto);
	client_listen.sin_addr = user->ip;

	if (connect(sc_client, (struct sockaddr *)&client_listen, sizeof(client_listen)) < 0){
		perror("Error al conectar con el cliente");
		exit(1);
	}
	//enviamos el mensaje
	sendMessage(sc_client, msg->remite, 256);
	uint32_t id = htonl(msg->id);
	write(sc_client, &id, 4);
	sendMessage(sc_client, msg->mensaje, 256);

	user->last_message = msg->id; //!

	//close(sc_client);
};


int tratar_mensaje(int *socket){
    int sc_client; //información del resultado de la operación

    pthread_mutex_lock(&mutex_mensaje);

	sc_client = *socket;
	
	/* ya se puede despertar al servidor*/
	mensaje_no_copiado = false;
	pthread_cond_signal(&cond_mensaje);
	pthread_mutex_unlock(&mutex_mensaje);

	char operacion[256];  //se puede reducir a la len del op mas largo
	
	if (readLine(sc_client, operacion, 256) < 0){
		//intentamos comunicar el error al cliente
		close(sc_client);
		perror("Error al recibir el mensaje");
	}
	if (strcmp(operacion, "REGISTER") == 0){
		char nombre[256];
		if (readLine(sc_client, nombre, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		char fecha[11];
		if (readLine(sc_client, fecha, 11) < 0){
			perror("Error al recibir el mensaje");
		}
		/*Check if client data is correct*/
		
		/**/
		if (searchUser(lista_usuarios, alias) != NULL){
			//alias already exists
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> REGISTER %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		//alias does not exist
		struct usuario nuevo_usuario;
		strcpy(nuevo_usuario.nombre, nombre);
		strcpy(nuevo_usuario.alias, alias);
		strcpy(nuevo_usuario.fecha, fecha);
		strcpy(nuevo_usuario.estado, "DISCONNECTED\0");
		//nuevo_usuario.ip = 0;
		nuevo_usuario.puerto = 0;
		nuevo_usuario.last_message = 0;

		//sección crítica
		addUser(lista_usuarios, nuevo_usuario);
		//end sección crítica
		u_int16_t response;
		response = htons(0);
		if (write(sc_client, &response, 2) < 0){
			perror("Error al enviar el mensaje");
			
		}
		printf("s> REGISTER %s OK\n", alias);

		close(sc_client);
		return 0;
	}	
	else if (strcmp(operacion, "UNREGISTER") == 0){
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		/*Check if client data is correct*/
		/**/
		if (searchUser(lista_usuarios, alias) == NULL){
			//alias does not exist
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> UNREGISTER %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		//alias exists
		//sección crítica
		removeUserNode(lista_usuarios, alias);
		//end sección crítica
		u_int16_t response;
		response = htons(0);
		if (write(sc_client, &response, 2) < 0){
			perror("Error al enviar el mensaje");
		}
		printf("s> UNREGISTER %s OK\n", alias);
		close(sc_client);
		return 0;
	}

	else if (strcmp(operacion, "CONNECT")== 0){
		
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		char port[7];
		if (readLine(sc_client, port, 7) < 0){
			perror("Error al recibir el mensaje");
		}
		/*Check if client data is correct*/
		/**/
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (user == NULL){
			//alias does not exist
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> CONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		//alias exists
		//sección crítica
		if (strcmp(user->data.estado, "CONNECTED") == 0){
			u_int16_t response;
			response = htons(2);
			write(sc_client, &response, 2);
			printf("s> CONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		strcpy(user->data.estado, "CONNECTED");
		user->data.puerto = atoi(port);
		struct sockaddr_in addr;
		socklen_t addr_size = sizeof(struct sockaddr_in);
		if (getpeername(sc_client, (struct sockaddr *)&addr, &addr_size) <0){
			perror("Error al obtener la ip del cliente");
		}
		user->data.ip = addr.sin_addr;
		//end sección crítica
		struct MsgNode *msg = user->data.mensajes_pendientes->head;
		while (msg != NULL){
			if (msg->data.id < user->data.last_message){
				msg = msg->next;
				continue;
			}
			enviar_mensaje(&(user->data), &(msg->data));
			msg = msg->next;
		}

		//--------
		u_int16_t response;
		response = htons(0);
		if (write(sc_client, &response, 2) < 0){
			perror("Error al enviar el mensaje");
		}
		printf("s> CONNECT %s OK\n", alias);
		close(sc_client);
	}
	else if (strcmp(operacion, "DISCONNECT")== 0){
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		/*Check if client data is correct*/
		/**/
		if (searchUser(lista_usuarios, alias) == NULL){
			//alias does not exist
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		//alias exists
		//sección crítica
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (strcmp(user->data.estado, "DISCONNECTED") == 0){
			u_int16_t response;
			response = htons(2);
			write(sc_client, &response, 2);
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		struct sockaddr_in addr;
		socklen_t addr_size = sizeof(struct sockaddr_in);
		if (getpeername(sc_client, (struct sockaddr *)&addr, &addr_size) <0){
			perror("Error al obtener la ip del cliente");
		}
		if (user->data.ip.s_addr != addr.sin_addr.s_addr){
			u_int16_t response;
			response = htons(3);
			write(sc_client, &response, 2);
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		
		strcpy(user->data.estado, "DISCONNECTED");
		user->data.puerto = 0;
		user->data.ip.s_addr = 0;

		//end sección crítica

		u_int16_t response;
		response = htons(0);
		if (write(sc_client, &response, 2) < 0){
			perror("Error al enviar el mensaje");
		}
		printf("s> DISCONNECT %s OK\n", alias);
		close(sc_client);
	}
	else if (strcmp(operacion, "SEND")== 0){
		char alias[256];
		char alias_destino[256];
		char mensaje[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		if (readLine(sc_client, alias_destino, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		if (readLine(sc_client, mensaje, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		printf("check1\n");
		struct UserNode *user = searchUser(lista_usuarios, alias);
		struct UserNode *user_destino = searchUser(lista_usuarios, alias_destino);
		printf("check2\n");
		if(user == NULL || user_destino == NULL){
			//alias does not exist
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> SEND %s %s FAIL\n", alias, alias_destino);
			close(sc_client);
			return 0;
		}
		//alias exists

		struct mensaje *msgToSave;
		msgToSave = malloc(sizeof(struct mensaje));
		msgToSave->id = user->data.last_message + 1;
		strcpy(msgToSave->remite, alias);
		strcpy(msgToSave->mensaje, mensaje);
		printf("s> envio mensaje %s\n", msgToSave->mensaje);
		addMsg(user_destino->data.mensajes_pendientes, *msgToSave);
	    
		u_int16_t response;
		response = htons(0);
		if (write(sc_client, &response, 2) < 0){
			perror("Error al enviar el mensaje");
		}

		if (user_destino->data.estado == "DISCONNECTED"){
			printf("s> MESSAGE %d FROM  %s TO %s STORED\n", msgToSave->id, alias, alias_destino);
			close(sc_client);
			return 0;
		}
		printf("s> SEND MESSAGE %d FROM %s TO %s\n", msgToSave->id, alias, alias_destino);
		//enviar mensaje
		enviar_mensaje(&(user_destino->data), msgToSave);
		char str_id[sizeof(int)*8+1];
		if (sprintf(str_id, "%d", msgToSave->id) < 0){
			perror("Error al convertir el id a string");
		}
		sendMessage(sc_client,str_id, sizeof(int)*8+1);

		//sección crítica
		
		// add(lista_mensajes, mensaje);

	}
	else if (strcmp(operacion, "CONNECTEDUSERS") == 0){
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			perror("Error al recibir el mensaje");
		}
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (user == NULL){
			//alias does not exist
			u_int16_t response;
			response = htons(2);
			write(sc_client, &response, 2);
			printf("s> CONNECTEDUSERS FAIL\n");
			close(sc_client);
			return 0;
		}
		if (user->data.estado == "DISCONNECTED"){
			u_int16_t response;
			response = htons(1);
			write(sc_client, &response, 2);
			printf("s> CONNECTEDUSERS FAIL\n");
			close(sc_client);
			return 0;
		}
		u_int16_t response;
		write(sc_client, &response, 2);
		int num_usuarios = 0;
		for (struct UserNode *i = lista_usuarios->head; i != NULL; i = i->next){
			if (strcmp(i->data.estado, "CONNECTED") == 0){
				num_usuarios++;
			}
		};
		u_int32_t n_users = htonl(num_usuarios);

		write(sc_client, &n_users, 4);
		for (struct UserNode *i = lista_usuarios->head; i != NULL; i = i->next){
			if (strcmp(i->data.estado, "CONNECTED") == 0){
				sendMessage(sc_client, i->data.alias, 256);
			}
		};
		printf("s> CONNECTEDUSERS OK\n");
		close(sc_client);
	}
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
		printf("error en getnameinfo()\n");
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

	lista_usuarios = newUserList();

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