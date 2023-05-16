#ifndef _SLL
#define _SLL
#include <stdint.h>
#include <netinet/in.h>
/*------------------Messages-List--------------*/
struct mensaje
{   
    int id;
    char remite[256];
    char mensaje[256];

};

struct MsgNode {
    struct mensaje data;
    struct MsgNode *next;
};

struct MsgList {
    struct MsgNode *head;
    struct MsgNode *tail;
};

struct MsgNode *newMsg(struct mensaje data);
struct MsgList *newMsgList();
void addMsg(struct MsgList *list, struct mensaje data);
void freeMsgList(struct MsgList *list);
void removeMsgNode(struct MsgList *list, int id);
struct MsgNode *searchMsg(struct MsgList *list, int id);

/*--------------------User-List---------------*/



struct usuario
{
    char nombre[256];
    char alias[256];
    char fecha[11];
    char estado[13];
    struct in_addr ip;
    uint16_t puerto;
    uint32_t last_message;
    struct MsgList *mensajes_pendientes;
};

struct UserNode {
    struct usuario data;
    struct UserNode *next;
};

struct UserList {
    struct UserNode *head;
    struct UserNode *tail;
};



struct UserNode *newUser(struct usuario data);
struct UserList *newUserList();
void addUser(struct UserList *list, struct usuario data);
void freeUserList(struct UserList *list);
void removeUserNode(struct UserList *list, char *alias);
struct UserNode *searchUser(struct UserList *list, char *alias);
void printList(struct UserList *list);

#endif