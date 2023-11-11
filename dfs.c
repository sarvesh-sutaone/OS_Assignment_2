#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define MAX_VERTICES 30

// Structure to pass arguments to the thread function
struct ThreadArgs {
    int graph[MAX_VERTICES][MAX_VERTICES];
    int vertices;
    int startVertex;
};

// Function to perform DFS traversal
void DFS(int graph[MAX_VERTICES][MAX_VERTICES], int vertices, int vertex, int visited[MAX_VERTICES], int path[MAX_VERTICES], int depth) {
    visited[vertex] = 1;
    path[depth] = vertex + 1;  // Add 1 to start from 1

    // Print or process the current path
    printf("%d ",path[depth]);  // To be sent through message queue by main thread, have to store it.

    // Explore neighbors
    for (int i = 0; i < vertices; ++i) {
        if (graph[vertex][i] && !visited[i]) {
            DFS(graph, vertices, i, visited, path, depth + 1);
        }
    }

    // Backtrack
    visited[vertex] = 0;
}

// Function to read the graph from a file
void readGraphFromFile(const char* filename, int* vertices, int graph[MAX_VERTICES][MAX_VERTICES]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    fscanf(file, "%d", vertices);

    for (int i = 0; i < *vertices; ++i) {
        for (int j = 0; j < *vertices; ++j) {
            fscanf(file, "%d", &graph[i][j]);
        }
    }

    fclose(file);
}

// Thread function to initiate DFS from a given start vertex
void* threadFunction(void* args) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
    int startVertex = threadArgs->startVertex;
    int vertices = threadArgs->vertices;
    int graph[MAX_VERTICES][MAX_VERTICES];

    for (int i = 0; i < vertices; ++i) {
        for (int j = 0; j < vertices; ++j) {
            graph[i][j] = threadArgs->graph[i][j];
        }
    }

    int visited[MAX_VERTICES] = {0};
    int path[MAX_VERTICES];
    DFS(graph, vertices, startVertex, visited, path, 0);

    pthread_exit(NULL);
}

struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int res[100];
    int rows;   
};

int create_shm(){
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

int main() {
    int vertices;
    int graph[MAX_VERTICES][MAX_VERTICES];

    char filename[100];
    strcpy(filename,"G1.txt"); // To be read from message queue.
    readGraphFromFile(filename, &vertices, graph);

    // To be taken as input from SHM.
    int shmid = create_shm();
    struct sharedData client_data;
    read_from_shared_memory(shmid,&client_data);
    int startVertex = client_data.start_vertex;
    startVertex--; // To make it 0 indexed to process it correctly further.
    
    pthread_t threads[MAX_VERTICES];

    for (int i = 0; i < vertices; ++i) {
        if (i == startVertex) {
            struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
            threadArgs->startVertex = i;
            threadArgs->vertices = vertices;

            for (int j = 0; j < vertices; ++j) {
                for (int k = 0; k < vertices; ++k) {
                    threadArgs->graph[j][k] = graph[j][k];
                }
            }

            pthread_create(&threads[i], NULL, threadFunction, (void*)threadArgs);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < vertices; ++i) {
        if (i == startVertex) {
            pthread_join(threads[i], NULL);
        }
    }

    return 0;
}

