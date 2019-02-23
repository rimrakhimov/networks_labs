//Taken from Abhishek Sagar

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "common.h"
#include <arpa/inet.h>

/*Server process is running on this port no. Client has to send data to this port no*/
#define SERVER_PORT     2000
#define THREAD_COUNTS 10

void *connection_handler(void *data) {
    student_struct_t *client_data = &(((thread_args_struct*)(data))->student_data);

    student_result_struct_t result;
    parse_input(client_data, &result.information);

    /* Server replying back to client now*/
    int sent_recv_bytes = sendto(socketfd, (char *)&result, sizeof(student_result_struct_t), 
        0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

    printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
}

student_struct_t test_struct;
result_struct_t res_struct;
char data_buffer[1024];

pthread_t threads[THREAD_COUNTS];
int thread_used[THREAD_COUNTS];
thread_args_struct thread_arguments[THREAD_COUNTS];

void parse_input(student_struct_t *inp_buffer, char *result) {
    int total_length = 0;
    int i;
    i = 0;
    while ((result[total_length] = inp_buffer->first_name[i]) != '\0') {
        total_length++;
        i++;
    }
    result[total_length++] = ',';
    result[total_length++] = ' ';
    i = 0;
    while ((result[total_length] = inp_buffer->second_name[i]) != '\0') {
        total_length++;
        i++;
    }
    result[total_length++] = ',';
    result[total_length++] = ' ';
    i = 1;
    while ((inp_buffer->age / i) != 0) {
        i *= 10;
    }
    for (i/=10; i>0; i/=10) {
        result[total_length] = (inp_buffer->age / i) % 10 + 48;
        total_length++;
    }
    result[total_length++] = ',';
    result[total_length++] = ' ';
    i = 0;
    while ((result[total_length] = inp_buffer->group[i]) != '\0') {
        total_length++;
        i++;
    }
}

void setup_udp_server_communication(){

   /*Step 1 : Initialization*/
   /*Socket handle and other variables*/
   /*Master socket file descriptor, used to accept new client connection only, no data exchange*/
   int socketfd = 0, 
       sent_recv_bytes = 0, 
       addr_len = 0, 
       free_thread_index = 0;

   /*variables to hold server information*/
   struct sockaddr_in server_addr, /*structure to store the server and client info*/
                      client_addr;

   /*step 2: tcp master socket creation*/
   if ((socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1)
   {
       printf("socket creation failed\n");
       exit(1);
   }

   /*Step 3: specify server Information*/
   server_addr.sin_family = AF_INET;/*This socket will process only ipv4 network packets*/
   server_addr.sin_port = SERVER_PORT;/*Server will process any data arriving on port no 2000*/
   
   /*3232249957; //( = 192.168.56.101); Server's IP address, 
   //means, Linux will send all data whose destination address = address of any local interface 
   //of this machine, in this case it is 192.168.56.101*/
   server_addr.sin_addr.s_addr = INADDR_ANY; 

   addr_len = sizeof(struct sockaddr);

   /* Bind the server. Binding means, we are telling kernel(OS) that any data 
    * you recieve with dest ip address = 192.168.56.101, and tcp port no = 2000, pls send that data to this process
    * bind() is a mechnism to tell OS what kind of data server process is interested in to recieve. Remember, server machine
    * can run multiple server processes to process different data and service different clients. Note that, bind() is 
    * used on server side, not on client side*/

   if (bind(socketfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
   {
       printf("socket bind failed\n");
       return;
   }

   /* Server infinite loop for servicing the client*/

    while(1){
        printf("Server ready to service clients msgs.\n");
        /*Drain to store client info (ip and port) when data arrives from client, sometimes, server would want to find the identity of the client sending msgs*/
        memset(data_buffer, 0, sizeof(data_buffer));

        /*Like in client case, this is also a blocking system call, meaning, server process halts here untill
        * data arrives on this comm_socket_fd from client whose connection request has been accepted via accept()*/
        /* state Machine state 3*/
        sent_recv_bytes = recvfrom(socketfd, (char *)data_buffer, sizeof(data_buffer), 0,
                (struct sockaddr *)&client_addr, &addr_len);

        /* state Machine state 4*/
        printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes, 
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int i = free_thread_index;
        memcpy(&(thread_arguments[i].student_data), data_buffer, sizeof(student_struct_t));
        thread_arguments[i].tid = i;
        pthread_create(&threads[free_thread_index], NULL, connection_handler, (void *)(&thread_arguments[i]));

        i++;
        while(thread_used[i]) {
            i = (i+1) % THREAD_COUNTS;
        }
        free_thread_index = i;
    } /*step 10 : wait for new client request again*/    
}

int main(int argc, char **argv){

   setup_udp_server_communication();
   return 0;
}