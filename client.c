#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 13514
#define MAX_BUFFER 100
#define MAX_THREAD 3
#define IP "127.0.0.1"


void *msgWait(void *sock);
char user[15];

int main(int argc, char *argv[]){
  system("clear");
  
  puts("Chatroom 1.0");
  printf("Enter your name: ");
  gets(user);
  system("clear");
  
  // Server Setup =============================================
  int sock;
  struct sockaddr_in servAddr;
  char sendMsg[MAX_BUFFER] = {0};

  int temp;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0){
    puts("socket failed.\n");
    exit(1);
  }

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(PORT);
  
  temp = inet_pton(AF_INET, IP, &servAddr.sin_addr); 
  if(temp < 0){
    puts("invalid address.\n");
    exit(1);
  }

  // Thread Setup ==================================================
  pthread_t thread[MAX_THREAD];

  // connect to server ==============================================
  temp = connect(sock, (struct sockaddr*) &servAddr, sizeof(servAddr));
  if(temp < 0){
    puts("connection failed.\n");
    exit(1);
  }
  send(sock, user, 15, 0);

  temp = pthread_create(&thread[0], NULL, msgWait, sock);
  if(temp){
      puts("message wait thread failed.\n");
  }
  
  puts("Chatroom 1.0");
  puts("Type /help for a list of commands.");
  
  while(1){
    gets(sendMsg);
    printf("\033[A"); // Delete current line
    printf("\33[2K"); //
    send(sock, sendMsg, MAX_BUFFER, 0);
    memset(sendMsg, 0x0, MAX_BUFFER);
  }
  pthread_exit(NULL);
}

void *msgWait(void *sock){
  int new_sock = (int)sock;
  char buffer[MAX_BUFFER] = {0};
  int firstTimeConnect = 1;
  while(1){
    int n, i;
    n = read(new_sock, buffer, MAX_BUFFER);
    if(n <= 0){
      puts("Disconected from server.");
      close(new_sock);
      pthread_exit(NULL);
      exit(1);
    }
    int type;
    char *type_c, *text, *sender;
    type_c = strtok(buffer, "&[");
    type = atoi(type_c);
    text = strtok(NULL, "&[");
    sender = strtok(NULL, "&[");
    switch(type){
		// Case
		// 0 = server
		// 1 = chat
		// 2 = private
		//
    	case 0:
    		printf("\033[0;35m"); // Magenta
    		printf("%s",text);
    		printf("\033[0m");
			fflush(stdout);
			if(firstTimeConnect){
				char *tempName = strtok(text, " ");
				strcpy(user, tempName);
				firstTimeConnect = 0;
			}
    		break;
    	case 1:
    		if(strcmp(sender, user) == 0)
    			printf("\033[0;32m"); // Green
    		printf("<%s> %s",sender,text);
    		printf("\033[0m");
    		break;
    	case 2:
    		if(strcmp(sender, user) == 0){
    			printf("\033[0;32m"); // Green
			} else {
    			printf("\033[0;33m"); // Yellow
    		}
			printf("<%s:private> %s",sender,text);
    		printf("\033[0m");
    		break;
    }
    
    printf("\n");
    //printf("%s\n", buffer);
    memset(buffer, 0x0, MAX_BUFFER);
  }
}
