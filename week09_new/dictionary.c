#include <stdlib.h>
#include <string.h>
#include "linked_list.c"
#include "constants.h"

// List operation return codes
#define SUCCESS 0
#define DICTIONARY_NOT_EXIST -2
#define DICTIONARY_IS_EMPTY -3
#define FAILURE -1

struct Dictionary {
    int size;
    struct Pair *first;
};

struct Pair {
    char key[FILE_NAME_LENGTH];
    struct LinkedList *value;
    struct Pair *succ;
};

struct Dictionary *dict_create() {
    struct Dictionary *dict = malloc(sizeof(struct Dictionary));
    memset(dict, 0, sizeof(struct Dictionary));
    return(dict);
}

int dict_destroy(struct Dictionary *dict) {
    if (dict == NULL) {
        return (DICTIONARY_NOT_EXIST);
    }

    struct Pair *pair = dict->first;
    while (pair != NULL) {
        struct Pair *tempPair = pair->succ;
        if (list_destroy(pair->value) != SUCCESS) {
            return FAILURE;
        }
        free(pair);
        pair = tempPair;
    }
    
    free(dict);
    
    return SUCCESS;   
}

int dict_insert(
    char filename[FILE_NAME_LENGTH], 
    char name[NAME_LENGTH], 
    char addr[ADDRESS_LENGTH], 
    int port,
    struct Dictionary *dict
) {
    if (dict == NULL) {
        return DICTIONARY_NOT_EXIST;
    }
    if (dict->size == 0) {
        struct Pair *pair = malloc(sizeof(struct Pair));
        memset(pair, 0, sizeof(struct Pair));
        memcpy(pair->key, filename, strlen(filename));
        pair->value = list_create();
        if (list_insert(name, addr, port, pair->value) != SUCCESS) {
            return FAILURE;
        }
        dict->first = pair;
        dict->size++;
    }
    else {
        struct Pair *pair = dict->first;
        struct Pair *tempPair = NULL;
        while (pair != NULL && strcmp(pair->key, filename) != 0) {
            tempPair = pair;
            pair = pair->succ;
        }
        if (pair == NULL) {
            pair = malloc(sizeof(struct Pair));
            memset(pair, 0, sizeof(struct Pair));
            memcpy(pair->key, filename, strlen(filename));
            pair->value = list_create();
            if (list_insert(name, addr, port, pair->value) != SUCCESS) {
                return FAILURE;
            }
            tempPair->succ = pair;
            dict->size++;
            return 1;
        }
        else {
            struct Node *node = list_first(pair->value);
            while (node != NULL && (strcmp(node->addr, addr) != 0 || node->port != port)) {
                node = node->succ;
            }
            if (node == NULL) {
                if (list_insert(name, addr, port, pair->value) != SUCCESS) {
                    return FAILURE;
                }
            }
            return 0;
        }
    }
}

int dict_size(struct Dictionary *dict) {
    return dict->size;
}

int dict_isEmpty(struct Dictionary *dict) {
    return (dict->size == 0);
}

struct Pair* dict_first(struct Dictionary *dict) {
    return dict->first;
}

struct LinkedList* dict_get(char filename[FILE_NAME_LENGTH], struct Dictionary *dict) {
    if (dict == NULL) {
        return NULL;
    }

    struct Pair *pair = dict->first;
    while (pair != NULL && strcmp(pair->key, filename) != 0) {
        pair = pair->succ;
    }
    if (pair == NULL) {
        return NULL;
    }
    return pair->value;
}