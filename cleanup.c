#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>

#define MAX_MTEXT_SIZE 100

// Structure for MQ
struct message{
    long mtype;
    int seq_num;
    int op_num;
    int res[100];
    char fname[100];
}typedef Message;

int main() {
    
    // The important stuff: Initialization of variables and Message queue declaration
    key_t key;
    int msgqid;
    
   
    long clientID;
    
    key = ftok(".", 'B');
    msgqid = msgget(key, 0666);

    if (msgqid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
	char choice;
    // Listen for server closure requests
    while(1)
    {
    
    	printf("Do you want to shut down the servers. Enter Y for Yes or N for No:\n");
	scanf("%c",&choice);
	
	if(choice=='Y' || choice=='y')
	{
		break;
	}	
    	else
    	{
    		continue;
    	}
    	
    }
	
	// Now Cleanup initiation
	// Send message to loadbalancer
	Message msg;
        msg.op_num = 5;
        msg.mtype = 997;
       
        if(msgsnd(msgqid,&msg,sizeof(struct message) - sizeof(long),0) == -1){
             // Error in sending message
             perror("Error in msgsnd");
             exit(EXIT_FAILURE);
        }
    	
    	
    	printf("\nMessage sent to server... Gracefully Terminating\n");
    

    return 0;
}

