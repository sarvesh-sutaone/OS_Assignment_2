// Including relevant headers and defining some global variables
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
int visited[MAX_VERTICES];
int lastVertex; 
int dfsRes[MAX_VERTICES];
int dfsIndex;

// Structure for data shared in SHM
struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int rows;   
};

// Structure for data shared in Message Queue
struct message{
    long mtype;
    int seq_num;
    int op_num;
    int res[100];
    char fname[100];
}typedef Message;

// Thread Argument Structure for BFS
typedef struct {
    int (*graph)[MAX_VERTICES];
    bool* visited;
    int* queue;
    int* front;
    int* rear;
    int* traversalResult;
    int numNodes;
} ThreadArgs;

// Thread Argument Structure for DFS
struct DFSThreadArgs {
    int adjMatrix[MAX_VERTICES][MAX_VERTICES];
    int vCount;
    int start;
};

// Function to create SHM given a value.
int create_shm(int val){
    key_t key = ftok(".",val);
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }
    int shmid = shmget(key,sizeof(struct sharedData),IPC_CREAT);
    if(shmid == -1){
        perror("Error in shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

// Function to write in a given SHM
void write_to_shm(int shmid, struct sharedData* shared_data){
    struct sharedData* shared_mem = (struct sharedData*)shmat(shmid,NULL,0);
    if((void*)shared_mem == (void*)-1){
        perror("Error in shmat");
        exit(EXIT_FAILURE);
    }
    memcpy(shared_mem,shared_data,sizeof(struct sharedData));
    shmdt(shared_mem);
}

// Structure to read in a given SHM.
void read_from_shared_memory(int shmid, struct sharedData* shared_data) {
    struct sharedData* shared_mem = (struct sharedData*)shmat(shmid, NULL, 0);
    if ((void*)shared_mem == (void*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    memcpy(shared_data, shared_mem, sizeof(struct sharedData));
    shmdt(shared_mem);
}

// Structure to delete a given SHM.
void delete_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("Error in shmctl");
        exit(EXIT_FAILURE);
    }
}

// Function to retreive a message queue with a given key.
int create_message_queue() {
    key_t key = ftok(".", 'B');
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }
    return msgid;
}

// Function to delete message queue with a given message id.
void delete_message_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Error deleting message queue");
        exit(EXIT_FAILURE);
    }
    printf("Message queue deleted\n");
}

// Function to perform Breadth-First Search traversal
void BFS(int graph[MAX_VERTICES][MAX_VERTICES], int startNode, int numNodes,int* traversalResult);

// Function to perform BFS.
void* BFSLevel(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int (*graph)[MAX_VERTICES] = threadArgs->graph;
    bool* visited = threadArgs->visited;
    int* queue = threadArgs->queue;
    int* front = threadArgs->front;
    int* rear = threadArgs->rear;
    int* traversalResult = threadArgs->traversalResult;
    int numNodes = threadArgs->numNodes;
 
    while (*front < *rear) {
        int levelNode = queue[++(*front)];
        traversalResult[*front] = levelNode;
 
        for (int i = 1; i <= numNodes; i++) {
            if (graph[levelNode - 1][i - 1] == 1 && !visited[i]) {
                queue[++(*rear)] = i;
                visited[i] = true;
            }
        }
    }
 
    pthread_exit(NULL);
}

// Function to initialise the thread argument structure.
void initThreadArgs(ThreadArgs* args, int (*graph)[MAX_VERTICES], bool* visited, int* queue, int* front, int* rear, int* traversalResult, int numNodes) {
    args->graph = graph;
    args->visited = visited;
    args->queue = queue;
    args->front = front;
    args->rear = rear;
    args->traversalResult = traversalResult;
    args->numNodes = numNodes;
}

// Function which performs BFS
void BFS(int graph[MAX_VERTICES][MAX_VERTICES], int startNode, int numNodes, int* traversalResult) {
    bool visited[MAX_VERTICES + 1];
 
    for (int i = 0; i <= MAX_VERTICES; i++) {
        visited[i] = false;
    }
 
    int queue[MAX_VERTICES + 1];
    int front = -1, rear = -1;
 
    queue[++rear] = startNode;
    visited[startNode] = true;
 
    while (front < rear) {
        int levelSize = rear - front;
 
        pthread_t threads[levelSize];
        ThreadArgs threadArgs[levelSize];
 
        for (int levelNode = 0; levelNode < levelSize; ++levelNode) {
            initThreadArgs(&threadArgs[levelNode], graph, visited, queue, &front, &rear, traversalResult, numNodes);
            if (pthread_create(&threads[levelNode], NULL, BFSLevel, (void*)&threadArgs[levelNode]) != 0) {
                perror("Error in creating thread");
                exit(EXIT_FAILURE);
            }
        }
 
        for (int levelNode = 0; levelNode < levelSize; ++levelNode) {
            pthread_join(threads[levelNode], NULL);
        }
    }
}

// Mutex for thread-safe operations
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to perform DFS
void dfs(int adjMatrix[][MAX_VERTICES], int vCount, int start);

// Thread function for DFS 
void *dfsThread(void *args);

// Function to perform DFS
void dfs(int adjMatrix[][MAX_VERTICES], int vCount, int start) {
    pthread_t thread[MAX_VERTICES];  
    struct DFSThreadArgs *threadArgs[MAX_VERTICES];

    pthread_mutex_lock(&mutex);
    visited[start] = 1;
    pthread_mutex_unlock(&mutex);

    int isLeaf = 1;  
    int threadCount = 0;

    for (int i = 0; i < vCount; i++) {
        if (adjMatrix[start][i] && !visited[i]) {
            isLeaf = 0;

            threadArgs[threadCount] = malloc(sizeof(struct DFSThreadArgs));
            if (threadArgs[threadCount] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            threadArgs[threadCount]->vCount = vCount;
            threadArgs[threadCount]->start = i;
            for (int j = 0; j < MAX_VERTICES; j++) {
                for (int k = 0; k < MAX_VERTICES; k++) {
                    threadArgs[threadCount]->adjMatrix[j][k] = adjMatrix[j][k];
                }
            }

            if (pthread_create(&thread[threadCount], NULL, dfsThread, (void *)threadArgs[threadCount]) != 0) {
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
            }
            threadCount++;
        }
    }

    if (isLeaf) {
        pthread_mutex_lock(&mutex);
        lastVertex = start + 1; 
        dfsRes[dfsIndex] = lastVertex;
        dfsIndex++;
        pthread_mutex_unlock(&mutex);
    }

    for (int i = 0; i < threadCount; i++) {
        if (pthread_join(thread[i], NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
        free(threadArgs[i]);
    }
    visited[start] = 0;
}

// Thread function for DFS
void *dfsThread(void *args) {
    struct DFSThreadArgs *threadArgs = (struct DFSThreadArgs *)args;
    dfs(threadArgs->adjMatrix, threadArgs->vCount, threadArgs->start);
    pthread_exit(NULL);
}

// Function to read graph from a given file
void readGraphFromFile(const char* filename, int graph[MAX_VERTICES][MAX_VERTICES], int* numNodes) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", numNodes);
    for (int i = 0; i < *numNodes; i++) {
        for (int j = 0; j < *numNodes; j++) {
            fscanf(file, "%d", &graph[i][j]);
        }
    }
    fclose(file);
}

// Thread Function which acts as a starting point for DFS and BFS.
void *starterFunction(void *args){
    Message* msg = (Message*)args;
    int numNodes;
    int graph[MAX_VERTICES][MAX_VERTICES];
    char filename[100];
    strcpy(filename,msg->fname);

    readGraphFromFile(filename,graph,&numNodes);

    int shmid = create_shm(msg->seq_num);
    struct sharedData client_data;
    read_from_shared_memory(shmid,&client_data);
    int startNode = client_data.start_vertex;

    // BFS
    if(msg->op_num == 4){
        int traversalResult[MAX_VERTICES];
        for(int i=0;i<MAX_VERTICES;++i){
            traversalResult[i] = 0;
        }
        BFS(graph,startNode,numNodes,traversalResult);
        struct message sent_msg;
        sent_msg.mtype = msg->seq_num;
        
        for(int i=0;i<MAX_VERTICES;++i)
        {
        	sent_msg.res[i]=traversalResult[i];
        }
        
        int msgid = create_message_queue();
        if(msgsnd(msgid,&sent_msg,sizeof(struct message) - sizeof(long),0) == -1){
                perror("Error in msgsnd");
                exit(EXIT_FAILURE);
        }
    }

    // DFS
    if(msg->op_num == 3){
        for (int i = 0; i < numNodes; ++i) {
            visited[i] = 0;
        }

        for(int i=0;i<MAX_VERTICES;++i){
            dfsRes[i] = 0;
        }
        dfsIndex = 0;
        dfs(graph,numNodes,startNode-1);
        int msgid = create_message_queue();
        struct message sent_msg;
        for(int i=0;i<MAX_VERTICES;++i)
        {
        	sent_msg.res[i]=dfsRes[i];
        }
        sent_msg.mtype = msg->seq_num;
        if(msgsnd(msgid,&sent_msg,sizeof(struct message) - sizeof(long),0) == -1){
            perror("Error in msgsnd");
            exit(EXIT_FAILURE);
        }
    }
    pthread_exit(NULL);
}

// Main Function 
int main(){
    int ss_identifier = -1;
    key_t key_for_server = ftok(".",'s');
    if(key_for_server == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    int msgid = create_message_queue();
    printf("Message Queue created\n");
    struct message msg;

    int shmid = shmget(key_for_server,sizeof(struct sharedData),0664);
    
    if(shmid == -1){
        printf("This is Secondary Server 1\n");
        shmid = shmget(key_for_server,sizeof(struct sharedData),0664|IPC_CREAT);
        ss_identifier = 1; // First Secondary Server
        
    }
    else{
        printf("This is Secondary Server 2\n");
        ss_identifier = 2; // Second Secondary Server
        
    }

    while(1){
        if(ss_identifier==1){
            msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 999, 0);
            if(msg.op_num==5)
       	    {
        	// Server exit Message received
        	printf("\nFirst Secondary Server Gracefully Exiting\n");
        	break;
            }
        }
        else if(ss_identifier==2){
            msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 998, 0);
            if(msg.op_num==5)
       	    {
        	// Server exit Message received
        	printf("\nSecond Secondary Server Gracefully Exiting\n");
        	
        	// Delete SHM for server detection
        	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
       	 		perror("Error in shmctl");
        		exit(EXIT_FAILURE);
   		}
   		
        	break;
            }
        }
        pthread_t thread;

        if(pthread_create(&thread,NULL,starterFunction,(void*)&msg) != 0){
            perror("Error in creating thread");
            exit(EXIT_FAILURE);
        }
        pthread_join(thread,NULL);
    }
}
