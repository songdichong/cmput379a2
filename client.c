#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <curses.h>
#include "A2Lib.h"
#include <signal.h>
int	sock;
int initMessage[2];
int pid;
int gridSize;
pthread_t * threadID;
struct parameters{
	int c;			//key that user pressed
	struct Point * grid;
};


int isConnected(char* MY_IP, int MY_PORT){
	
	struct	sockaddr_in	server;

	struct	hostent		*host;

	host = gethostbyname ("localhost");

	if (host == NULL) {
		perror ("Client: cannot get host description\n");
		exit(1);
	}

	sock = socket (AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror ("Client: cannot open socket\n");
		exit(1);
	}

	bzero (&server, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(MY_IP);
	server.sin_port = htons (MY_PORT);

	if (connect (sock, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server\n");
		exit(1);
	}
	return 1;
}

//signal handler (sigint + sigpipe)
void sigHandle(int signum){
	if (signum == SIGINT){
		close(sock);
		pthread_cancel(*threadID);//close keyboard thread
		endwin();
		printf("Exiting\n");
		exit(1);
	}
	if (signum == SIGPIPE){
		close(sock);
		pthread_cancel(*threadID);
		endwin();
		printf("Server disconnected\n");
		exit(1);
	}
}

void * readKey(void * arg){
	struct parameters * y = arg;
	char packet[1];
	struct sigaction act;
	sigset_t block_mask;
	sigemptyset(&block_mask);
	sigaddset(&block_mask,SIGPIPE);
	memset(&act, '\0', sizeof(act));
	act.sa_handler = &sigHandle;
	act.sa_mask = block_mask;
	act.sa_flags = SA_NODEFER;
	sigaction(SIGPIPE, &act, NULL);
	
	while(1){
		y->c = 0;
		y->c = getch();
		//i=105 up; j = 106 left; k = 107 down; l = 108 right; space = 32 fire;
		if (y->c == 105|y->c == 106|y->c == 107|y->c == 108|y->c == 32|y->c==120){
			//valid input, send packet to server
			packet[0] = y->c;
			send(sock,packet,sizeof(char),0);	
		}
	}
}

void updateBoard(struct parameters input){
	//initialize the screen
	initscr();
	cbreak();
	noecho();
	clear();
	curs_set(0);
	int startX,startY,i;
	startX = startY = 0;
	//print gameboard
	for (i = 0;i<=gridSize;i++){
		mvprintw(0,i,"-");
		mvprintw(gridSize+1,i,"-");
		mvprintw(i,gridSize+1,"|");
		mvprintw(i+1,0,"|");
	}
	mvprintw(0,0,"+");
	mvprintw(0,gridSize+1,"+");
	mvprintw(gridSize+1,0,"+");
	mvprintw(gridSize+1,gridSize+1,"+");
	int deadFlag = 0;
	int discFlag = 0;
	int score = 0;
	//draw all player
	for (i=0;i<gridSize*gridSize;i++){
		if ((input.grid[i].pid) > 0){
			if 		((strcmp((&input.grid[i].dir),"i")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"^");
			}
			else if ((strcmp((&input.grid[i].dir),"j")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"<");
			}
			else if ((strcmp((&input.grid[i].dir),"k")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"v");
			}
			else if ((strcmp((&input.grid[i].dir),"l")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,">");
			}
		}

		//draw bullets
		if (input.grid[i].bullet == 1){
			mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"O");
			if (input.grid[i].bPid == pid){
				attron(A_BOLD);
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"O");
				attroff(A_BOLD);
			}
		}
		
		//redraw the current player
		if (input.grid[i].pid == pid){
			attron(A_BOLD);
			mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"^");
			if 			((strcmp((&input.grid[i].dir),"i")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"^");
			} else if 	((strcmp((&input.grid[i].dir),"j")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"<");
			} else if 	((strcmp((&input.grid[i].dir),"k")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,"v");
			} else if 	((strcmp((&input.grid[i].dir),"l")==0)){
				mvprintw(((i-(i%gridSize))/gridSize)+1,(i%gridSize)+1,">");
			}
			attroff(A_BOLD);
			
			//check if current user is dead or not
			if (input.grid[i].dead == 1){
				deadFlag = 1;
				score = input.grid[i].score;
			}
			
			//check if server disconnect
			else if (input.grid[i].disconnect == 1){
				discFlag = 1;
				score = input.grid[i].score;
			}
		}
	}
	
	if (deadFlag == 1 | discFlag ==1 ){
		pthread_cancel(*threadID);
		close(sock);
		//print score
		if(deadFlag == 1){
			mvprintw(startX+gridSize+2,1,"You are dead! Your final score is: %d", score);
		}else if (discFlag == 1){
			mvprintw(startX+gridSize+2,1,"Connection terminated, Your final score is: %d",score);
		}
		mvprintw(startX+gridSize+3,1,"press anykey to contine");
		refresh();
		//deallocates memeory and end ncurses
		getch();
		endwin();
		exit(1);
	}
	//refresh the screen
	refresh();
}

//input: an array of points recvData[numPoint]
//output: an array of points grid[gridSize*gridSize], and draw gameboard based on this
struct Point * createGrid(struct Point * recvData,int numPoint){
	struct Point * grid;
	grid = malloc(sizeof(struct Point)*gridSize*gridSize);
	initGrid(grid,gridSize);
	int location;
	for (int i = 0; i<numPoint; i++){
		location = recvData[i].location;
		grid[location] = recvData[i];
	}
	return grid;
}

//check connection
void checkRecv (int i){
	if(i==0){
		perror("Connection failed\n");
		endwin();
		close(sock);
		exit(1);
	}else if(i==-1){
		printf("Reading error\n");
		endwin();
		close(sock);
		exit(1);
	}
}

int main(int argc, char *argv[])
{	
	if (argc<3){
		printf("not enough arguments\n");
		exit(1);
	}
	int MY_PORT = atoi(argv[2]);
	char* MY_IP = argv[1];
	int connection = isConnected(MY_IP,MY_PORT);
	
	int i = recv(sock,initMessage,sizeof(int)*2,0);
	checkRecv(i);
	
	pid = initMessage[0];
	gridSize = initMessage[1];
	int receiveSize = gridSize*gridSize;
	struct parameters input;
	input.c = 0;
	int gameOn = 1;
	
	//first data pack: tells how many points are changed
	int numPoint;
	i = recv(sock,&numPoint,sizeof(int),0);
	checkRecv(i);
	//create recvData[numPoint] for the next data pack
	struct Point recvData[numPoint];
	i = recv(sock,&recvData,numPoint*sizeof(struct Point),0);
	checkRecv(i);
	
	//use recvData to create the new grids
	struct Point * received;
	received = createGrid(recvData, numPoint);
	input.grid = received;
	
	updateBoard(input);
	threadID = malloc(sizeof(pthread_t));
	//create a thread for reading user input
	pthread_create(threadID, NULL, readKey, (void *) (&input));
	//create signal handler
	struct sigaction act;
	sigset_t block_mask;
	sigemptyset(&block_mask);
	sigaddset(&block_mask,SIGPIPE);
	memset(&act, '\0', sizeof(act));
	act.sa_handler = &sigHandle;
	act.sa_flags = SA_NODEFER;
	sigaction(SIGINT, &act, NULL);
	
	char packet[1];
	
	while (gameOn){
		i = recv(sock,&numPoint,sizeof(int),0);
		checkRecv(i);
		//create recvData[numPoint] for the next data pack
		struct Point recvData[numPoint];
		i = recv(sock,&recvData,numPoint*sizeof(struct Point),0);
		checkRecv(i);
		//use recvData to create the new grids
		struct Point * received;
		received = createGrid(recvData, numPoint);
		input.grid = received;
		updateBoard(input);
	}
	return 0;
}


