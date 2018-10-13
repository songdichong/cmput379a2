#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "A2Lib.h"

struct LinkedList * players;
int sock;
struct Point * grid;
int size;
int port;
pthread_t * updateID;
int mainExit = 0;
//mutex
pthread_mutex_t mutex2;  //stop player input when evaulating


void evaulate_board(struct LinkedList *players,struct Point * grid) {	//evaulate bullet, empty input, save player info 
	struct Player * cPlayer = players->head;
	while (cPlayer != NULL) {
		if (grid[cPlayer->cur_location].bullet == 1) {	//if player is standing on bullet, then player is dead
			cPlayer->dead = 1;
			grid[cPlayer->cur_location].score = cPlayer->score;	//update again because player might have shoot someone
			grid[cPlayer->cur_location].dead = 1;
			// give player 2 a score
			struct Player * p2 = grid[cPlayer->cur_location].bPlayer;
			p2->score += 1;
			grid[p2->cur_location].score = p2->score;
		}		
		//update player location(old,cur,new(just in case))
		cPlayer->old_location = cPlayer->cur_location;	
		cPlayer->new_location = cPlayer->cur_location;
		cPlayer= cPlayer->next;
	}
}
void player_shot(struct Player * cPlayer, struct Point * grid,int size) {
	//generate bullet
	if (cPlayer->cur_dir == 'i') {
		if (cPlayer->cur_location >= (size * 2)) {
			int b2Location = cPlayer->cur_location - size * 2;
			grid[b2Location].bullet = 1;
			grid[b2Location].bPlayer = cPlayer;
			grid[b2Location].bPid = cPlayer->pid;
		}
		if (cPlayer->cur_location >= size) {
			int b3Location = cPlayer->cur_location - size;
			grid[b3Location].bullet = 1;
			grid[b3Location].bPlayer = cPlayer;
			grid[b3Location].bPid = cPlayer->pid;
		}
	}
	else if (cPlayer->cur_dir == 'k') {
		if (cPlayer->cur_location < (size * size-2*size)) {
			int b2Location = cPlayer->cur_location + size * 2;
			grid[b2Location].bullet = 1;
			grid[b2Location].bPlayer = cPlayer;
			grid[b2Location].bPid = cPlayer->pid;
		}
		if (cPlayer->cur_location < size*size - size) {
			int b3Location = cPlayer->cur_location + size;
			grid[b3Location].bullet = 1;
			grid[b3Location].bPlayer = cPlayer;
			grid[b3Location].bPid = cPlayer->pid;
		}
	}
	else if (cPlayer->cur_dir == 'j') {
		if ((cPlayer->cur_location%size)>=2) {
			int b2Location = cPlayer->cur_location -2;
			grid[b2Location].bullet = 1;
			grid[b2Location].bPlayer = cPlayer;
			grid[b2Location].bPid = cPlayer->pid;
		}
		if ((cPlayer->cur_location%size) >= 1) {
			int b3Location = cPlayer->cur_location -1;
			grid[b3Location].bullet = 1;
			grid[b3Location].bPlayer = cPlayer;
			grid[b3Location].bPid = cPlayer->pid;
		}
	}
	else if (cPlayer->cur_dir == 'l') {
		if ((cPlayer->cur_location%size) <=size-3) {
			int b2Location = cPlayer->cur_location + 2;
			grid[b2Location].bullet = 1;
			grid[b2Location].bPlayer = cPlayer;
			grid[b2Location].bPid = cPlayer->pid;
		}
		if ((cPlayer->cur_location%size) <=size -2) {
			int b3Location = cPlayer->cur_location + 1;
			grid[b3Location].bullet = 1;
			grid[b3Location].bPlayer = cPlayer;
			grid[b3Location].bPid = cPlayer->pid;
		}
	}
}
void move_player(struct Player * cPlayer,struct LinkedList * players,struct Point * grid) {	//move player to new location if possible; Caleld by update board
	if (grid[cPlayer->new_location].player == NULL) {
		//move player info to to there
		grid[cPlayer->new_location].player = cPlayer;
		grid[cPlayer->new_location].pid = cPlayer->pid;
		grid[cPlayer->new_location].score = cPlayer->score;
		grid[cPlayer->new_location].dir = cPlayer->cur_dir;
		grid[cPlayer->new_location].dead = cPlayer->dead;

		//reset old location
		grid[cPlayer->old_location].pid = 0;
		grid[cPlayer->old_location].player = NULL;
		grid[cPlayer->old_location].score = 0;
		grid[cPlayer->old_location].dead = 0;
		grid[cPlayer->old_location].dir = 0;

		//update player location
		cPlayer->cur_location = cPlayer->new_location;
	}
	else {
		struct Player * p2 = grid[cPlayer->new_location].player;
		if (p2->cur_location == p2->old_location) {	//if p2 is there originally, do nothing
			;
		}
		else {//otherwise, return p2 back to its place
			return_player(p2,players,grid);	//this is a little bit complicated, have to use recursive function
		}
	}

}

void return_player(struct Player * cPlayer, struct LinkedList * players, struct Point * grid) {	//Calleld by updateBoard
	if (grid[cPlayer->old_location].player != NULL) {
		return_player(grid[cPlayer->old_location].player, players, grid);
	}
	cPlayer->cur_location = cPlayer->old_location;
	grid[cPlayer->new_location].player = NULL;
	grid[cPlayer->new_location].pid = 0;
	grid[cPlayer->new_location].score = 0;
	grid[cPlayer->new_location].dead = 0;
	grid[cPlayer->new_location].dir = 0;
	
	grid[cPlayer->old_location].player = cPlayer;
	grid[cPlayer->old_location].pid = cPlayer->pid;
	grid[cPlayer->old_location].score = cPlayer->score;
	grid[cPlayer->old_location].dead = cPlayer->dead;
	grid[cPlayer->old_location].dir = cPlayer->cur_dir;

}

void RemoveFromList(struct Player* remove,struct LinkedList* pList,struct Point* grid){	// remove item 'remove' from the list, 'remove' must be in list; Called by updateBoard only
	struct Player* next = remove->next;
	struct Player* prev = remove->prev;
	if (pList->head==remove){
		pList->head=next;
		if(next!=NULL){
			next->prev = NULL;	
		}
	}else{ // not head, need to apply changes to prev and next
		if(prev!=NULL){
			prev->next = next;
		}
		if(next!=NULL){
			next->prev = prev;	
		}
	}
	/*clean up the point first*/
	CleanPoint(grid ,remove->cur_location);
	/*clean up on removed player */
	pthread_cancel(*(remove->cThread));
	char buffer;
	while(recv(remove->cSocket,&buffer,sizeof(char),0)>0); // wait for client close first
	close(remove->cSocket);
	free(remove);
	pList->size -=1;
}

void AddToList(struct Player* newP,struct LinkedList* pList){   // insert an item at the head ; Called by Main function only
	pthread_mutex_lock(&mutex2);
	newP->next = pList->head;
	pList->head= newP;
	if (newP->next != NULL) {
		newP->next->prev = newP;
	}	
	pList->size +=1;
	pthread_mutex_unlock(&mutex2);
}

void initList(struct LinkedList* pList){		//Called once by main function
	pList->head = NULL;
	pList->size = 0;
}

void *t_receive_input(void *arg){			//Generated by main function
	struct Player * p = arg;		
	char c;
	while(mainExit==0){
		int i= recv(p->cSocket,&c,sizeof(char),0);	//receive input from client
		if(i==0){
			perror("Connection ended\n");
			pthread_exit(NULL);
		}else if(i==-1){
			printf("Reading error\n");	
		}
		pthread_mutex_lock(&mutex2);
		p->input = c;
		if(c=='x'){	//if user request imemdiat exit, send them a point
			grid[p->cur_location].disconnect = 1;
			int numPoint = 1;
			send(p->cSocket, &numPoint,sizeof(int),MSG_NOSIGNAL);
			send(p->cSocket, &grid[p->cur_location],sizeof(struct Point), MSG_NOSIGNAL);
		}
		pthread_mutex_unlock(&mutex2);
	}
}

void *t_update_game(void *arg){
	struct Wrapper *w = arg;
	struct Point * grid = w->grid;
	struct LinkedList * players = w->players;
	struct timeval start, stop;
	int timePassed, timer;
	timer = (int)(w->timer * 1000000);
	usleep(timer);
	int updateExit =0; // indicate if this thread have to exit
	
	while (1) {
		gettimeofday(&start, NULL);
		
		//clean up (bullet, disconnect player)
		for (int i = 0; i < (w->size)*(w->size); i++) {		//remove previous bullet from game
			grid[i].bullet = 0;
			if(grid[i].disconnect ==1){				//remove non-exixtent player from the game
				RemoveFromList(grid[i].player, players,grid);
			}
		}

		//calculate new_position,dir for each player(without seeing if it is colliding with another)
		struct Player * curPlayer = players->head;
		pthread_mutex_lock(&mutex2);
		while (curPlayer != NULL) {
			char move = curPlayer->input; //make a copy of client input
			curPlayer->input = 0;	//empty player input
			int moving = 0;
			if (move == 'i' && curPlayer->old_location >= (w->size)) { curPlayer->new_location = curPlayer->old_location - (w->size); moving = 1;  }
			else if (move == 'k' && curPlayer->old_location < (w->size*(w->size - 1))) { curPlayer->new_location = curPlayer->old_location + (w->size); moving = 1;}
			else if (move == 'j' && (curPlayer->old_location) % (w->size) != 0) { curPlayer->new_location = curPlayer->old_location - 1; moving = 1;}
			else if (move == 'l' && curPlayer->old_location % (w->size) != (w->size - 1)) { curPlayer->new_location = curPlayer->old_location + 1; moving = 1;}
			if ( move == 'i' || move == 'j'||move =='k'||move=='l'){ curPlayer->cur_dir = move;grid[curPlayer->cur_location].dir =move ;}	//update direction when client enter a movement(whether he is actually moved or not)
			if (moving) { move_player(curPlayer, players, grid); }
			else if (move == 32) {
				player_shot(curPlayer, grid, w->size);
			}
			curPlayer = curPlayer->next;
		}
		evaulate_board(players, grid);
		int counter =0;
		for(int i =0;i<(w->size)*(w->size);i++){
			if (grid[i].pid!=0 || grid[i].bullet==1){
				counter++;
			}
		}
		struct Point updatePoint[counter];
		counter =0;
		for(int i =0;i<(w->size)*(w->size);i++){
			if (grid[i].pid!=0 || grid[i].bullet==1){
				updatePoint[counter] = grid[i];
				counter++;
			}
		}
		curPlayer = players->head;
		while (curPlayer != NULL) {
			int i = send(curPlayer->cSocket,&counter, sizeof(int), MSG_NOSIGNAL);
			i =send(curPlayer->cSocket, updatePoint, counter* sizeof(struct Point), MSG_NOSIGNAL);
			
			if(i ==-1){
				if(errno==EPIPE){
					grid[curPlayer->cur_location].disconnect = 1;	//set disconnect to 1, clean up on next try
				}
			}
			curPlayer = curPlayer->next;
		}
		for(int i = 0; i<size*size;i++){
			if(grid[i].dead ==1){
				grid[i].disconnect=1;
			}
		}
		pthread_mutex_unlock(&mutex2);
		//calcualte how long to wait
		gettimeofday(&stop, NULL);
		timePassed = (int)(stop.tv_usec - start.tv_usec) + (int)((stop.tv_sec - start.tv_sec) * 1000000);
		if (timePassed < timer) {	//dont wait if the time already passed or the program want to exit immediately
			usleep(timer - timePassed);
		}
	}
	struct Player * cPlayer = players->head;
	while(cPlayer!=NULL){	
		RemoveFromList(cPlayer, players,grid);
		cPlayer = cPlayer->next;
	}
	close(sock);
}

void sigHandle(int signum)	//Occur in main function, mutex is not important
{
	//if receive sigusr1, checkdir immediately
	if (signum == SIGINT||signum ==SIGTERM||signum==SIGCHLD)
	{
		pthread_mutex_lock(&mutex2);
		struct Player * curPlayer = players->head;
		struct Player * prevPlayer;
		while (curPlayer!=NULL){
			int i=1;
			send(curPlayer->cSocket,&i, sizeof(int), MSG_NOSIGNAL);
			grid[curPlayer->cur_location].disconnect=1;
			send(curPlayer->cSocket,&grid[curPlayer->cur_location], size*size* sizeof(struct Point), MSG_NOSIGNAL);
			char b;
			prevPlayer = curPlayer;
			curPlayer = curPlayer->next;
			RemoveFromList(prevPlayer,players,grid);
		}
		//signal the updateBoard thread to exit
		printf("Exiting");
		//wait for updateBoard to close connection on next refresh
		pthread_cancel(*updateID);
		pthread_mutex_unlock(&mutex2);
		//destroy mutuex
		pthread_mutex_destroy(&mutex2);
		close(sock);
		exit(1);
	}
}
int main(int argc, char * argv[]){
	//passing argument
	if (argc<5){
		printf("not enough arguments\n");
		exit(1);
	}
	size = atoi(argv[1]);
	double timer = atof(argv[2]);
	port = atoi(argv[3]);
	int seed = atoi(argv[4]);
	srand(seed);
	
	pid_t pid;
	pid = fork();
	if(pid<0){exit(EXIT_FAILURE);}
	if(pid>0){exit(EXIT_SUCCESS);}
	if(setsid()<0){exit(EXIT_FAILURE);}
			  
	//signal handler to exit server
	struct sigaction act;
	sigset_t block_mask;
	sigemptyset(&block_mask);
	sigaddset(&block_mask,SIGPIPE);
	memset(&act, "\0", sizeof(act));
	act.sa_handler = &sigHandle;
	act.sa_mask = block_mask;
	act.sa_flags = SA_NODEFER;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	pid = fork();
	if(pid<0){exit(EXIT_FAILURE);}
	if(pid>0){exit(EXIT_SUCCESS);}
	//initalize mutex
	pthread_mutex_init(&mutex2,NULL);
	
	int initMessage[2];
	int pidCounter = 1;
	int snew,fromlength;
	struct sockaddr_in master,from	;
	players = malloc(sizeof(struct LinkedList));
	if(players==NULL){
		perror("malloc arr"); exit(EXIT_FAILURE);
	}
	initList(players);
	grid = malloc((size*size * sizeof(struct Point)));
	initGrid(grid, size);
	sock = socket(AF_INET,SOCK_STREAM,0); 
	if(sock<0){
		perror("Server: cannot open master socket");
		exit(1);
	}
	master.sin_family=AF_INET;
	master.sin_addr.s_addr=inet_addr("127.0.0.1");
	master.sin_port = htons(port);
	if(bind(sock,(struct sockaddr*) &master, sizeof(master))){
		perror("System:cant bind master socket");
		exit(1);
	}
	//create thread to update the game every t sec
	updateID = malloc(sizeof(pthread_t));
	struct Wrapper * w = malloc(sizeof(struct Wrapper));
	w->players = players;
	w->grid = grid;
	w->size = size;
	w->timer = timer;
	pthread_create(updateID, NULL, t_update_game, (void *)w);
	
	
	//main thread is used to create player only
	while(1){
		listen(sock,5);
		//create new socket
		fromlength = sizeof(from);
		snew = accept(sock, (struct sockaddr*) & from, &fromlength);
		if (snew < 0) {
			perror("Server: accept failed");
		}
		struct Player* newPlayer = malloc(sizeof(struct Player));
		initPlayer(newPlayer, grid, size, pidCounter,snew,&mutex2);
		initMessage[0] = pidCounter,
		initMessage[1] =size;
		int i = send(snew, initMessage, sizeof(initMessage), MSG_NOSIGNAL);
		if(i ==-1 && errno == EPIPE){
			free(newPlayer);
		}else{
			//create player
			AddToList(newPlayer,players);
			pthread_t * ctID = malloc(sizeof(pthread_t));
			pthread_create(ctID, NULL, t_receive_input, (void *)newPlayer);
			newPlayer->cThread = ctID;
			if(pidCounter<32767){
				pidCounter++;
			}else{	//if 32767 player already played the game ....
				struct Player * curPlayer = players->head;
				int try=1;	//try to give player this pid
				int goodPid = 0;	// indicate if ths pid is good
				while(goodPid!=1){
					int founded = 0;
					while(curPlayer!=NULL && founded!=1){
						if(curPlayer->pid==try){
							founded =1;	
						}
						curPlayer=curPlayer->next;
					}
					 if(founded==1){
						try++;
					}else{goodPid = 1;}
				}
			}
		}
	}
}
//Remaining code to add
//1. Exit server step Done
//2. exit client step, server have to do some handling Done
//3. Player dead step	Done
//4. mutex 1 (Dont change player list or grid from other location, while the board is updating) Done
//5. mutex 2 (stop player_input when board is evaulating), alsoc clear player input after evaluating; Done;
//6. dead should be save in player ;
//7. disconnected player dont move
//8. mutex on disconnect var Done
//10. instead of sending whole grid, only send the point that are occupied Done
//1. mutex between initPlayer(sending player board), and updateBoard
//2. mutex: when playerInit at a location, the location could be occupied already