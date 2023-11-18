#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>

struct sharedData{
    int flag;
    int graph[30][30];
    int start_vertex;
    int res[100];
    int rows;   
};

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
    int ss_identifier = -1;
    key_t key = ftok(".",'s');
    if(key == -1){
        perror("Error in ftok");
        exit(EXIT_FAILURE);
    }

    int shmid = shmget(key,sizeof(struct sharedData),0664);
    if(shmid == -1){
        printf("This is Secondary Server 1\n");
        shmid = shmget(key,sizeof(struct sharedData),0664|IPC_CREAT);
        ss_identifier = 1; // First Secondary Server
    }
    else{
        printf("This is Secondary Server 2\n");
        ss_identifier = 2; // Second Secondary Server
    }

    

    // delete_shm(shmid);
}

// The above code solves the problem of two secondary servers.