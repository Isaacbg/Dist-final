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

struct parametros_thread {
	int socket;
	struct sockaddr_in client_ip;
};

//variables globales
pthread_mutex_t mutex_mensaje;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje;

pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;

struct UserList *lista_usuarios;



void enviar_mensaje(struct usuario *user ,struct mensaje *msg){
	/* Enviamos el  mensaje al usuario destino, si está desconectado mandamos un mensaje de guardado al remitente.
	 * Si el usuario destino está conectado, le enviamos el mensaje y un mensaje de confirmación al remitente.
	 * Si el usuario destino estaba desconectado y se conecta, le enviamos el mensaje y un mensaje de confirmación al remitente.
	*/

	// Usuario destino
	struct sockaddr_in client_listen;
	int sc_client;
	
	if (strcmp(user->estado, "DISCONNECTED") == 0){
		printf("s> MESSAGE %d FROM  %s TO %s STORED\n", msg->id, msg->remite, user->alias);
		return ;
	}

	if ((sc_client = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error al crear el socket");
		exit(1);
	}
	client_listen.sin_family = AF_INET;
	client_listen.sin_port = htons(user->puerto);
	client_listen.sin_addr = user->ip;

	if (connect(sc_client, (struct sockaddr *)&client_listen, sizeof(struct sockaddr_in)) < 0){
		perror("Error al conectar con el cliente");
		exit(1);
	}

	// Enviamos el mensaje al usuario destino	
	sendMessage(sc_client, "SEND_MESSAGE\0", 13);
	sendMessage(sc_client, msg->remite, strlen(msg->remite) + 1);
	char id[33];
	sprintf(id, "%d", msg->id);
	sendMessage(sc_client, id, strlen(id) + 1);
	sendMessage(sc_client, msg->mensaje, strlen(msg->mensaje) + 1);

	printf("s> SEND MESSAGE %d FROM %s TO %s\n", msg->id, msg->remite, user->alias);

	user->last_message = msg->id; 
	

	struct UserNode *remite = searchUser(lista_usuarios, msg->remite);
	// Si el usuario remitente está desconectado cuando el usuario destino recibe el mensaje, se ignora el mensaje de confirmación
	if (strcmp(remite->data.estado, "DISCONNECTED") == 0){
		return;
	}

	// Usuario remitente
	struct sockaddr_in remite_listen;
	int sc_remite;
	if ((sc_remite = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error al crear el socket");
		exit(1);
	}
	
	remite_listen.sin_family = AF_INET;
	remite_listen.sin_port = htons(remite->data.puerto);
	remite_listen.sin_addr = remite->data.ip;

	if (connect(sc_remite, (struct sockaddr *)&remite_listen, sizeof(struct sockaddr_in)) < 0){
		perror("Error al conectar con el cliente");
		exit(1);
	}

	// Enviamos el mensaje de confirmación al usuario remitente
	sendMessage(sc_remite, "SEND_MESS_ACK\0", 14);
	sendMessage(sc_remite, id, strlen(id) + 1);

};


int tratar_mensaje(struct parametros_thread *parametros){
	/* Función que trata los mensajes recibidos por el servidor.
	 * Según el código de operación, se realiza una acción u otra.
 	*/

    int sc_client; 
	struct sockaddr_in client_ip;

    pthread_mutex_lock(&mutex_mensaje);

	// Copiamos localmente los datos del socket y la ip del cliente
	sc_client = parametros->socket;
	client_ip = parametros->client_ip;
	
	mensaje_no_copiado = false;
	pthread_cond_signal(&cond_mensaje);
	pthread_mutex_unlock(&mutex_mensaje);

	char operacion[256];
	
	if (readLine(sc_client, operacion, 256) < 0){
		// Si falla la comunicación se cierra el socket
		close(sc_client);
		perror("Error al recibir el mensaje");
	}
	printf("s> %s\n", operacion);
	if (strcmp(operacion, "REGISTER") == 0){
		// Se registra un nuevo usuario
		char nombre[256];
		char response;
		if (readLine(sc_client, nombre, 256) < 0){
			printf("s> REGISTER FAIL BEFORE ALIAS\n");
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			return 0;
		}
		char alias[256];
		if (readLine(sc_client, alias, 256) < 0){
			printf("s> REGISTER FAIL BEFORE ALIAS\n");
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			return 0;
		}
		char fecha[11];
		if (readLine(sc_client, fecha, 11) < 0){
			printf("s> REGISTER %s FAIL\n", alias);
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			return 0;
		}
		
		pthread_mutex_lock(&mutex_usuarios);
		if (searchUser(lista_usuarios, alias) != NULL){
			// Si el alias ya está registrado, se envía un mensaje de error
			pthread_mutex_unlock(&mutex_usuarios);
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> REGISTER %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		// Creamos e inicializamos la estructura del nuevo usuario 
		struct usuario nuevo_usuario;
		strcpy(nuevo_usuario.nombre, nombre);
		strcpy(nuevo_usuario.alias, alias);
		strcpy(nuevo_usuario.fecha, fecha);
		strcpy(nuevo_usuario.estado, "DISCONNECTED\0");
		nuevo_usuario.ip.s_addr = 0;
		nuevo_usuario.puerto = 0;
		nuevo_usuario.last_message = 0;

		// Añadir a la lista de usuarios
		addUser(lista_usuarios, nuevo_usuario);
		
		pthread_mutex_unlock(&mutex_usuarios);

		// Envío de respuesta al cliente
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			perror("Error al enviar el mensaje");	
		}
		printf("s> REGISTER %s OK\n", alias);

		close(sc_client);
		return 0;
	}	
	else if (strcmp(operacion, "UNREGISTER") == 0){
		// Se elimina un usuario
		char alias[256];
		char response;
		// Leer alias
		if (readLine(sc_client, alias, 256) < 0){
			printf("s> UNREGISTER FAIL BEFORE ALIAS\n");
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			return 0;
		}
		pthread_mutex_lock(&mutex_usuarios);
		// Buscamos al usuario
		if (searchUser(lista_usuarios, alias) == NULL){
			// No se encuentra el usuario
			pthread_mutex_unlock(&mutex_usuarios);
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> UNREGISTER %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		// Eliminamos al usuario
		removeUserNode(lista_usuarios, alias);
		pthread_mutex_unlock(&mutex_usuarios);
		// Envío de respuesta al cliente
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			perror("Error al enviar el mensaje");	
		}
		printf("s> UNREGISTER %s OK\n", alias);
		close(sc_client);
		return 0;
	}

	else if (strcmp(operacion, "CONNECT")== 0){
		// Se conecta un usuario
		char alias[256];
		char response;
		// Leer alias
		if (readLine(sc_client, alias, 256) < 0){
			perror("s> CONNECT FAIL BEFORE ALIAS\n");
			response = '3';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			return 0;
		}
		// Leer puerto
		char port[7];
		if (readLine(sc_client, port, 7) < 0){
			printf("CONNECT %s FAIL\n", alias);
			return 0;
		}
		
		pthread_mutex_lock(&mutex_usuarios);
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (user == NULL){
			pthread_mutex_unlock(&mutex_usuarios);
			//alias does not exist
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> CONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		//alias exists
		//sección crítica
		if (strcmp(user->data.estado, "CONNECTED") == 0){
			pthread_mutex_unlock(&mutex_usuarios);
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> CONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		strcpy(user->data.estado, "CONNECTED");
		user->data.puerto = atoi(port);
		user->data.ip = client_ip.sin_addr;
		//end sección crítica
		struct MsgNode *msg = user->data.mensajes_pendientes->head;
		while (msg != NULL){
			if (msg->data.id <= user->data.last_message){
				msg = msg->next;
				continue;
			}
			enviar_mensaje(&(user->data), &(msg->data));
			msg = msg->next;
		}
		pthread_mutex_unlock(&mutex_usuarios);
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			perror("Error al enviar el mensaje");	
		}
		printf("s> CONNECT %s OK\n", alias);
		close(sc_client);
		return 0;
	}
	else if (strcmp(operacion, "DISCONNECT")== 0){
		char alias[256];
		char response;
		if (readLine(sc_client, alias, 256) < 0){
			printf("DISCONNECT FAIL BEFORE ALIAS\n");
			return 0;
		}
		/*Check if client data is correct*/
		/**/
		pthread_mutex_lock(&mutex_usuarios);
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (user == NULL){
			pthread_mutex_unlock(&mutex_usuarios);
			//alias does not exist
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}

		//alias exists
		if (strcmp(user->data.estado, "DISCONNECTED") == 0){
			pthread_mutex_unlock(&mutex_usuarios);
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		if (user->data.ip.s_addr != client_ip.sin_addr.s_addr){
			pthread_mutex_unlock(&mutex_usuarios);
			response = '3';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> DISCONNECT %s FAIL\n", alias);
			close(sc_client);
			return 0;
		}
		
		strcpy(user->data.estado, "DISCONNECTED");
		user->data.puerto = 0;
		user->data.ip.s_addr = 0;

		pthread_mutex_unlock(&mutex_usuarios);
		
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			perror("Error al enviar el mensaje");	
		}
		printf("s> DISCONNECT %s OK\n", alias);
		close(sc_client);
		return 0;
	}
	else if (strcmp(operacion, "SEND")== 0){
		char alias[256];
		char alias_destino[256];
		char mensaje[256];
		char response;
		if (readLine(sc_client, alias, 256) < 0){
			printf("SEND FAIL BEFORE ALIAS\n");
			return 0;
		}
		if (readLine(sc_client, alias_destino, 256) < 0){
			printf("s> SEND %s FAIL\n", alias);
			return 0;
		}
		if (readLine(sc_client, mensaje, 256) < 0){
			printf("s> SEND %s %s FAIL\n", alias, alias_destino);
			return 0;
		}
		pthread_mutex_lock(&mutex_usuarios);
		struct UserNode *user = searchUser(lista_usuarios, alias);
		struct UserNode *user_destino = searchUser(lista_usuarios, alias_destino);
		printf("check2\n");
		if(user == NULL || user_destino == NULL){
			pthread_mutex_unlock(&mutex_usuarios);
			//alias does not exist
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> SEND %s %s FAIL\n", alias, alias_destino);
			close(sc_client);
			return 0;
		}
		//alias exists

		struct mensaje *msgToSave;
		msgToSave = malloc(sizeof(struct mensaje));
		msgToSave->id = user_destino->data.last_message + 1;
		strcpy(msgToSave->remite, alias);
		strcpy(msgToSave->mensaje, mensaje);
		// printf("s> envio mensaje %s\n", msgToSave->mensaje);
		addMsg(user_destino->data.mensajes_pendientes, *msgToSave);
		
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			perror("Error al enviar el mensaje");	
		}
		char str_id[sizeof(int)*8+1];
		if (sprintf(str_id, "%d", msgToSave->id) < 0){
			printf("s> SEND %s %s FAIL\n", alias, alias_destino);
			return 0;
		}
		sendMessage(sc_client,str_id, strlen(str_id)+1);

		enviar_mensaje(&(user_destino->data), msgToSave);
		
		pthread_mutex_unlock(&mutex_usuarios);

		close(sc_client);
		return 0;
	}
	else if (strcmp(operacion, "CONNECTEDUSERS") == 0){
		char alias[256];
		char response;
		if (readLine(sc_client, alias, 256) < 0){
			printf("s> CONNECTEDUSERS FAIL BEFORE ALIAS \n");
			return 0;
		}
		pthread_mutex_lock(&mutex_usuarios);
		struct UserNode *user = searchUser(lista_usuarios, alias);
		if (user == NULL){
			pthread_mutex_unlock(&mutex_usuarios);
			//alias does not exist
			response = '2';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			close(sc_client);
			return 0;
		}
		if (strcmp(user->data.estado, "DISCONNECTED") == 0){
			pthread_mutex_unlock(&mutex_usuarios);
			response = '1';
			if (sendMessage(sc_client, &response, 1) < 0){
				perror("Error al enviar el mensaje");	
			}
			printf("s> CONNECTEDUSERS FAIL\n");
			close(sc_client);
			return 0;
		}
		response = '0';
		if (sendMessage(sc_client, &response, 1) < 0){
			pthread_mutex_unlock(&mutex_usuarios);
			perror("Error al enviar el mensaje");	
		}
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
				sendMessage(sc_client, i->data.alias, strlen(i->data.alias) + 1);
			}
		};
		pthread_mutex_unlock(&mutex_usuarios);
		printf("s> CONNECTEDUSERS OK\n");
		close(sc_client);
		return 0;
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

	struct ifaddrs *ifaddr, *ifa;
	int family, s;
    char ipLocal[NI_MAXHOST];
	if (getifaddrs(&ifaddr) == -1) {
		printf("error al obtener la ip local\n");
		return -1;
    }

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),ipLocal, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if((strcmp(ifa->ifa_name,"wlo1")==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0){
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
				}
			break;
    }
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

	struct parametros_thread parametros;



	for(;;) {
		parametros.socket = accept(sc_server,(struct sockaddr *)&parametros.client_ip, &size);
		if (parametros.socket < 0){
			close(parametros.socket);
			perror("Error al aceptar el socket del cliente");
			break;
		}
		if (pthread_create(&thid, &t_attr, (void *)tratar_mensaje, (void *)&parametros)== 0) {
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