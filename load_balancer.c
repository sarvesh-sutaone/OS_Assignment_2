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

int main() {
    // Create a message queue
    int msgid = create_message_queue();
    printf("Connection established succesfully!\n");

    // Main loop to handle messages
    while (1) {
        struct message msg;
        // Receive message from the queue
        msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 997, 0);
        printf("Message read with %ld\n",msg.mtype);
        
        if(msg.op_num==5)
        {
        	printf("Cleanup message received\n");
        	
        	//We will come out of the forever while loop
        	// See how many messages are left to be handled
        	// Finish those
        	// Once that is done we sent termination signals to the servers
        	printf("\n Initiating Termination Process\n");
        	break;
        }
        else if(msg.op_num == 1 || msg.op_num == 2)
        {
            msg.mtype = 1000;
            if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
            perror("Error sending message to primary server");
            exit(EXIT_FAILURE);
            }
            printf("Message forwarded to primary server\n");
        }
        
        else{ 
            if(msg.seq_num % 2 == 0){
                msg.mtype = 998;
                if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
                    perror("Error sending message to secondary server");
                    exit(EXIT_FAILURE);
                }
                printf("Message forwarded to secondary server 2\n");
            }

            else{
                msg.mtype = 999;
                if (msgsnd(msgid, &msg, sizeof(struct message) - sizeof(long), 0) == -1) {
                    perror("Error sending message to secondary server\n");
                    exit(EXIT_FAILURE);
                }
                printf("Message forwarded to secondary server 1\n");
            }
        }
    }
    
    
     /*---------------------------------------------AFTER CLEANUP MESSAGE RECEIVED--------------------------------------------------------------------------------------------*/
    
    // Handle Remaining requests
    struct msqid_ds buf;
    
    // Get information about the message queue
    if (msgctl(msgid, IPC_STAT, &buf) == -1) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
    
    // Print the number of messages remaining in the queue
    printf("Number of messages in the queue: %ld\n", buf.msg_qnum);
    
    int cnt=buf.msg_qnum;
    while(cnt!=0)
    {
    	
    	struct message msg;
        // Receive message from the queue
        msgrcv(msgid, &msg, sizeof(struct message) - sizeof(long), 997, 0);
        printf("Message read with %ld\n\n",msg.mtype);
        
        
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
                printf("Message sent to secondary server 1\n");
            }
        }
        
    	cnt--;
    }
    
    // Finished dealing with the remaining messages in the buffer--------------------------------------------------------------
    Message kill_msg;
    kill_msg.op_num=5;
    
    // Send termination message to primary server*****************************
    
    kill_msg.mtype=1000;
    
    if (msgsnd(msgid, &kill_msg, sizeof(struct message) - sizeof(long), 0) == -1) {
            perror("Error sending message to primary server");
            exit(EXIT_FAILURE);
    }
    
    printf("Message to stop Primary server sent\n");
    
    
    // Send termination message to secondary server****************************
    
    kill_msg.mtype = 999;
    
    if (msgsnd(msgid, &kill_msg, sizeof(struct message) - sizeof(long), 0) == -1) {
             perror("Error sending message to secondary server\n");
             exit(EXIT_FAILURE);
    }
    
    printf("Message to stop Secondary server 1 sent\n");
    
    kill_msg.mtype = 998;
    
    if (msgsnd(msgid, &kill_msg, sizeof(struct message) - sizeof(long), 0) == -1) {
             perror("Error sending message to secondary server\n");
             exit(EXIT_FAILURE);
    }
    
    printf("Message to stop Secondary server 2 sent\n");
    
    
    // Cleanup and exit
    msgctl(msgid, IPC_RMID, NULL); // Remove the message queue
    printf("Load Balancer exiting...\n");
    exit(EXIT_SUCCESS);


    return 0;
}
