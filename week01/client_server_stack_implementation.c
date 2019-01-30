#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#define TRUE 1
#define MAX_COMMAND_LENGTH 256

// Stack operations errors:
#define SUCCESS 0
#define STACK_NOT_EXISTS_ERROR 10
#define STACK_IS_EMPTY_ERROR 11

// Commands codes:
#define PEEK 0
#define PUSH 1
#define POP 2
#define EMPTY 3
#define DISPLAY 4
#define CREATE 5
#define STACK_SIZE 6
#define WRONG_COMMAND_ERROR -1

struct Node {
    int value;
    struct Node *prev;
};

struct Stack {
    struct Node *head;
    int size;
};

struct Stack *current_stack = NULL;

int err_status = SUCCESS;

int stack_exists() {
    return (current_stack != NULL);
}

int empty() {
    if (stack_exists()) {
        return (current_stack->size == 0);
    } else {
        return STACK_NOT_EXISTS_ERROR;
    }
}

int peek() {
    int is_empty_status;
    if ((is_empty_status = empty()) == STACK_NOT_EXISTS_ERROR) {
        err_status = STACK_NOT_EXISTS_ERROR;
        return -1;
    } else if (is_empty_status) {
        err_status = STACK_IS_EMPTY_ERROR;
        return -1;
    }
    err_status = SUCCESS;
    return (current_stack->head->value);
}

void push(int data) {
    if (empty() == STACK_NOT_EXISTS_ERROR) {
        err_status = STACK_NOT_EXISTS_ERROR;
        return;
    }
    struct Node *new_node = malloc(sizeof(struct Node));
    new_node->value = data;
    new_node->prev = current_stack->head;
    current_stack->head = new_node;
    current_stack->size++;
    err_status = SUCCESS;
}

void pop() {
    int is_empty_status;
    if ((is_empty_status = empty()) == STACK_NOT_EXISTS_ERROR) {
        err_status = STACK_NOT_EXISTS_ERROR;
    } else if (is_empty_status) {
        err_status = STACK_IS_EMPTY_ERROR;
    } else {
        struct Node *current_head = current_stack->head;
        current_stack->head = current_head->prev;
        free(current_head);
        current_stack->size--;
        err_status = SUCCESS;
    }    
}

void create() {
    if (stack_exists()) {
        // To free out of memory all nodes of current stack
        for (int i = 0; i < current_stack->size; i++) {
            pop();
        }
        free(current_stack);
    }
    current_stack = malloc(sizeof(struct Stack));
    current_stack->head = NULL;
    current_stack->size = 0;
}

void display() {
    int is_empty_status;
    if ((is_empty_status = empty()) == STACK_NOT_EXISTS_ERROR) {
        err_status = STACK_NOT_EXISTS_ERROR;
    } else if (is_empty_status) {
        printf("Stack is empty\n");
        err_status = SUCCESS;
    } else {
        struct Node *enum_node = current_stack->head;
        int stack_size = current_stack->size;
        int stack_values[stack_size];
        for (int i = 0; i < stack_size; ++i) {
            stack_values[i] = enum_node->value;
            enum_node = enum_node->prev;
        }
        for (int i = stack_size-1; i > -1; --i) {
            printf("%d ", stack_values[i]);
        }
        printf("\n");
        err_status = SUCCESS;
    }
}

void stack_size() {
    if (stack_exists()) {
        printf("%d\n", current_stack->size);
        err_status = SUCCESS;
    } else {
        err_status = STACK_NOT_EXISTS_ERROR;
    }
}

int is_peek_command(char *command) {
    return (!strcmp(command, "peek"));
}

int is_push_command(char *command) {
    return (!strcmp(command, "push"));
}

int is_pop_command(char *command) {
    return (!strcmp(command, "pop"));
}

int is_empty_command(char *command) {
    return (!strcmp(command, "empty"));
}

int is_display_command(char *command) {
    return (!strcmp(command, "display"));
}

int is_create_command(char *command) {
    return (!strcmp(command, "create"));
}

int is_stack_size_command(char *command) {
    return (!strcmp(command, "stack_size"));
}

int type_command() {
    char buf[MAX_COMMAND_LENGTH];
    if (scanf("%s", buf) < 0) {
        perror("scanf");
        exit(EXIT_FAILURE);
    };
    if (is_peek_command(buf)) {
        return PEEK;
    }
    else if (is_push_command(buf)) {
        return PUSH;
    }
    else if (is_pop_command(buf)) {
        return POP;
    }
    else if (is_empty_command(buf)) {
        return EMPTY;
    }
    else if (is_display_command(buf)) {
        return DISPLAY;
    }
    else if (is_create_command(buf)) {
        return CREATE;
    }
    else if (is_stack_size_command(buf)) {
        return STACK_SIZE;
    }
    return WRONG_COMMAND_ERROR;
}

int get_push_value() {
    int value;
    int res;
    if ((res = scanf("%d", &value)) < 0) {
        perror("scanf");
        exit(EXIT_FAILURE);
    }
    if (res == 0) {
        err_status = WRONG_COMMAND_ERROR;
        return -1;
    }
    err_status = SUCCESS;
    return value;
}

int check_server_error_status() {
    if (err_status == STACK_NOT_EXISTS_ERROR) {
        printf("ERROR: Stack does not exist. Command has not been executed\n");
        return -1;
    }
    else if (err_status == STACK_IS_EMPTY_ERROR) {
        printf("ERROR: Stack is empty. Command has not been executed\n");
        return -1;
    }
    return 0;
}

int check_client_error_status() {
    if (err_status == WRONG_COMMAND_ERROR) {
        printf("ERROR: Push command has no arguments or argument is not an integer\n");
        return -1;
    }
    return 0;
}

void discard_input() {
    int c;
    while((c = getchar()) != '\n' && c != EOF);
}

int sig_status;
void usr1_signal_handler(int signum) {
    sig_status = signum;
    return;
}

int main() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {
        close(pipefd[1]);
        int command[2];
        while (TRUE) {
            read(pipefd[0], command, 2 * sizeof(int));
            if (command[0] == PEEK) {
                int current_peek = peek();
                if (!check_server_error_status()) {
                    printf("%d\n", current_peek);
                }
            } else if (command[0] == PUSH) {
                push(command[1]);
                if (!check_server_error_status()) {
                    printf("%d was pushed into stack\n", command[1]);
                }
            } else if (command[0] == POP) {
                pop();
                if (!check_server_error_status()) {
                    printf("Upper element was popped from the stack\n");
                }
            } else if (command[0] == EMPTY) {
                int is_empty = empty();
                if (is_empty != STACK_NOT_EXISTS_ERROR) {
                    if (is_empty) {
                        printf("Stack is empty\n");
                    } else {
                        printf("Stack has at least one element\n");
                    }
                } else {
                    printf("ERROR: Stack does not exist\n");
                }
            } else if (command[0] == DISPLAY) {
                display();
                check_server_error_status();
            } else if (command[0] == CREATE) {
                create();
                printf("Stack was successfully created\n");
            } else if (command[0] == STACK_SIZE) {
                stack_size();
                check_server_error_status();
            }
            kill(getppid(), SIGUSR1);
        }
    } else {
        close(pipefd[0]);
        int command[2];
        signal(SIGUSR1, usr1_signal_handler);
        while (TRUE) {
            sig_status = 0;
            printf("$ ");
            command[0] = type_command();
            if (command[0] == WRONG_COMMAND_ERROR) {
                printf("Unknown command\n");
                discard_input();
                continue;
            }
            if (command[0] == PUSH) {
                command[1] = get_push_value();
                if (check_client_error_status()) {
                    discard_input();
                    continue;
                }
            }
            write(pipefd[1], command, 2 * sizeof(int));
            discard_input();
            while (!sig_status);
        }
    }
    return 0;
}