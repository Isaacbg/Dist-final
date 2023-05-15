#include "sll.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*------------------Messages-List--------------*/

struct MsgNode *newMsg(struct mensaje data) {
    struct MsgNode *new = malloc(sizeof(struct MsgNode));
    new->data = data;
    new->next = NULL;
    return new;
}

struct MsgList *newMsgList() {
    struct MsgList *new = malloc(sizeof(struct MsgList));
    new->head = NULL;
    new->tail = NULL;
    return new;
}

void addMsg(struct MsgList *list, struct mensaje data) {
    struct  MsgNode *new = newMsg(data);
    if (list->head == NULL) {
        list->head = new;
        list->tail = new;
    } else {
        
        list->tail->next = new;
        list->tail = new;
    }
}

void freeMsgList(struct MsgList *list) {
    struct MsgNode *current = list->head;
    struct MsgNode *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

void removeMsgNode(struct MsgList *list, int id) {
    struct MsgNode *current = list->head;
    struct MsgNode *previous = NULL;
    while (current != NULL) {
        if (current->data.id == id) {
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

struct MsgNode *searchMsg(struct MsgList *list, int id) {
    struct MsgNode *current = list->head;
    while (current != NULL) {
        if (current->data.id == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/*--------------------User-List---------------*/
struct UserNode *newUser(struct usuario data) {
    struct UserNode *new = malloc(sizeof(struct UserNode));
    new->data = data;
    new->next = NULL;
    new->data.mensajes_pendientes = newMsgList();
    return new;
}

// Create a new list
struct UserList *newUserList() {
    struct UserList *new = malloc(sizeof(struct UserList));
    new->head = NULL;
    new->tail = NULL;
    return new;
}

// Add a node to the end of the list
void addUser(struct UserList *list, struct usuario data) {
    struct  UserNode *new = newUser(data);
    if (list->head == NULL) {
        list->head = new;
        list->tail = new;
    } else {
        
        list->tail->next = new;
        list->tail = new;
    }
}

// Free the list
void freeUserList(struct UserList *list) {
    struct UserNode *current = list->head;
    while (current != NULL) {
        struct UserNode *next = current->next;
        freeMsgList(current->data.mensajes_pendientes);
        free(current);
        current = next;
    }
    free(list);
}

// Remove the first node with the given key
void removeUserNode(struct UserList *list, char *alias) {
    struct UserNode *current = list->head;
    struct UserNode *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->data.alias, alias) == 0) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            //freeMsgList(current->data.mensajes_pendientes);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

//search an element in the list given a value for data.key 
struct UserNode *searchUser(struct UserList *list, char *alias) {
    struct UserNode *current = list->head;
    while (current != NULL) {
        if (strcmp(current->data.alias, alias) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
/*
//print the list
void printList(struct List *list) {
    struct Node *current = list->head;
    while (current != NULL) {
        printf("%d : %s , %d , %lf \n", current->data.clave, current->data.val1, current->data.val2, current->data.val3);
        current = current->next;
    }
    printf("fin \n");
}
*/
