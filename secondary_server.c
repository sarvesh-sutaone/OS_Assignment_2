#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#define MAX_VERTICES 30

struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int res[100];
    int rows;   
};

struct message{
    long mtype;
    int seq_num;
    int op_num;
    int res[100];
    char fname[100];
}typedef Message;


typedef struct {
    int (*graph)[MAX_VERTICES];
    bool* visited;
    int* queue;
    int startNode;
    int levelSize;
    int numNodes; // New field for the number of nodes
    int* rear;    // New field for the rear pointer
} ThreadArgs;


int create_shm(int val){
    key_t key = ftok(".",val);
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    printf("Vlaue of val: %d\n",val);
    printf("Vlaue of key: %d\n",key);

    int shmid = shmget(key,sizeof(struct sharedData),IPC_CREAT);

    if(shmid == -1){
        perror("Error in shmget y");
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

// Function to perform Breadth-First Search traversal
void BFS(int graph[MAX_VERTICES][MAX_VERTICES], int startNode, int numNodes);

// Thread function for BFS traversal at a specific level
void* BFSLevel(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int (*graph)[MAX_VERTICES] = threadArgs->graph;
    bool* visited = threadArgs->visited;
    int* queue = threadArgs->queue;
    int startNode = threadArgs->startNode;
    int levelSize = threadArgs->levelSize;
    int numNodes = threadArgs->numNodes; // Access numNodes from ThreadArgs
    int* rear = threadArgs->rear;        // Access rear pointer from ThreadArgs

    for (int levelNode = 0; levelNode < levelSize; ++levelNode) {
        int currentNode = queue[levelNode];
        printf("%d ", currentNode);

        // Explore adjacent nodes
        for (int i = 1; i <= numNodes; i++) {
            if (graph[currentNode - 1][i - 1] == 1 && !visited[i]) {
                // Enqueue the adjacent node and mark it as visited
                queue[++(*rear)] = i;
                visited[i] = true;
            }
        }
    }

    // Release thread resources
    pthread_exit(NULL);
}

void BFS(int graph[MAX_VERTICES][MAX_VERTICES], int startNode, int numNodes) {
    // Array to keep track of visited nodes
    bool visited[MAX_VERTICES + 1];

    // Initialize all nodes as not visited
    for (int i = 0; i <= MAX_VERTICES; i++) {
        visited[i] = false;
    }

    // Queue to store nodes for BFS traversal
    int queue[MAX_VERTICES + 1];
    int front = -1, rear = -1;

    // Enqueue the starting node and mark it as visited
    queue[++rear] = startNode;
    visited[startNode] = true;

    while (front < rear) {
        int currentNode = queue[++front];
        printf("%d ", currentNode);

        // Explore adjacent nodes
        for (int i = 1; i <= numNodes; i++) {
            if (graph[currentNode - 1][i - 1] == 1 && !visited[i]) {
                // Enqueue the adjacent node and mark it as visited
                queue[++rear] = i;
                visited[i] = true;
            }
        }
    }
}


// Function to read graph from a file
void readGraphFromFile(const char* filename, int graph[MAX_VERTICES][MAX_VERTICES], int* numNodes) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Read the number of nodes
    fscanf(file, "%d", numNodes);

    // Read the adjacency matrix
    for (int i = 0; i < *numNodes; i++) {
        for (int j = 0; j < *numNodes; j++) {
            fscanf(file, "%d", &graph[i][j]);
        }
    }

    fclose(file);
}

void *starterFunction(void *args){
    Message* msg = (Message*)args;
    int numNodes;
    int graph[MAX_VERTICES][MAX_VERTICES];

    char filename[100];
    strcpy(filename,msg->fname);
    readGraphFromFile(filename,graph,&numNodes);
    printf("%d\n",msg->seq_num);
    int shmid = create_shm(1);
    struct sharedData client_data;
    read_from_shared_memory(shmid,&client_data);
    int startNode = client_data.start_vertex;
    
    printf("starting: %d\n",startNode);

    if(msg->op_num == 4){
        BFS(graph,startNode,numNodes);
    }

    if(msg->op_num == 3){
        // Perform DFS
    }
    pthread_exit(NULL);
}

int main(){
    // int ss_identifier = -1;
    // key_t key_for_server = ftok(".",'s');
    // // int shmid = shmget(key_for_server,sizeof(struct sharedData),0664);
    // // delete_shm(shmid);
    // if(key_for_server == -1){
    //     perror("Error in ftok");
    //     exit(EXIT_FAILURE);
    // }

    int msgid = create_message_queue();
    printf("Message id created\n");
    struct message msg;
    // int shmid = shmget(key_for_server,sizeof(struct sharedData),0664);
    msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 999, 0);
    int shmid = create_shm(1);
    // if(shmid == -1){
    //     printf("This is Secondary Server 1\n");
    //     shmid = shmget(key_for_server,sizeof(struct sharedData),0664|IPC_CREAT);
    //     ss_identifier = 1; // First Secondary Server
    //     msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 999, 0);
    // }
    // else{
    //     printf("This is Secondary Server 2\n");
    //     ss_identifier = 2; // Second Secondary Server
    //     msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 998, 0);
    // }

    // pthread_t thread;

    // if(pthread_create(&thread,NULL,starterFunction,(void*)&msg) != 0)
    // {
    //     perror("Error in creating thread");
    //     exit(EXIT_FAILURE);
    // }

    // pthread_join(thread,NULL);
    
}

// The above code solves the problem of two secondary servers.