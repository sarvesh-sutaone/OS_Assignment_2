#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <pthread.h>
#define MAX_VERTICES 30

struct message{
    int seq_num;
    int op_num;
    int res[100];
    int mtype;
    char fname[100];
}typedef Message;

struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int rows;   
};

int create_shm(int val){
    key_t key = ftok(".",'a');
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key,sizeof(struct sharedData),IPC_CREAT|0664);

    if(shmid == -1){
        perror("Error in shmget");
        exit(EXIT_FAILURE);
    }

    return shmid;
}

void write_to_shm(int shmid, struct sharedData* shared_data){
    struct sharedData* shared_mem = (struct sharedData*)shmat(shmid,NULL,0);
    
    if((void*)shared_mem == (void*)-1){
        perror("Error in shmat");
        exit(EXIT_FAILURE);
    }

    memcpy(shared_mem,shared_data,sizeof(struct sharedData));
    shmdt(shared_mem);
}

void read_from_shared_memory(int shmid, struct sharedData* shared_data) {
    struct sharedData* shared_mem = (struct sharedData*)shmat(shmid, NULL, 0); 

    if ((void*)shared_mem == (void*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    memcpy(shared_data, shared_mem, sizeof(struct sharedData));

    shmdt(shared_mem);
}

void delete_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Error in shmctl");
        exit(EXIT_FAILURE);
    }
}

void *handleRequest(void *arg){
    Message* msg = (Message*)arg;

    struct sharedData client_data;
    int shmid = create_shm(1);
    read_from_shared_memory(shmid,&client_data);
    
    char fileName[100];
    strcpy(fileName,msg->fname);
    FILE *file = fopen(fileName,"w");

    if(file == NULL){
        perror("Error in creating file");
        exit(EXIT_FAILURE);
    }

    fprintf(file,"%d\n",client_data.rows);

    for(int i=0;i<client_data.rows;++i){
        for(int j=0;j<client_data.rows;++j){
            fprintf(file, "%d ",client_data.graph[i][j]);
        }
        fprintf(file,"\n");
    }

    fclose(file);

    // This thread will send back the message to client through message queue.
    if(msg->op_num == 1){
        printf("File created successfully.");
    }

    if(msg->op_num == 2){
        printf("File modified successfully");
    }


}

int main(){
    // Assume it receives write request from LB via single queue.
    Message msg;
    msg.op_num = 2; // From Msg Queue
    msg.seq_num = 1; // From Msg Queue
    strcpy(msg.fname,"G1.txt"); // From Msg Queue
    pthread_t thread;

    if(pthread_create(&thread,NULL,handleRequest,(void*)&msg) != 0){
        perror("Error in creating thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread,NULL);
}