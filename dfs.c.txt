#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX 5

int visited[MAX];
int lastVertex;  // Variable to store the last vertex of the current path

// Structure to pass arguments to the thread function
struct ThreadArgs {
    int adjMatrix[MAX][MAX];
    int vCount;
    int start;
};

// Mutex for thread-safe operations
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// dfs function is defined with three arguments
void dfs(int adjMatrix[][MAX], int vCount, int start);

// Thread function for DFS traversal
void *dfsThread(void *args);

// dfs function is defined with three arguments
void dfs(int adjMatrix[][MAX], int vCount, int start) {
    pthread_t thread[MAX];  // Maximum possible threads
    struct ThreadArgs *threadArgs[MAX];

    pthread_mutex_lock(&mutex);
    visited[start] = 1;
    pthread_mutex_unlock(&mutex);

    int isLeaf = 1;  // Flag to check if the current node is a leaf

    int threadCount = 0;

    for (int i = 0; i < vCount; i++) {
        if (adjMatrix[start][i] && !visited[i]) {
            isLeaf = 0;  // If there's an unvisited neighbor, the current node is not a leaf

            // Prepare arguments for the new thread
            threadArgs[threadCount] = malloc(sizeof(struct ThreadArgs));
            if (threadArgs[threadCount] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            threadArgs[threadCount]->vCount = vCount;
            threadArgs[threadCount]->start = i;
            for (int j = 0; j < MAX; j++) {
                for (int k = 0; k < MAX; k++) {
                    threadArgs[threadCount]->adjMatrix[j][k] = adjMatrix[j][k];
                }
            }

            // Create a new thread for DFS traversal
            if (pthread_create(&thread[threadCount], NULL, dfsThread, (void *)threadArgs[threadCount]) != 0) {
                perror("Thread creation failed");
                exit(EXIT_FAILURE);
            }

            threadCount++;
        }
    }

    // If the current node is a leaf, update the lastVertex variable
    if (isLeaf) {
        pthread_mutex_lock(&mutex);
        lastVertex = start + 1;  // Store 1-based vertex number
        printf("Last Vertex: %d\n", lastVertex);
        pthread_mutex_unlock(&mutex);
    }

    // Wait for all child threads to finish
    for (int i = 0; i < threadCount; i++) {
        if (pthread_join(thread[i], NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
        free(threadArgs[i]);
    }

    // Backtrack
    visited[start] = 0;
}

// Thread function for DFS traversal
void *dfsThread(void *args) {
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    dfs(threadArgs->adjMatrix, threadArgs->vCount, threadArgs->start);
    pthread_exit(NULL);
}

// main function is used to implement the above functions
int main() {
    int adjMatrix[MAX][MAX] = {
        {0, 1, 0, 0, 0},
        {1, 0, 1, 0, 1},
        {0, 1, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 0, 0}
    };

    int numNodes = 5;
    int start_vertex = 1;

    for (int i = 0; i < MAX; ++i) {
        visited[i] = 0;
    }

    printf("Last Vertices of All Paths:\n");
    dfs(adjMatrix, numNodes, start_vertex - 1);  // Adjust the starting vertex to be 0-based

    return 0;
}
