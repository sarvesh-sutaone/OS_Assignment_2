/*

a. Receives client requests via message queue  
b. Sends odd number read requests to SS1  
c. Sends even number read requests to SS2  
d. Sends write requests to Primary Server  
e. Cleanup 


*/


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
#define MAX_SIZE 200

struct message{
    long mtype;
    int seq_num;
    int op_num;
    int res[100];
    char fname[100];
}typedef Message;

int create_message_queue() {
    key_t key = ftok(".", 'B');
    int msgid = msgget(key, 0664|IPC_CREAT);
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

// Function to forward client requests based on sequence number
void forward_request(int msgid, struct message *msg) {
    // Update message type based on Sequence_Number
    msg->mtype = (msg->seq_num % 2 == 0) ? 998 : 999;

    // Forward the message to the appropriate secondary server
    if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
        perror("Error sending message to secondary server");
        exit(EXIT_FAILURE);
    }
}

// Example usage of the load balancer function
int main() {
    // Create a message queue
    int msgid = create_message_queue();
    printf("Message id created\n");

    // Main loop to handle messages
    while (1) {
        struct message msg;
        // Receive message from the queue
        msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 997, 0);
        printf("Message read with %ld\n",msg.mtype);
        if(msg.op_num == 1 || msg.op_num == 2)
        {
            msg.mtype = 1000;
            if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
            perror("Error sending message to primary server");
            exit(EXIT_FAILURE);
            }
            printf("Message sent to primary server\n");
        }
        
        else{ 
            if(msg.seq_num % 2 == 0){
                msg.mtype = 998;
                if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
                    perror("Error sending message to secondary server");
                    exit(EXIT_FAILURE);
                }
                printf("Message sent to secondary server 2\n");
            }

            else{
                msg.mtype = 999;
                if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
                    perror("Error sending message to secondary server\n");
                    exit(EXIT_FAILURE);
                }
                printf("Message sent to secondary server 2\n");
            }
        }
    }
    delete_message_queue(msgid);

    return 0;
}