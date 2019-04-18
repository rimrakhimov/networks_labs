#include <stdlib.h>
#include "constants.h"

// List operation return codes
#define SUCCESS 0
#define LIST_NOT_EXIST 1
#define NODE_NOT_EXIST 2
#define LIST_IS_EMPTY 3
#define INDEX_OUT_OF_RANGE 4

struct Node {
    char name[NAME_LENGTH];
    char addr[ADDRESS_LENGTH];
    int port;
    struct Node *succ;
    struct Node *pred;
};

struct LinkedList {
    int size;
    struct Node *first;
    struct Node *last;
};

struct LinkedList* list_create() {
    struct LinkedList *linkedList = malloc(sizeof(struct LinkedList));
    memset(linkedList, 0, sizeof(struct LinkedList));
    return(linkedList);
}

int list_destroy(struct LinkedList *linkedList) {
    if (linkedList == NULL) {
        return(LIST_NOT_EXIST);
    }
    struct Node *node = linkedList->first;
    for (int i = 0; i < linkedList->size-1; ++i) {
        node = node->succ;
        free(node->pred);
    }
    free(node);
    free(linkedList);
    
    return SUCCESS;
}

int list_insert(char name[20], char addr[15], int port, struct LinkedList *linkedList) {
    if (linkedList == NULL) {
        return LIST_NOT_EXIST;
    }
    if (linkedList->size == 0) {
        struct Node *node = malloc(sizeof(struct Node));
        node->succ = NULL;
        node->pred = NULL;
        memcpy(node->name, name, NAME_LENGTH);
        memcpy(node->addr, addr, ADDRESS_LENGTH);
        node->port = port;
        linkedList->first = node;
        linkedList->last = node;
    }
    else {
        struct Node *tempNode = linkedList->last;
        struct Node *node = malloc(sizeof(struct Node));
        memcpy(node->name, name, NAME_LENGTH);
        memcpy(node->addr, addr, ADDRESS_LENGTH);
        node->port = port;
        linkedList->last = node;
        node->succ = NULL;
        node->pred = tempNode;
        tempNode->succ = node;
    }
    linkedList->size++;
    return SUCCESS;
}

int list_delete(int index, struct LinkedList *linkedList) {
    if (linkedList == NULL) {
        return LIST_NOT_EXIST;
    }

    if (linkedList->size <= index) {
        return INDEX_OUT_OF_RANGE;
    }

    struct Node *node = linkedList->first;
    if (linkedList->size == 1) {
        linkedList->first = NULL;
        linkedList->last = NULL;
    } else {
        if (index == 0) {
            struct Node *tempNode = node->succ;
            tempNode->pred = NULL;
            linkedList->first = tempNode;
        } else if (index == linkedList->size - 1) {
            struct Node *tempNode = node->pred;
            tempNode->succ = NULL;
            linkedList->last = tempNode;
        } else {
            for (int i = 0; i < index; ++i) {
                node = node->succ;
            }
            node->pred->succ = node->succ;
            node->succ->pred = node->pred;
        }
    }

    free(node);
    linkedList->size--;

    return SUCCESS;
}

int list_delete_node(struct Node* node, struct LinkedList* linkedList) {
    if (linkedList == NULL) {
        return LIST_NOT_EXIST;
    }

    if (node == NULL) {
        return NODE_NOT_EXIST;
    }

    if (node->pred == NULL && node->succ == NULL) {
        linkedList->first = NULL;
        linkedList->last = NULL;
    } else if (node->pred == NULL) {
        linkedList->first = node->succ;
        node->succ->pred = NULL;
    } else if (node->succ == NULL) {
        linkedList->last = node->pred;
        node->pred->succ = NULL;
    } else {
        node->pred->succ = node->succ;
        node->succ->pred = node->pred;
    }

    free(node);
    linkedList->size--;

    return SUCCESS;

}

int list_size(struct LinkedList *linkedList) {
    return linkedList->size;
}

int list_isEmpty(struct LinkedList *linkedList) {
    return (linkedList->size == 0);
}

struct Node* list_first(struct LinkedList *linkedList) {
    return linkedList->first;
}

struct Node* list_last(struct LinkedList *linkedList) {
    return linkedList->last;
}

struct Node* list_get(int index, struct LinkedList *linkedList) {
    if (linkedList == NULL || index >= list_size(linkedList)) {
        return NULL;
    }
    struct Node *node = linkedList->first;
    for (int i = 0; i < index; ++i) {
        node = node->succ;
    }
    return node;
}