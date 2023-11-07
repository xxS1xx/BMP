#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "config2.h"

struct SetupArgs{
    char *filename;
    long file_size;
};

struct RoutineArgs{
    int client_fd;
    int num_of_blocks;
    int num_of_blocks_last;
}; 

void* setup(void *args);

int main(){
    int status, valread, client_fd, total_num_of_blocks, num_of_blocks_per_chunk, num_of_blocks_per_chunk_last;
    long file_size = 0;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char buffer[1024] = { 0 };
    char filename[1024] = { 0 };

    // Setup
    struct SetupArgs setupArgs;
    setup(&setupArgs);
    file_size = setupArgs.file_size;
    strcpy(filename,setupArgs.filename);
    printf("File size: %ld\n\n", file_size);

    // --------------------------------------------------------------------

    // struct RoutineArgs routineArgs[WORKERS];
    // pthread_t th[WORKERS];

    // for(int i=0; i<WORKERS; i++){
    //     routineArgs[i].num_of_blocks = num_of_blocks_per_chunk;
    //     routineArgs[i].num_of_blocks_last = num_of_blocks_per_chunk_last;
    //     if(pthread_create(&th[i], NULL, &routine, &routineArgs[i]) !=0 ){
    //         perror("Thread creation error");
    //         exit(-1);
    //     }
    // }

    // for(int i=0; i<WORKERS; i++){
    //     if(pthread_join(th[i], NULL) != 0){
    //         perror("Thread joining error");
    //         exit(-1);
    //     }
    // }
}

void* setup(void *args){
    int status, valread, client_fd;
    long file_size=0;
    struct sockaddr_in serv_addr;
    char *connected="Client connected successfully.";
    char buffer[1024] = { 0 };
    struct SetupArgs *setupArgs = (struct SetupArgs*)args;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error\n");
        exit(-1);
    }


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Conver IPv4 to IPv6 addresses from text to binary form
    // if ( inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0 ) {
    //     perror("Invalid address");
    //     exit(-1);
    // }

    // ********** Server IP Address here **********
    if ( inet_pton(AF_INET, SERVERIP, &serv_addr.sin_addr) <= 0 ) {
        perror("Invalid address");
        exit(-1);
    }

    if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection failed");
        exit(-1);
    }

    // client connected successfully
    send(client_fd, connected, strlen(connected), 0);

    valread = read(client_fd, buffer, 1024);
    printf("%s\n\n", buffer);

    // Enter filename to be send
    char filename[1024];
    memset(buffer,'\0',sizeof(buffer));
    valread = read(client_fd, buffer, 1024);
    printf("%s\n", buffer);
    scanf("%s", filename);
    send(client_fd, filename, strlen(filename), 0);

    // Recieve file size
    valread = read(client_fd, buffer, 1024);
    sscanf(buffer,"%ld", &file_size);
    setupArgs->file_size = file_size;
    setupArgs->filename = filename;

    // closing the connected socket
    close(client_fd);

    return NULL;
}

void* routine(void* args){
    int valread, client_fd, num_of_blocks;
    char hello[200];
    char output[200];
    char buffer[BLOCKSIZE] = { 0 };
    char buffer2[10] = { 0 };
    char temp[200];

    struct RoutineArgs *routineArgs = (struct RoutineArgs*)args;
    client_fd = routineArgs->client_fd;

    sprintf(hello, "Hello from client{sock_num: %d, thread_id: %lu}", client_fd, (unsigned long)pthread_self());

    send(client_fd, hello, strlen(hello), 0);

    // pthread_self gives currrent thread ID
    // printf("Hello message send{sock_num: %d, thread_id: %lu}\n", client_fd, (unsigned long)pthread_self());

    // Receiving index of block
    // Problem here why to keep 9 instead of 10
    int idx=-1;
    valread = read(client_fd,buffer2,9);
    sscanf(buffer2,"%d",&idx);
    // printf("Index: %s %d\n", buffer2, idx);
    memset(buffer2,'\0',9);

    // Receiving Block
    if(idx!=WORKERS-1){
        num_of_blocks = routineArgs->num_of_blocks;
    }else{
        num_of_blocks = routineArgs->num_of_blocks_last;
    }


    sprintf(output, "Output%d", idx);
    strcpy(temp,OUPUTPATH);
    strcat(temp,output);
    strcpy(output,temp);

    FILE *file = fopen(output,"wb");
    if(!file){
        perror("Error in opening output folder");
        exit(-1);
    }

    for(int i=0; i<num_of_blocks; i++){
        valread = read(client_fd, buffer, BLOCKSIZE);
        // printf("%d - %s\n", idx, buffer);
        fwrite(buffer, sizeof(char), valread, file);
        memset(buffer,'\0',BLOCKSIZE); // try this after commenting
    }
    
    fclose(file);

    return NULL;
}