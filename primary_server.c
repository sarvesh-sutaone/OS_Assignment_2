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
    long mtype;
    int seq_num;
    int op_num;
    int res[100];
    char fname[100];
}typedef Message;

struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int rows;   
};

int create_shm(int val){
    key_t key = ftok(".",val);
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key,sizeof(struct sharedData),0664);

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
    
    int shmid = create_shm(msg->seq_num);
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
        strcpy(msg->fname,"File created successfully.\n");
    }

    if(msg->op_num == 2){
        strcpy(msg->fname,"File modified successfully.\n");
    }

    pthread_exit(NULL);

}

int create_message_queue() {
    key_t key = ftok(".", 'B');
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }
    return msgid;
}
void delete_message_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Error deleting message queue");
        exit(EXIT_FAILURE);
    }
    printf("Message queue deleted\n");
}

int main(){
    int msgid = create_message_queue();
    printf("Connection established successfully!\n");

    // Main loop to handle messages
    while (1) 
    {
        struct message msg;
        // Receive message from the queue
        msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 1000, 0);
        printf("Request Received\n");    
        pthread_t thread;
        
        if(msg.op_num==5)
        {
        	// Server exit Message received
        	break;
        }

        if(pthread_create(&thread,NULL,handleRequest,(void*)&msg) != 0)
        {
            perror("Error in creating thread");
            exit(EXIT_FAILURE);
        }

        pthread_join(thread,NULL);
        msg.mtype = msg.seq_num;
        if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
            perror("Error sending message to client from primary server");
            exit(EXIT_FAILURE);
        }
        printf("Request Processed\n");
    }
    
    printf("\nPrimary Server Gracefully Exiting\n"); 
}
