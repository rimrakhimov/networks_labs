#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "linked_list.c"
#include "constants.h"

#define TRUE 1
#define FALSE 0

#define PEER_NAME "Rim"

#define TO_NET_ORDER TRUE

int server_port; // Port binded to the server

struct LinkedList *known_nodes;

pthread_mutex_t known_nodes_lock;

uint32_t htonl_o(uint32_t hostlong) {
    if (TO_NET_ORDER) {
        return htonl(hostlong);
    } else {
        return hostlong;
    }
}

uint32_t ntohl_o(uint32_t netlong) {
    if (TO_NET_ORDER) {
        return ntohl(netlong);
    } else {
        return netlong;
    }
}

/* Create introduction message for given IP interface */
int create_introduction(char *addr, char *buffer, int len) {
    int size = 0, str_len = 0;
    memset(buffer, 0, len);

    // set name of the peer
    str_len = strlen(PEER_NAME);
    memcpy(buffer, PEER_NAME, str_len);
    size += str_len;

    buffer[size++] = ':';

    // set ip address of the peer
    str_len = strlen(addr);
    memcpy(buffer+size, addr, str_len);
    size += str_len;

    buffer[size++] = ':';

    // set port number of the peer
    char port_number[5];
    sprintf(port_number, "%d", server_port);
    str_len = strlen(port_number);
    memcpy(buffer+size, port_number, str_len);
    size += str_len;

    buffer[size] = '\0';

    return 0;
}

/* Create message with information about specified peer */
int get_peer_information(struct Node* node, char *buf, int len) {
    int size = 0, str_len = 0;
    memset(buf, 0, len);

    // set name of the peer
    str_len = strlen(node->name);
    memcpy(buf, node->name, str_len);
    size += str_len;

    buf[size++] = ':';

    // set ip address of the peer
    str_len = strlen(node->addr);
    memcpy(buf+size, node->addr, str_len);
    size += str_len;

    buf[size++] = ':';

    // set port number of the peer
    char port_number[5];
    sprintf(port_number, "%d", node->port);
    str_len =  strlen(port_number);
    memcpy(buf+size, port_number, str_len);
    size += str_len;
    
    buf[size] = '\0';

    return 0;
}

void *synch(void *data) {
    struct Node *node = (struct Node*)data;

    /* Initialization */
    /* Socket handle */
    int sockfd = 0;

    int addr_len = sizeof(struct sockaddr);

    /* to store socket addesses : ip address and port */
    struct sockaddr_in dest, server_addr;

    /* Specify server information */
    /* Ipv4 sockets */
    dest.sin_family = AF_INET;
    dest.sin_port = htons(node->port);
    if (inet_aton(node->addr, &dest.sin_addr) == 0) {
        perror("address convertion to binary form failed");
        pthread_exit(NULL);
    }

    /* Create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket creation while sync failed");
        pthread_exit(NULL);
    }

    if (connect(sockfd, (struct sockaddr *)&dest, addr_len) < 0) {
        perror("connection while sync failed");
        printf("node %s:%d is deleted from list of known nodes\n",
            inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
        pthread_mutex_lock(&known_nodes_lock);
        list_delete_node(node, known_nodes);
        pthread_mutex_unlock(&known_nodes_lock);
        close(sockfd);
        pthread_exit(NULL);
    }

    /* Get information about IP address used for the connection */
    if (getsockname(sockfd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
        perror("get socket name while sync failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    /* Prepare introduction message to send */
    char introduction[INTRODUCTION_LENGTH];  // structure for node to introduce itself
    if (create_introduction(inet_ntoa(server_addr.sin_addr), introduction, sizeof(introduction)) != 0) {
        printf("Creation of introduction message failed\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("Sending sync operation to address: %s, port: %d\n",
        inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

    int sync_code = htonl_o(1);

    /* Sent sync command to begin synchronization */
    if (send(sockfd, &sync_code, sizeof(int), 0) < 0) {
        perror("send sync command failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    int recv_bytes = 0;

    int status;
    if ((recv_bytes = recv(sockfd, &status, sizeof(int), MSG_WAITALL)) < 0) {
        perror("recv status while synchronized");
        close(sockfd);
        pthread_exit(NULL);
    } else if (recv_bytes == 0) {
        printf("stream socket peer has performed an orderly shutdown while waiting for status code\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (ntohl_o(status) != 1) {
        printf("peer %s:%d is not ready to synchronize. It is deleted from list of known nodes\n", 
            inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
        pthread_mutex_lock(&known_nodes_lock);
        list_delete_node(node, known_nodes);
        pthread_mutex_unlock(&known_nodes_lock);
        close(sockfd);
        pthread_exit(NULL);
    }

    /* Send introduction message */
    if (send(sockfd, introduction, sizeof(introduction), 0) < 0) {
        perror("send introduction message failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    int nodes_count = htonl_o(list_size(known_nodes)); // available nodes in the peer's database

    /* Send number of available nodes in peer's database */
    if (send(sockfd, &nodes_count, sizeof(int), 0) < 0) {
        perror("send number of nodes failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    /* Send information about each peer from the database */
    for (int i = 0; i < ntohl_o(nodes_count); ++i) {
        struct Node *node = list_get(i, known_nodes);
        char buf[PEER_INFORMATION_LENGTH];

        if (get_peer_information(node, buf, sizeof(buf)) != 0) {
            printf("Creation of peer information message failed\n");
            close(sockfd);
            pthread_exit(NULL);
        }
        
        if (send(sockfd, buf, sizeof(buf), 0) < 0) {
            perror("send peer information failed");
            close(sockfd);
            pthread_exit(NULL);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

void *setup_tcp_communication(void *data) {
    while(TRUE) {
        for (int i = 0; i < list_size(known_nodes); ++i) {
            struct Node *node = list_get(i, known_nodes);
            pthread_t sync_thread;
            pthread_create(&sync_thread, NULL, synch, (void*)node);
        }
        sleep(10);
    }
}

struct Node* get_known_peer(char addr[ADDRESS_LENGTH], int port) {
    struct Node *node = known_nodes->first;
    while (node != NULL) {
        if (strcmp(addr, node->addr) == 0 && port == node->port) {
            return node;
        }
        node = node->succ;
    }
    return NULL;
}

int check_node(char addr[ADDRESS_LENGTH], int port) {
    /* Initialization */
    /* Socket handle */
    int sockfd = 0;

    int addr_len = sizeof(struct sockaddr);

    /* to store socket addesses : ip address and port */
    struct sockaddr_in dest, server_addr;

    /* Specify server information */
    /* Ipv4 sockets */
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (inet_aton(addr, &dest.sin_addr) == 0) {
        return 0;
    }

    /* Create a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        return 0;
    }

    if (connect(sockfd, (struct sockaddr *)&dest, addr_len) < 0) {
        close(sockfd);
        return 0;
    }

    /* Get information about IP address used for the connection */
    if (getsockname(sockfd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
        close(sockfd);
        return 0;
    }

    int sync_code = htonl_o(0);

    /* Sent sync command to begin synchronization */
    if (send(sockfd, &sync_code, sizeof(int), 0) < 0) {
        close(sockfd);
        return 0;
    }

    int recv_bytes = 0;

    int status;
    if ((recv_bytes = recv(sockfd, &status, sizeof(int), MSG_WAITALL)) < 0) {
        close(sockfd);
        return 0;
    } else if (recv_bytes == 0) {
        close(sockfd);
        return 0;
    }

    if (ntohl_o(status) != 1) {
        close(sockfd);
        return 0;
    }

    return 1;
}

void *connection_handler(void *data) {
    int* sockfd = (int*)data;

    struct sockaddr_in client_addr, server_addr, test_addr;
    int addr_len = sizeof(struct sockaddr);
    memset(&client_addr, 0, addr_len);
    memset(&server_addr, 0, addr_len);
    memset(&test_addr, 0, addr_len);

    /* get address of the server which handle the connection */
    if (getsockname(*sockfd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
        perror("get socket name failed");
        close(*sockfd);
        free(sockfd);
        pthread_exit(NULL);
    }
    printf("Server address: %s. Server Port: %d\n", 
        inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    /* get address of client server communicates with */
    if (getpeername(*sockfd, (struct sockaddr *)&client_addr, &addr_len) < 0) {
        perror("get peer address failed");
        close(*sockfd);
        free(sockfd);
        pthread_exit(NULL);
    }

    int recv_bytes = 0;

    int operation;
    if ((recv_bytes = recv(*sockfd, &operation, sizeof(int), MSG_WAITALL)) < 0) {
        perror("recv operation code while handling connection");
        close(*sockfd);
        free(sockfd);
        pthread_exit(NULL);
    } else if (recv_bytes == 0) {
        printf("stream socket peer has performed an orderly shutdown 4\n");
        close(*sockfd);
        free(sockfd);
        pthread_exit(NULL);
    }

    int status = htonl_o(1);

    /* Sent sync command to begin synchronization */
    if (send(*sockfd, &status, sizeof(int), 0) < 0) {
        perror("send ready to sync status failed");
        close(*sockfd);
        free(sockfd);
        pthread_exit(NULL);
    }

    if (ntohl_o(operation) == 1) {
        char peer_name[NAME_LENGTH];
        char peer_addr[ADDRESS_LENGTH];
        int peer_port = 0;

        memset(peer_name, 0, sizeof(peer_name));
        memset(peer_addr, 0, sizeof(peer_addr));

        printf("Sync operation received\n");

        char *ptr;

        char buf_intro[INTRODUCTION_LENGTH];
        memset(buf_intro, 0, sizeof(buf_intro));
        if ((recv_bytes = recv(*sockfd, buf_intro, sizeof(buf_intro), MSG_WAITALL)) < 0) {
            perror("recv introduction message while handling sync operation failed");
            close(*sockfd);
            free(sockfd);
            pthread_exit(NULL);
        } else if (recv_bytes == 0) {
            printf("stream socket peer has performed an orderly shutdown 5\n");
            close(*sockfd);
            free(sockfd);
            pthread_exit(NULL);
        }
        printf("Introduction message received: %s\n", buf_intro);

        ptr = strtok(buf_intro, ":");
        memcpy(peer_name, ptr, strlen(ptr));

        ptr = strtok(NULL, ":");
        memcpy(peer_addr, ptr, strlen(ptr));

        ptr = strtok(NULL, ":");
        peer_port = atoi(ptr);
        
        struct Node* node;
        if ((node = get_known_peer(peer_addr, peer_port)) == NULL) {
            pthread_mutex_lock(&known_nodes_lock);
            list_insert(peer_name, peer_addr, peer_port, known_nodes);
            pthread_mutex_unlock(&known_nodes_lock);
            printf("New node. Was added to the list of known nodes\n");
        } else if (strlen(node->name) == 0) {
            printf("Added name %s for address %s, port %d\n", 
                peer_name, peer_addr, peer_port);
            pthread_mutex_lock(&known_nodes_lock);
            memcpy(node->name, peer_name, sizeof(peer_name));
            pthread_mutex_unlock(&known_nodes_lock);
        }

        int number_of_nodes = 0;
        char buf_peer[PEER_INFORMATION_LENGTH];
        if ((recv_bytes = recv(*sockfd, &number_of_nodes, sizeof(int), MSG_WAITALL)) < 0) {
            perror("recv number of nodes while handling sync operation failed");
            close(*sockfd);
            free(sockfd);
            pthread_exit(NULL);
        } else if (recv_bytes == 0) {
            printf("stream socket peer has performed an orderly shutdown 6\n");
            close(*sockfd);
            free(sockfd);
            pthread_exit(NULL);
        }

        for (int i = 0; i < ntohl_o(number_of_nodes); ++i) {
            /* Initialization of variables for peer data */
            memset(buf_peer, 0, sizeof(buf_peer));
            memset(peer_name, 0, sizeof(peer_name));
            memset(peer_addr, 0, sizeof(peer_addr));
            peer_port = 0;

            if ((recv_bytes = recv(*sockfd, buf_peer, sizeof(buf_peer), MSG_WAITALL)) < 0) {
                perror("recv node information while handling sync operation failed");
                close(*sockfd);
                free(sockfd);
                pthread_exit(NULL);
            } else if (recv_bytes == 0) {
                printf("stream socket peer has performed an orderly shutdown 7\n");
                close(*sockfd);
                free(sockfd);
                pthread_exit(NULL);
            }

            if (buf_peer[0] != ':') {
                ptr = strtok(buf_peer, ":");
                memcpy(peer_name, ptr, strlen(ptr));

                ptr = strtok(NULL, ":");
                memcpy(peer_addr, ptr, strlen(ptr));
            } else {
                ptr = strtok(buf_peer, ":");
                memcpy(peer_addr, ptr, strlen(ptr));
            }

            ptr = strtok(NULL, ":");            
            peer_port = atoi(ptr);

            inet_aton(peer_addr, &test_addr.sin_addr);
            test_addr.sin_port = htons(peer_port);
            if (test_addr.sin_port == server_addr.sin_port && memcmp(&test_addr.sin_addr, &server_addr.sin_addr, sizeof(struct in_addr)) == 0) {
                continue;
            }
            
            if ((node = get_known_peer(peer_addr, peer_port)) == NULL) {
                if (!check_node(peer_addr, peer_port)) {
                    continue;
                }

                pthread_mutex_lock(&known_nodes_lock);
                list_insert(peer_name, peer_addr, peer_port, known_nodes);
                pthread_mutex_unlock(&known_nodes_lock);

                printf("New peer %s with address %s and port %d was added to the list of known nodes\n",
                    peer_name, peer_addr, peer_port);
            } else if (strlen(node->name) == 0) {

                printf("Change name of peer for address %s, port %d\n", 
                    peer_addr, peer_port);
                pthread_mutex_lock(&known_nodes_lock);
                memcpy(node->name, peer_name, sizeof(peer_name));
                pthread_mutex_unlock(&known_nodes_lock);
            }
        }
    } else {
        printf("Wrong command got: %d\n", ntohl_o(operation));
    }

    close(*sockfd);
    free(sockfd);
    pthread_exit(NULL);
}

void* setup_tcp_server_communication(void *data) {
    /*
     * Step 1 : Initialization
     * Socket handle and other variables
     * Master socket file descriptor, used to accept new client connection only, no data exchange
     */
    int master_sock_tcp_fd = 0,
        addr_len = sizeof(struct sockaddr);

    /* Client specific communication socket file descriptor, 
     *  used for only data exchange/communication between client and server
     */
    int comm_socket_fd = 0;

    /* Variables to hold server information */
    struct sockaddr_in server_addr, client_addr; //structure to store the server and client info
    memset(&server_addr, 0, addr_len);
    memset(&client_addr, 0, addr_len);

    /* Step 2: tcp master socket creation */
    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket creation in main server thread failed");
        exit(EXIT_FAILURE);
    }

    /* Step 3: specify server information */
    server_addr.sin_family = AF_INET;   // This socket will process only ipv4 network packets
    server_addr.sin_port = htons(0); // Port is randomly assigned
    server_addr.sin_addr.s_addr = INADDR_ANY; // Server's IP address

    /* Bind the server.
     * bind() is a mechnism to tell OS what kind of data server process is interested in to recieve. 
     * Remember, server machine can run multiple server processes to process different data 
     *  and service different clients.
     */

    if (bind(master_sock_tcp_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0) {
        perror("socket bind in main server thread failed");
        exit(EXIT_FAILURE);
    }

    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&server_addr, &addr_len) < 0) {
        perror("getsockname in main server thread failed");
        exit(EXIT_FAILURE);
    }
    printf("Server was successfully opened on port %d\n", ntohs(server_addr.sin_port));
    server_port = ntohs(server_addr.sin_port);

    pthread_t synch_thread;
    pthread_create(&synch_thread, NULL, setup_tcp_communication, NULL);

    /*
     * Step 4 : Tell the Linux OS to maintain the queue of max length to Queue incoming
     *  client connections.
     */
    if (listen(master_sock_tcp_fd, CONNECTIONS_NUMBER) < 0) {
        perror("listen in main server thread failed"); 
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections...\n");

    /* Server infinite loop for servicing the client */
    while(TRUE) {
        /* Step 6 : Wait for client connection */
        comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *)&client_addr, &addr_len);

        /* if accept failed to return a socket descriptor, display error and exit */
        if(comm_socket_fd < 0) {
            perror("accept error");
            continue;
        }

        printf("Connection accepted from client : %s:%u\n", 
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t conn_thread;
        int *thread_arg = malloc(sizeof(int));
        *thread_arg = comm_socket_fd;
        pthread_create(&conn_thread, NULL, connection_handler, (void *)thread_arg);
    }

    return NULL;
}

int main() {
    if (pthread_mutex_init(&known_nodes_lock, NULL) != 0) {
        perror("known nodes mutex init failed");
        exit(EXIT_FAILURE);
    }

    known_nodes = list_create();

    char shouldWait;

    printf("Wait for connection? Write Y or N\n");
    scanf("%c", &shouldWait);

    if (shouldWait == 'N') {
        char addr[ADDRESS_LENGTH];
        char *ip_address;
        int port;

        memset(addr, 0, sizeof(addr));

        printf("Write an IP address and port of the peer to connect in form: \n\"ip_address:port\"\n");
        scanf("%s", addr);
        ip_address = strtok(addr, ":");
        port = atoi(strtok(NULL, "\n"));
        list_insert("", ip_address, port, known_nodes);
    } else if (shouldWait != 'Y') {
        printf("Unknown answer. Process terminates\n");
        exit(EXIT_FAILURE);
    }

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, setup_tcp_server_communication, NULL);

    pthread_join(server_thread, NULL);

    return 0;
}