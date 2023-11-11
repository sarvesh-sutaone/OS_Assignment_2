#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define MAX_NODES 30

// Graph structure
typedef struct {
    int num_nodes;
    int adjacency_matrix[MAX_NODES][MAX_NODES];
    bool visited[MAX_NODES];
} Graph;

// Thread data structure
typedef struct {
    Graph *graph;
    int node;
    int level;
} ThreadData;

// Queue data structure for BFS
typedef struct {
    int data[MAX_NODES];
    int front, rear;
} Queue;

void enqueue(Queue *q, int node) {
    q->data[q->rear++] = node;
}

int dequeue(Queue *q) {
    return q->data[q->front++];
}

bool is_empty(Queue *q) {
    return q->front == q->rear;
}

// BFS traversal function
void bfs_traversal(Graph *graph, int start_node, int level) {
    Queue queue;
    queue.front = queue.rear = 0;

    enqueue(&queue, start_node);
    graph->visited[start_node - 1] = true;

    while (!is_empty(&queue)) {
        int current_node = dequeue(&queue);
        printf("%d ", current_node); // This will be sent to client via message queue. (Will have to store it)

        // Enqueue adjacent nodes
        for (int i = 0; i < graph->num_nodes; ++i) {
            if (graph->adjacency_matrix[current_node - 1][i] == 1 && !graph->visited[i]) {
                enqueue(&queue, i + 1);
                graph->visited[i] = true;
            }
        }
    }
}

// Thread entry point for BFS traversal
void *bfs_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    bfs_traversal(data->graph, data->node, data->level);
    free(arg);
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
    char filename[100];
    strcpy(filename,"G1.txt"); // Will recieve from Msg Queue
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        return 1;
    }

    Graph graph;
    fscanf(file, "%d", &graph.num_nodes);

    for (int i = 0; i < graph.num_nodes; ++i) {
        for (int j = 0; j < graph.num_nodes; ++j) {
            fscanf(file, "%d", &graph.adjacency_matrix[i][j]);
        }
    }

    fclose(file);

    // Start node from Shared Memory
    int shmid = create_shm();
    struct sharedData client_data;
    read_from_shared_memory(shmid,&client_data);
    int start_node = client_data.start_vertex;
    

    if (start_node < 1 || start_node > graph.num_nodes) {
        fprintf(stderr, "Invalid starting node.\n");
        return 1;
    }

    pthread_t threads[MAX_NODES];
    int level = 1;

    ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
    data->graph = &graph;
    data->node = start_node;
    data->level = level;

    pthread_create(&threads[0], NULL, bfs_thread, data);
    pthread_join(threads[0], NULL);

    return 0;
}
