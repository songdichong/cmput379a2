struct Point{
	int location; 
	int bullet; //0 indicate False, 1 indicate True
	int  bPid;
	struct Player * bPlayer; // this is the player who sended the bullet to this address
	struct Player * player; // for calculating move only
	
	int pid; 	// negative indicate no playere is there, positive indicate the player is there
	char dir;	//indicate player(if exist) direction; i indicate up, k indicate down, j indicate left, l indicate right
	int dead;	//1 indicate player dead, 0 indicate player alive;
	int disconnect;
	int score; //player score (have to been updated frequently)
};

struct Player{
	int old_location;  //y*size+x 
	int old_dir;
	int new_location;	//y*size+x 
	int cur_location;   //y*size+x
	
	int cur_dir;		
	int pid;	
	int dead;
	int disconnect;
	int score;
	
	int cSocket;
	char input;
	pthread_t * cThread;
	struct Player* next;
	struct Player* prev; //this is needed so Linkedlist can be updated
};

void initPlayer(struct Player * newPlayer, struct Point * grid,int size,int pid,int snew,pthread_mutex_t * mutex1) {	//called by main function only
	newPlayer->cSocket = snew;
	newPlayer->cur_dir = 'i';
	newPlayer->old_dir = 'i';
	newPlayer->pid = pid;
	newPlayer->dead = 0;
	newPlayer->input = 0;
	newPlayer->score = 0;
	newPlayer->next = NULL;
	newPlayer->prev = NULL;
	int location = rand();	//assuem the grid doesnt have more slot than MAX_RAND
	location = location % (size*size);
	while (grid[location].pid != 0 || grid[location].bullet != 0) {
		location = rand();
		location = location % (size*size);
	}
	newPlayer->cur_location = location ;
	newPlayer->old_location = location ;
	//update the grid (lock mutex)
	pthread_mutex_lock(mutex1);	//mutex is need to ensure no changes can occur during the update
	grid[location].player = newPlayer;
	grid[location].pid = pid;
	grid[location].score = 0;
	grid[location].dead = 0;
	grid[location].dir = 'i';
	pthread_mutex_unlock(mutex1);
}

void initGrid(struct Point *grid,int size) {
	for (int i = 0; i < (size*size); i++) {
		grid[i].location = i;
		grid[i].bullet = 0;
		grid[i].bPlayer = NULL;
		grid[i].pid = 0;
		grid[i].player = NULL;
		grid[i].bPid= 0;
		grid[i].dir = 0;
		grid[i].dead = 0;
		grid[i].disconnect=0;
		grid[i].score = 0;
	}
}
void CleanPoint(struct Point* grid ,int location){
	grid[location].pid = 0;
	grid[location].bullet = 0;
	grid[location].player =NULL;
	grid[location].dead = 0;
	grid[location].bPlayer=NULL; 
	grid[location].bPid= 0;
	grid[location].dir = 0;
	grid[location].disconnect = 0;
	grid[location].score = 0;
}

struct LinkedList{
	struct Player* head;
	int size;
};

struct Wrapper{
	struct LinkedList * players;	
	struct Point * grid;
	int size;
	double timer;
};
