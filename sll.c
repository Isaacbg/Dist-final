#include "sll.h"
#include <stdlib.h>
#include <stdio.h>



struct Node *newNode(struct elemento data) {
    struct Node *new = malloc(sizeof(struct Node));
    new->data = data;
    new->next = NULL;
    return new;
}

// Create a new list
struct List *newList() {
    struct List *new = malloc(sizeof(struct List));
    new->head = NULL;
    new->tail = NULL;
    return new;
}

// Add a node to the end of the list
void add(struct List *list, struct elemento data) {
    struct  Node *new = newNode(data);
    if (list->head == NULL) {
        list->head = new;
        list->tail = new;
    } else {
        
        list->tail->next = new;
        list->tail = new;
    }
}

// Free the list
void freeList(struct List *list) {
    struct Node *current = list->head;
    while (current != NULL) {
        struct Node *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// Remove the first node with the given key
void removeNode(struct List *list, int key) {
    struct Node *current = list->head;
    struct Node *prev = NULL;
    while (current != NULL) {
        if (current->data.clave == key) {
            if (prev == NULL) {
                list->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

//search an element in the list given a value for data.key 
struct Node *search(struct List *list, int key) {
    struct Node *current = list->head;
    while (current != NULL) {
        if (current->data.clave == key) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//print the list
void printList(struct List *list) {
    struct Node *current = list->head;
    while (current != NULL) {
        printf("%d : %s , %d , %lf \n", current->data.clave, current->data.val1, current->data.val2, current->data.val3);
        current = current->next;
    }
    printf("fin \n");
}

