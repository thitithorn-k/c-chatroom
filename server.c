#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define PORT 13514
#define MAX_BUFFER 100
#define MAX_THREAD 3

// Function prototype =================================================================
void *AcceptConnection(void *td);
void *StartServer();
int sendToUser(char *text, char*sender, int type, int targetID);
void printTime();
int findIdWithUsername(char *name);
int getFreeThread(int *freeThread);
void *AdminConsole();
void announce(char *text);
void kickUser(char *name);
void pipeto(char** cmd);
// Function prototype End =============================================================

pthread_t thread_s[2];
pthread_t thread[MAX_THREAD];
int freeThread[MAX_THREAD] = {0};

int connectingUser[MAX_THREAD] = {0};
char connectingUsername[MAX_THREAD][15];

char *help = "Command			Description\n/user			display online user\n/p <username> <message>	send private message\n";
char logBuff[MAX_BUFFER];
FILE *logFile;

struct thread_data{
  int t_id;
  int socket;
};

// Set Time
time_t timer;
char t_buff[28];
struct tm *tm_info;

// Main ===============================================================================
int main(int argv, char *argc[]){
	
	// Pipe for log file==========================================
	timer = time(NULL);
	tm_info = localtime(&timer);
	strftime(t_buff, 28, "log-%Y-%m-%d-%H-%M-%S.txt", tm_info);
	char *cmd[] = {"tee", t_buff, NULL};
	pipeto(cmd);
	
	int temp, t_id;
	  
	  // Create Thread *startServer ==============================
	temp = pthread_create(&thread_s[0], NULL, StartServer, NULL);
	if(temp){
		puts("StartServer thread failed.\n");
	}
	temp = pthread_create(&thread_s[1], NULL, AdminConsole, NULL);
    if(temp){
		puts("AdminConsole thread failed.\n");
    }
  pthread_exit(NULL);
}

// Create socket and Listen for user's accept =========================================
void *StartServer(){
  system("clear");

  // Server Setup =============================================
  int server_fd, new_socket;
  struct sockaddr_in servAddr;
  int servAddrLen = sizeof(servAddr);
  int temp;
  char buffer[MAX_BUFFER] = {0};

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == 0){
    puts("socket failed.\n");
    exit(1);
  }

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(PORT);
   
  temp = bind(server_fd, (struct socketaddr*) &servAddr, sizeof(servAddr));
  if(temp < 0){
    puts("bind failed.\n");
    exit(1);
  }

  temp = listen(server_fd, 5);
  if(temp < 0){
    puts("listen failed.\n");
    exit(1);
  }
  
  printTime();
  puts("Server is running...");
  fflush(stdout);

  // Accept connection =============================================
  while(1){
    new_socket = accept(server_fd, (struct sockaddr*) &servAddr, (socklen_t*) &servAddrLen);
    if(new_socket < 0){
      puts("accept failed.\n");
      exit(1);
    }
    int t_id;
    t_id = getFreeThread(freeThread);
    if(t_id < 0){
      read(new_socket, buffer, 15);
      send(new_socket, "0&[Server Full.&[Server" , MAX_BUFFER, 0);
	  printTime();
      printf("%s have been kicked by server. (Thread Full)\n", buffer);
	  fflush(stdout);
      close(new_socket);
      memset(buffer, 0x0, MAX_BUFFER);
      continue;
    } else {
      struct thread_data td;
      td.t_id = t_id;
      td.socket = new_socket;
      temp = pthread_create(&thread[t_id], NULL, AcceptConnection, (void*)&td);
      if(temp){
        puts("create thread failed.\n");
      } else {
        freeThread[t_id] = 1;
      }
    }
  }
}

// Thread for Each User who connected to server =======================================
void *AcceptConnection(void *td){
  struct thread_data *thread_d = (struct thread_data*) td;
  int new_socket = thread_d->socket;
  char sendMsg[MAX_BUFFER] = {0};
  char buffer[MAX_BUFFER] = {0};
  int id, n, thread_id, i;
  char username[15];
  thread_id = thread_d->t_id;
  read(new_socket, buffer, 15);
  strcpy(username, buffer);
  for(i = 0; i < MAX_THREAD; i++){
	//Check if username is already in use
  	if((strcmp(connectingUsername[i], username)) == 0){
  		printTime();
		printf("username is already in use. (%s)\n", buffer);
		snprintf(username, MAX_BUFFER, "%s(%d)", buffer, thread_id);
  	}
  }
  //Put this socket to "connectingUser" array
  for(id = 0; id < MAX_THREAD; id++){
    if(connectingUser[id] == 0){
        connectingUser[id] = new_socket;
        strcpy(connectingUsername[id], username);
        strcpy(buffer, username);
        strcat(buffer, " join the chat room.");
        printTime();
        puts(buffer);
        sendToUser(buffer, id, 0, -1);
        break;
    }
  }
  //Wait for user message
  while(1){
    n = read(new_socket, buffer, MAX_BUFFER);
    if(n <= 0){
      strcpy(buffer, connectingUsername[id]);
      strcat(buffer, " disconnected.");
      printTime();
      puts(buffer);
      sendToUser(buffer, id, 0, -1);
      close(connectingUser[id]);
      connectingUser[id] = 0;
      freeThread[thread_id] = 0;
      break;
    } else {
		// if recieved message is command message
    	if(buffer[0] == '/'){
    		char *command = strtok(buffer, " ");
    		if(strcmp(command, "/help") == 0){
    			memset(buffer, 0x0, MAX_BUFFER);
    			sendToUser(help, id, 0, id);
    		} else if(strcmp(command, "/p") == 0){
    			char *user = strtok(NULL, " ");
    			char *message = strtok(NULL, "\0");
				if(user == NULL || message == NULL){
					puts("error");
					fflush(stdout);
					strcpy(buffer, "Command error. Type /help for a list of commands.");
    				sendToUser(buffer, id, 0, id);
					continue;
				}
    			int targetID = findIdWithUsername(user);
    			if(targetID == -1){
    				strcpy(buffer, "user not found.");
    				sendToUser(buffer, id, 0, id);
    			} else {
    				sendToUser(message, id, 2, id);
    				sendToUser(message, id, 2, targetID);
    			}
    		} else if(strcmp(command, "/user") == 0){
    			memset(buffer, 0x0, MAX_BUFFER);
    			strcat(buffer, "Online user\n ");
    			for(i = 0; i < MAX_THREAD; i++){
    				if(connectingUser[i] != 0){
    					strcat(buffer, connectingUsername[i]);
    					strcat(buffer, "\n ");
    				}
    			}
    			sendToUser(buffer, id, 0, id);
    		}
		// if recieved message is just a text
    	} else {
		    sendToUser(buffer, id, 1, -1);
		    memset(buffer, 0x0, MAX_BUFFER);
    	}
    }
  }
  pthread_exit(NULL);
}
/*
int KickUser(int id){
	
	close(connectingUser[id]);
	connectingUser[id] = 0;
	freeThread[thread_id] = 0;
	
}*/

// Find free thread ==================================================================
int getFreeThread(int *freeThread){
  int i;
  for(i = 0; i < MAX_THREAD; i++){
    if(freeThread[i] == 0){
      return i;
    }
  }
  return -1;
}

// Send message to all connecting user
// type
// 0 = server
// 1 = chat
// 2 = private
//
int sendToUser(char* text, char* sender, int type, int targetID){
  int i;
  int senderId;
  senderId = (int) sender;
  char sendText[MAX_BUFFER] = {0};
  sprintf(sendText, "%d", type);
  strcat(sendText, "&[");
  strcat(sendText, text);
  switch(type){
  	case 0:
  		printTime();
  		printf("Server ");
  		strcat(sendText, "&[server");
  		break;
  	case 1:
  	case 2:
  		printTime();
  		printf("<%s> ", connectingUsername[senderId]);
  		strcat(sendText, "&[");
    	strcat(sendText, connectingUsername[senderId]);
    	break;
  }
  //puts(connectingUsername[senderId]);
  //puts(sendText);
  if(targetID == -1){
  	printf("to all: ");
	for(i = 0; i < MAX_THREAD; i++){
		if(connectingUser[i] != 0){
			send(connectingUser[i], sendText , MAX_BUFFER, 0);
		}
	}
  } else {
  	printf("to <%s>: ", connectingUsername[targetID]);
  	send(connectingUser[targetID], sendText , MAX_BUFFER, 0);
  }
  printf("%s\n", text);
  fflush(stdout);
  return 0;
}

int findIdWithUsername(char *name){
	int i;
	for(i = 0; i < MAX_THREAD; i++){
		if(strcmp(connectingUsername[i], name) == 0) return i;
	}
	return -1;
}

// PrintTime for log
void printTime(){
	timer = time(NULL);
	tm_info = localtime(&timer);
	strftime(t_buff, 28, "%Y-%m-%d %H:%M:%S", tm_info);
	printf("[%s] ",t_buff);
	return;
}

// Admin Console =====================================================================
void *AdminConsole(){
	char command[MAX_BUFFER];
	int i;
	while(1){
		gets(command);
		char *temp = strtok(command, " ");
		if(!strcmp(temp, "/help")){
			puts("Command\t\t\tDescription\n/kick [username]\tKick the user out of the chatroom\n/user\t\t\tDisplay all online user\n/a [message]\t\tAnnounce to every user.");
		} else if(!strcmp(temp, "/kick")){
			char *temp = strtok(NULL, "\0");
			kickUser(temp);
		} else if(!strcmp(temp, "/user")){
			puts("Currently online user:");
			for(i = 0; i < MAX_THREAD; i++){
    			if(connectingUser[i] != 0){
    				puts(connectingUsername[i]);
    			}
    		}
		} else if(!strcmp(temp, "/a")){
			char *temp = strtok(NULL, "\n");
			announce(temp);
		} else {
			puts("Command not found. Type /help for a list of commands.");
		}
		fflush(stdout);
	}
}

// Announce ==========================================================================
void announce(char *message){
	sendToUser(message, "Admin", 0, -1);
	return;
}

// Kick user =========================================================================
void kickUser(char *name){
	int id = findIdWithUsername(name);
	if(id == -1){
		puts("user not found.");
		return;
	}
	close(connectingUser[id]);
	
	connectingUser[id] = 0;
	memset(connectingUsername[id], 0x0, 15);
	
	pthread_cancel(thread[id]);
	freeThread[id] = 0;
	printTime();
	printf("%s have been kicked by Admin.\n", name);
	return;
}

void pipeto(char** cmd) {
  int fds[2];
  pipe(fds);
  if(fork()) {
    // Set up stdout as the write end of the pipe
    dup2(fds[1], 1);
    close(fds[0]);
    close(fds[1]);
  } else {
    // Double fork to not be a direct child
    if(fork()) exit(0);
    // Set up stdin as the read end of the pipe, and run tee
    dup2(fds[0], 0);
    close(fds[0]);
    close(fds[1]);
    execvp(cmd[0], cmd);
  }
}
