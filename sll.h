#ifndef _SLL
#define _SLL
struct elemento
{
    int clave;
    char val1[256];
    int val2;
    double val3;
};

struct Node {
    struct elemento data;
    struct Node *next;
};

struct List {
    struct Node *head;
    struct Node *tail;
};



struct Node *newNode(struct elemento data);
struct List *newList();
void add(struct List *list, struct elemento data);
void freeList(struct List *list);
void removeNode(struct List *list, int data);
struct Node *search(struct List *list, int key);
void printList(struct List *list);

#endif