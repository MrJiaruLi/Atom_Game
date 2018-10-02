#include <stdio.h>
#include "atoms.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

typedef union move_data move_data;
typedef struct coordinate coordinate;

struct coordinate {
	uint8_t x;
	uint8_t y;
	uint16_t padding;
};
union move_data {
	coordinate move;
	uint32_t raw_move_data;
};


uint8_t player; // number of the player
uint8_t width;
uint8_t height;
int track = 0;  // used to detect the loser
int turn = 0;   // used to count how many turns players played
int winner = 0; // used to dectect the appearance of the winner
int startGame = 0; // used to detect the game is on
int placeUsed = 0; // used to ddetect the place command is used
int gameLoaded = 0; // used to detect the game is loaded.
int fileLength = 0; // use fileLength to get the number of movements stored in the file.
int turnload = 0;  // used to detect playing from 0 turn after loading the game
int undotrack = 0;// used to detect when undo we just delete the last list, when we processing the undo command, we donot want to link between move.
int cannotUndo = 0; //used to detect the situation with no move.
int report = 0; //used to check not to print the same statement.
char *colour;     // all the colour we have
char* file_name;
grid_t **gridMap;
game_t *gameProcess; // the header of the list
player_t *currentPlayers; //used to store all the players
move_data* dataload;

int checkDigit(int length, char *input, int valid) {
	char* temp = (char*)input;
	for(int i = 0; i < length; i++) {
		if(!isdigit(*temp) && *temp != '\n') {
			valid = 1;
			return valid;
		}
	temp = temp + 1;
	}
	return valid;
}
void help() {
	printf("\n");
	printf("HELP displays this help message\n");
	printf("QUIT quits the current game\n");
	printf("\n");
	printf("DISPLAY draws the game board in terminal\n");
	printf("START <number of players> <width> <height> starts the game\n");
	printf("PLACE <x> <y> places an atom in a grid space\n");
	printf("UNDO undoes the last move made\n");
	printf("STAT displays game statistics\n");
	printf("\n");
	printf("SAVE <filename> saves the state of the game\n");
	printf("LOAD <filename> loads a save file\n");
	printf("PLAYFROM <turn> plays from n steps into the game\n");
	printf("\n");
}
void quit() {
	printf("Bye!\n");
	return;
}
void display(uint8_t width, uint8_t height, grid_t **gridMap) {
	//print First Line
	printf("\n");
	for(int i = 0; i < 3*width + 1; i++) {
		if(i == 0) {
			printf("+");
		}else if(i == 3*width) {
			printf("+\n");
		}else {
			printf("-");
		}
	}
	for(int m = 0; m < height; m++) {
		for(int n = 0; n < width; n++) {
			printf("|");
			if(gridMap[m][n].owner == NULL) {
				printf("  ");
			}else {
				printf("%c%d", gridMap[m][n].owner->colour, gridMap[m][n].atom_count);
			}
			if(n == width-1) {
				printf("|\n");
			}
		}
	}
	//print the last line
	for(int i = 0; i < 3*width + 1; i++) {
		if(i == 0) {
			printf("+");
		}else if(i == 3*width) {
			printf("+\n");
		}else {
			printf("-");
		}
	}
	printf("\n");
}
void append(move_t* m1, move_t* m2) {
	while(m1->next != NULL){
		m1 = m1->next;
	}
	m1->next = m2;
	m2->parent = m1;
	m2->next = NULL;

}
void expand (int x, int y, grid_t** gridMap, player_t* currentPlayers,int *track, uint8_t width, uint8_t height,uint8_t player, int *winner) {
	//limit is 2, the Coordinate is in the corner

	if(gridMap[y][x].owner == NULL) {
		gridMap[y][x].owner = &currentPlayers[*track];
		gridMap[y][x].owner->colour = currentPlayers[*track].colour;
		currentPlayers[*track].grids_owned ++ ;
		gridMap[y][x].atom_count = 1;
	}else {
		if(gridMap[y][x].owner->colour != currentPlayers[*track].colour) {
			gridMap[y][x].owner->grids_owned--;
			gridMap[y][x].owner = &currentPlayers[*track];
			gridMap[y][x].owner->colour = currentPlayers[*track].colour;
			currentPlayers[*track].grids_owned ++ ;
			gridMap[y][x].atom_count++;
		}else {
			gridMap[y][x].atom_count++;
		}
	}

	if(x==0 && y==0) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 2) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}

	}else if(x==0 && y==height-1) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 2) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}

	}else if(x==width-1 && y == 0) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 2) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}

	}else if(x==width-1 && y==height-1) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 2) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}
		// Limit is 3 from this line.
	}else if(y== 0 && x!=0 && x!= width-1) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 3) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}

	}else if(x==0 && y!=0 && y!= height-1) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 3) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}
	}else if(x==width-1 && y!=0 && y!= height-1) {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 3) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}
	}else if(y==height-1 && x!=0 && x!= width-1){
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 3) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;
		}
	}else {
		if(*winner == 1) {
			return;
		}
		if(gridMap[y][x].atom_count == 4) {
			gridMap[y][x].owner = NULL;
			gridMap[y][x].atom_count = 0;

			expand(x,y-1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x+1,y,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x,y+1,gridMap,currentPlayers,track,width,height,player,winner);
			expand(x-1,y,gridMap,currentPlayers,track,width,height,player,winner);

			currentPlayers[*track].grids_owned--;

		}
	}

	int counter = 0;
	for(int i = 0; i < player; i++) {
		if(currentPlayers[i].grids_owned == 0) {
			counter++;
		}
	}

	if(counter == (player - 1)) {
		*winner = 1;
	}
}

void place(int x, int y, grid_t** gridMap, player_t *currentPlayers,int* track,uint8_t player,uint8_t width,uint8_t height,int *turn,int*winner,game_t* gameProcess) {
	cannotUndo = 0;
	if(gridMap[y][x].owner == NULL) {
		if(*turn == 0 && undotrack == 0) {
			move_t* new_move = malloc(sizeof(move_t));
			gameProcess->head = new_move;
			new_move->x = x;
			new_move->y = y;
			gameProcess->head->parent = NULL;
			gameProcess->head->next = NULL;
		}else if (undotrack == 0){
			move_t* new_move = malloc(sizeof(move_t));
			new_move->x = x;
			new_move->y = y;
			append(gameProcess->head, new_move);
		}

		gridMap[y][x].owner = &currentPlayers[*track];
		gridMap[y][x].owner->colour = currentPlayers[*track].colour;
		gridMap[y][x].atom_count = 1;
		currentPlayers[*track].grids_owned++;
		*turn = *turn + 1;
		//undotrack = 0;
	}else {
		if(gridMap[y][x].owner->colour != currentPlayers[*track].colour) {
			printf("Cannot Place Atom Here\n");
			printf("\n");
			return;
		}else {
			if(*turn == 0 && undotrack == 0) {
				move_t* new_move = malloc(sizeof(move_t));
				gameProcess->head = new_move;
				new_move->x = x;
				new_move->y = y;
				gameProcess->head->next = NULL;
				gameProcess->head->parent = NULL;
			}else if(undotrack == 0){
				move_t* new_move = malloc(sizeof(move_t));
				new_move->x = x;
				new_move->y = y;
				append(gameProcess->head, new_move);
			}
			*turn = *turn + 1;
			expand(x,y,gridMap,currentPlayers,track,width,height,player,winner);
			//undotrack = 0;
		}
	}
	if(*winner == 1) {
		for(int i = 0; i < player; i++) {
			if(currentPlayers[i].grids_owned != 0) {
				if(currentPlayers[i].colour == 'R'){
					printf("Red Wins!\n");
					return;
				}
				if(currentPlayers[i].colour == 'G'){
					printf("Green Wins!\n");
					return;
				}
				if(currentPlayers[i].colour == 'P'){
					printf("Purple Wins!\n");
					return;
				}
				if(currentPlayers[i].colour == 'B'){
					printf("Blue Wins!\n");
					return;
				}
				if(currentPlayers[i].colour == 'Y'){
					printf("Yellow Wins!\n");
					return;
				}
				if(currentPlayers[i].colour == 'W'){
					printf("White Wins!\n");
					return;
				}
			}
		}
	}

	do{
		if(*track == player - 1) {
			*track = 0;
		}else {
			*track = *track + 1;
		}
	}while(currentPlayers[*track].grids_owned == 0 && *turn > player);

	if(startGame == 1 && report == 0) {

		if(currentPlayers[*track].colour == 'R') {
			printf("Red\'s Turn\n");
			printf("\n");
		}
		if(currentPlayers[*track].colour == 'G') {
			printf("Green\'s Turn\n");
			printf("\n");
		}
		if(currentPlayers[*track].colour == 'P') {
			printf("Purple\'s Turn\n");
			printf("\n");
		}
		if(currentPlayers[*track].colour == 'B') {
			printf("Blue\'s Turn\n");
			printf("\n");
		}
		if(currentPlayers[*track].colour == 'Y') {
			printf("Yellow\'s Turn\n");
			printf("\n");
		}
		if(currentPlayers[*track].colour == 'W') {
			printf("White\'s Turn\n");
			printf("\n");
		}
	}

}

move_t* getlist(game_t* gameProcess, int number) {
	move_t* m1 = gameProcess->head;
	for(int m = 0; m < number; m++) {
		m1 = m1->next;
	}
	return m1;
}

void save(char* file_name, uint8_t height, uint8_t width, uint8_t player, int turn, game_t* gameProcess) {
	FILE* file;

	file = fopen(file_name, "w+");

	fwrite(&width, 1, 1, file);

	fwrite(&height, 1, 1, file);

	fwrite(&player, 1, 1, file);

	for(int i = 0; i < turn; i++) {
		coordinate result = {getlist(gameProcess,i)->x, getlist(gameProcess,i)->y, 0};
		move_data temp;
		temp.move = result;
		uint32_t raw_move_data = temp.raw_move_data;
		fwrite(&raw_move_data, 4, 1, file);
	}

	fclose(file);
	printf("Game Saved\n");
	printf("\n");

}

void playfrom(int turnFrom, move_data* dataload, int *fileLength, int* winner, int* track, int* turnload) {

	colour = malloc(sizeof(char)*player);
	gridMap = malloc(sizeof(grid_t)*height);
	for(int i = 0; i < height; i++) {
		gridMap[i] = malloc(sizeof(grid_t)*width);
	}
	for(int m = 0; m < height; m++) {
		for(int n = 0; n < width; n++) {
			gridMap[m][n].owner = NULL;
			gridMap[m][n].atom_count = 0;
		}
	}
	gameProcess = malloc(sizeof(game_t));
	//malloc and initialize.
	currentPlayers = malloc(sizeof(player_t)*player);
	for(int i = 0; i < player; i++) {
		currentPlayers[i].grids_owned = 0;
		if(i == 0) {
			currentPlayers[i].colour = 'R';
		}else if(i == 1) {
			currentPlayers[i].colour = 'G';
		}else if(i == 2) {
			currentPlayers[i].colour = 'P';
		}else if(i == 3) {
			currentPlayers[i].colour = 'B';
		}else if(i == 4) {
			currentPlayers[i].colour = 'Y';
		}else if(i == 5) {
			currentPlayers[i].colour = 'W';
		}
	}
	if(turnFrom > *fileLength) {
		turnFrom = *fileLength;
	}

	for(int i = 0; i < turnFrom; i++) {
		place(dataload[i].move.x, dataload[i].move.y,gridMap,currentPlayers,track,player,width,height,turnload,winner,gameProcess);
	}

	printf("Game Ready\n");

	if(turnFrom % player == 0) {printf("Red\'s Turn\n"); printf("\n");}
	if(turnFrom % player == 1) {printf("Green\'s Turn\n"); printf("\n");}
	if(turnFrom % player == 2) {printf("Purple\'s Turn\n"); printf("\n");}
	if(turnFrom % player == 3) {printf("Blue\'s Turn\n"); printf("\n");}
	if(turnFrom % player == 4) {printf("Yellow\'s Turn\n"); printf("\n");}
	if(turnFrom % player == 5) {printf("White\'s Turn\n"); printf("\n");}
	startGame = 1;

}

int main(int argc, char* argv[]) {

	while(1) {

		char command[MAX_LINE];
		fgets(command,MAX_LINE,stdin);

		if(command[0] == '\n') {
			continue;
		}

		char* token = strtok(command, " \n");
		char *command_first = token;

		char *inputN[3];
		int counter = 1;
		int index = 0;

		while(token != NULL) {
			token= strtok(NULL, " \n");
			if(token != NULL) {
				inputN[index] = token;
				index = index + 1;
				counter = counter + 1;
			}
		}


		if(strcmp(command_first, "HELP") == 0) {
			help();
			continue;
		}
		if(strcmp(command_first, "QUIT") == 0) {
			if(startGame == 1) {
				free(colour);
				for(int i = 0; i < height; i++) {
					free(gridMap[i]);
				}
				free(gridMap);

				free(currentPlayers);
				if((placeUsed == 1 || (gameLoaded == 1 && turnload != 0)) && cannotUndo != 1) {
					while(gameProcess->head->next != NULL){
						gameProcess->head = gameProcess->head->next;
						free(gameProcess->head->parent);
					}
					free(gameProcess->head);
				}
				free(gameProcess);
			}

			if(gameLoaded == 1) {
				free(dataload);
			}

			quit();
			return 0;
		}
		if(strcmp(command_first, "DISPLAY") == 0) {
			if(startGame == 1) {
				display(width,height, gridMap);
			}else {
				printf("Invalid Command\n");
				printf("\n");
			}

		}
		if(strcmp(command_first, "START") == 0) {
			if(counter < 4) {
				printf("Missing Argument\n");
				printf("\n");
				continue;
			}else if(counter > 4) {
				printf("Too Many Arguments\n");
				printf("\n");
				continue;
			}else {// Input for command START is Valid.
				if(checkDigit(strlen(inputN[0]), inputN[0],0) == 1 ||
				 	checkDigit(strlen(inputN[1]), inputN[1],0) == 1 ||
					checkDigit(strlen(inputN[2]), inputN[2],0) == 1) {
					printf("Invalid command arguments\n");
					printf("\n");
					continue;
				}
				else {
					player = atoi(inputN[0]);
					width = atoi(inputN[1]);
					height = atoi(inputN[2]);

					if(player < MIN_PLAYERS || player > MAX_PLAYERS) {
						printf("Invalid command arguments\n");
						printf("\n");
						continue;
					}else if(width < MIN_WIDTH || width > MAX_WIDTH || height < MIN_HEIGHT || height > MAX_HEIGHT) {
						printf("Invalid command arguments\n");
						printf("\n");
						continue;
					}else if(player > width*height) {
						printf("Cannot Start Game\n");
						printf("\n");
						continue;
					}else if (startGame == 1) {
						printf("Invalid command arguments\n");
						printf("\n");
						continue;
					}else if(gameLoaded == 1){
						printf("Invalid Command\n");
						continue;
					}else {
						startGame = 1;
						printf("Game Ready\n");
						printf("Red\'s Turn\n");
						printf("\n");

						colour = malloc(sizeof(char)*player);
						gridMap = malloc(sizeof(grid_t)*height);

						for(int i = 0; i < height; i++) {
							gridMap[i] = malloc(sizeof(grid_t)*width);
						}
						gameProcess = malloc(sizeof(game_t));
						currentPlayers = malloc(sizeof(player_t)*player);

						for(int i = 0; i < player; i++) {
							currentPlayers[i].grids_owned = 0;
							if(i == 0) {
								currentPlayers[i].colour = 'R';
							}else if(i == 1) {
								currentPlayers[i].colour = 'G';
							}else if(i == 2) {
								currentPlayers[i].colour = 'P';
							}else if(i == 3) {
								currentPlayers[i].colour = 'B';
							}else if(i == 4) {
								currentPlayers[i].colour = 'Y';
							}else if(i == 5) {
								currentPlayers[i].colour = 'W';
							}
						}

						for(int m = 0; m < height; m++) {
							for(int n = 0; n < width; n++) {
								gridMap[m][n].owner = NULL;
								gridMap[m][n].atom_count = 0;
							}
						}

						for(int i = 0; i < player; i++) {
							if(i == 0) {
								colour[i] = 'R';
							}else if(i == 1) {
								colour[i] = 'G';
							}else if(i == 2) {
								colour[i] = 'P';
							}else if(i == 3) {
								colour[i] = 'B';
							}else if(i == 4) {
								colour[i] = 'Y';
							}else if(i == 5) {
								colour[i] = 'W';
							}
						}

					}
				}
			}
		}
		if(strcmp(command_first, "PLACE") == 0) {
			if(startGame == 0) {
				printf("Invalid Command\n");
				continue;
			}
			if(counter != 3) {
				printf("Invalid Coordinates\n");
				continue;
			}else {
				if(checkDigit(strlen(inputN[0]), inputN[0],0) == 1 ||
					checkDigit(strlen(inputN[1]), inputN[1],0) == 1){
						printf("Invalid command arguments\n");
						continue;
				}else {
					int xCo = atoi(inputN[0]);
					int yCo = atoi(inputN[1]);
					if(xCo < 0 || xCo >= width || yCo >= height || yCo < 0) {
						printf("Invalid Coordinates\n");
						continue;
					}else {
						placeUsed = 1;
						undotrack = 0;
						place(xCo,yCo,gridMap,currentPlayers,&track,player,width,height,&turn,&winner,gameProcess);

						if(winner == 1) {
							free(colour);
							for(int i = 0; i < height; i++) {
								free(gridMap[i]);
							}
							free(gridMap);


							while(gameProcess->head->next != NULL){
								gameProcess->head = gameProcess->head->next;
								free(gameProcess->head->parent);
							}
							free(gameProcess->head);
							free(gameProcess);
							free(currentPlayers);

							if(gameLoaded == 1) {
								free(dataload);
							}

							return 0;
						}
					}
				}
			}
		}
		if(strcmp(command_first,"STAT") == 0) {
			if(startGame == 0) {
				printf("Game Not In Progress\n");
				continue;
			}
			for(int i = 0; i < player; i++) {
				if(i == 0) {
					printf("Player Red:\n");
				}else if(i == 1) {
					printf("Player Green:\n");
				}else if(i == 2) {
					printf("Player Purple:\n");
				}else if(i == 3) {
					printf("Player Blue:\n");
				}else if(i == 4) {
					printf("Player Yellow:\n");
				}else if(i == 5) {
					printf("Player White\n");
				}
				if(currentPlayers[i].grids_owned == 0 && turn > player) {
					printf("Lost\n");
				}else {
					printf("Grid Count: %d\n", currentPlayers[i].grids_owned);
				}
				printf("\n");
			}
		}
		if(strcmp(command_first, "SAVE") == 0) {
			if(counter != 2) {
				printf("Invalid Command\n");
				printf("\n");
				continue;
			}else {
				if(strlen(inputN[0]) > MAX_LINE) {
					printf("Invalid Command\n");
					continue;
				}
				int existence = access(inputN[0],F_OK);
				if(existence == 0) {
					printf("File Already Exists\n");
					printf("\n");
					continue;
				}
				file_name = inputN[0];
				save(file_name,height,width,player,turn,gameProcess);
			}


		}
		if(strcmp(command_first, "LOAD") == 0) {
			if(counter != 2) {
				printf("Invalid Command\n");
				printf("\n");
				continue;
			}else {
				if(strlen(inputN[0]) > MAX_LINE) {
					printf("Invalid Command\n");
					continue;
				}
				file_name = inputN[0];
				int checkExistence = access(file_name,F_OK);
				if(checkExistence == -1) {
					printf("Cannot Load Save\n");
					continue;
				}
				if(gameLoaded == 0) {
					printf("Game Loaded\n");
					printf("\n");
					FILE* file;

					file = fopen(inputN[0], "r");
					fseek(file, 0, SEEK_END);
					fileLength = (ftell(file)-3)/4;
					dataload = malloc(sizeof(move_data)*fileLength);
					rewind(file);
					fread(&width, 1, 1, file);
					fread(&height, 1, 1, file);
					fread(&player, 1, 1, file);
					for(int i = 0; i < fileLength; i++) {
						uint32_t load_move_data;
						fread(&load_move_data, 4 , 1, file);
						dataload[i].raw_move_data = load_move_data;
					}
					gameLoaded = 1;
				}else {
					printf("Restart Application To Load Save\n");
					continue;
				}

			}
		}
		if(strcmp(command_first, "PLAYFROM") == 0) {
			if(counter != 2) {
				printf("Invalid Command\n");
				printf("\n");
				continue;
			}else {

				if(strcmp(inputN[0], "END") == 0) {
					 turnload = fileLength;
				}else {
					if(checkDigit(strlen(inputN[0]), inputN[0],0) == 1) {
						printf("Invalid Command\n");
						continue;
					}else {
						turnload = atoi(inputN[0]);
						if(turnload < 0) {
							printf("Invalid Turn Number\n");
							continue;
						}else if(gameLoaded == 0) {
							printf("Invalid Command\n");
							continue;
						}
					}
				}
				playfrom(turnload, dataload, &fileLength,&winner,&track,&turn);
			}
		}
		if(strcmp(command_first,"UNDO") == 0) {
			if(counter != 1) {
				printf("Invalid Command\n");
				continue;
			}
			if(startGame == 0) {
				printf("Cannot Undo\n");
				continue;
			}
			if(turn == 0) {
				printf("Cannot Undo\n");
				continue;
			}

			for(int i = 0; i < player; i++) {
				currentPlayers[i].grids_owned = 0;
				if(i == 0) {
					currentPlayers[i].colour = 'R';
				}else if(i == 1) {
					currentPlayers[i].colour = 'G';
				}else if(i == 2) {
					currentPlayers[i].colour = 'P';
				}else if(i == 3) {
					currentPlayers[i].colour = 'B';
				}else if(i == 4) {
					currentPlayers[i].colour = 'Y';
				}else if(i == 5) {
					currentPlayers[i].colour = 'W';
				}
			}

			for(int m = 0; m < height; m++) {
				for(int n = 0; n < width; n++) {
					gridMap[m][n].owner = NULL;
					gridMap[m][n].atom_count = 0;
				}
			}

			turn = 0;
		  track = 0;
			undotrack = 1;
			report = 1;
			move_t* getlast = gameProcess->head;
			move_t* undomove = gameProcess->head;

			while(getlast->next != NULL) {
				getlast = getlast->next;
			}

			if(cannotUndo == 1) {
				printf("Cannot Undo\n");
				continue;
			}
			if(getlast == gameProcess->head){
				free(gameProcess->head);
				gameProcess->head = NULL;
				cannotUndo = 1;
				printf("Red\'s Turn\n");
				printf("\n");
				continue;
			} else {
				getlast = getlast->parent;
				free(getlast->next);
				getlast->next = NULL;
			}

			while(undomove->next != NULL) {
				place(undomove->x,undomove->y,gridMap,currentPlayers, &track,player,width,height,&turn,&winner,gameProcess);
				undomove = undomove->next;
			}
			place(undomove->x,undomove->y,gridMap,currentPlayers, &track,player,width,height,&turn,&winner,gameProcess);

			if(currentPlayers[track].colour == 'R') {
				printf("Red\'s Turn\n");
				printf("\n");
			}
			if(currentPlayers[track].colour == 'G') {
				printf("Green\'s Turn\n");
				printf("\n");
			}
			if(currentPlayers[track].colour == 'P') {
				printf("Purple\'s Turn\n");
				printf("\n");
			}
			if(currentPlayers[track].colour == 'B') {
				printf("Blue\'s Turn\n");
				printf("\n");
			}
			if(currentPlayers[track].colour == 'Y') {
				printf("Yellow\'s Turn\n");
				printf("\n");
			}
			if(currentPlayers[track].colour == 'W') {
				printf("White\'s Turn\n");
				printf("\n");
			}
			report = 0;
		}

	}
  return 0;
}
