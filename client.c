#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
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

int main(){
    key_t key;
    int msgqid;
    key = ftok(".",'B');
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    msgqid = msgget(key,0666);
    if(msgqid == -1){
        perror("Error in msgget");
        exit(EXIT_FAILURE);
    }

    printf("1. Add a new to graph to the database\n");
    printf("2. Modify an existing graph of the database.\n");
    printf("3. Perform DFS on existing graph of the database. \n");
    printf("4. BFS on existing graph of the database.\n");
    printf("Enter Sequence Number: ");
    int sequence_number;
    scanf("%d",&sequence_number);
    printf("Enter Operation Number: ");
    int operation;
    scanf("%d",&operation);
    printf("Enter Graph File Name: ");
    char graph_name[100];
    scanf("%s",graph_name);

    Message msg;
    msg.seq_num =  sequence_number;
    msg.op_num = operation;
    msg.mtype = 1;
    strcpy(msg.fname,graph_name);
    if(msgsnd(msgqid,&msg,sizeof(struct message) - sizeof(long),0) == -1){
        perror("Error in msgsnd");
        exit(EXIT_FAILURE);
    }

    if(operation == 1){ // Add new graph
        printf("Enter number of nodes of the graph: ");
        int num_nodes;
        scanf("%d",&num_nodes);
        int **graph = (int**)malloc(MAX_VERTICES*sizeof(int*));
        for(int i=0;i<num_nodes;++i){
            graph[i] = (int*)malloc(MAX_VERTICES*sizeof(int));
        }

        printf("Enter adjacency matrix:\n");
        for(int i=0;i<num_nodes;++i){
            for(int j=0;j<num_nodes;++j){
                scanf("%d",&graph[i][j]);
            }
        }
        

        // Create a SHM and store in it.
        struct sharedData client_data;
        client_data.flag = 0; // To indicate this is sent by client.
        client_data.rows = num_nodes;
        client_data.start_vertex = -1;
        for(int i=0;i<num_nodes;++i){
            for(int j=0;j<num_nodes;++j){
                client_data.graph[i][j] = graph[i][j];
            }
        }
        int shmid = create_shm(sequence_number);
        write_to_shm(shmid,&client_data);
        
        //Receieve response from Primary Server regarding Success and delete SHM.
    }

    else if(operation == 2){// Modify existing
        printf("Enter number of nodes of the graph: ");
        int num_nodes;
        scanf("%d",&num_nodes);
        int **graph = (int**)malloc(MAX_VERTICES*sizeof(int*));
        for(int i=0;i<num_nodes;++i){
            graph[i] = (int*)malloc(MAX_VERTICES*sizeof(int));
        }

        printf("Enter adjacency matrix:\n");
        for(int i=0;i<num_nodes;++i){
            for(int j=0;j<num_nodes;++j){
                scanf("%d",&graph[i][j]);
            }
        }

        // Send to SHM.
        struct sharedData client_data;
        client_data.flag = 0; // To indicate this is sent by client.
        client_data.rows = num_nodes;
        client_data.start_vertex = -1;
        for(int i=0;i<num_nodes;++i){
            for(int j=0;j<num_nodes;++j){
                client_data.graph[i][j] = graph[i][j];
            }
        }
        int shmid = create_shm(sequence_number);
        write_to_shm(shmid,&client_data);

        // Receive response from Primary server regarding successful modification and delete SHM.
    }

    else if(operation == 3){// DFS
        printf("Enter starting vertex:");
        int start_vertex;
        scanf("%d",&start_vertex);

        // Send to SHM.
        struct sharedData client_data;
        client_data.flag = 0; // To indicate this is sent by client.
        client_data.rows = 0;
        client_data.start_vertex = start_vertex;
        int shmid = create_shm(sequence_number);
        write_to_shm(shmid,&client_data);

        // Send message through message queue.

        // Receive message through message queue.
    }

    else if(operation == 4){// BFS
        printf("Enter starting vertex:");
        int start_vertex;
        scanf("%d",&start_vertex);

        // Send to SHM.
        struct sharedData client_data;
        client_data.flag = 0; // To indicate this is sent by client.
        client_data.rows = 0;
        client_data.start_vertex = start_vertex;
        int shmid = create_shm(sequence_number);
        write_to_shm(shmid,&client_data);

        // Send message through message queue.

        // Receieve message through message queue and delete SHM.
    }

    else{
        printf("Invalid Operation");
    }
}