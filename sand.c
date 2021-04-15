#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>	
#include <time.h>
#include "colors.h"
#include "defs.h"

int parseArgs(char *inputFEN, int argc, char *argv[]){
	int c;
	while((c = getopt(argc, argv, "f:")) != -1){
		switch(c){
			case 'f':
				strcpy(inputFEN, optarg);
				return FEN_MODE;
				break;
			case '?':
				if (optopt == 'f')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return -1;
			default:
				fprintf(stderr, "Error with arg parsing", optopt);
				exit(1);
		}
	}
	return NORMAL_MODE;
}

void initRand(){
	time_t t;
	srand((unsigned) time(&t));
}
int randInt(int lb, int ub){
	return rand() % (ub - lb + 1) + lb;
}

const int levels = 3;
struct node {
	int data;
	char name[30];
	struct node *children[4];
};

int indent(int data){
	return levels - data + 1;
}
int numChildren(){
	return sizeof(((struct node *)0) -> children) / sizeof(((struct node *)0) -> children[0]);
}

// returns pointer to node
struct node* newNode(int data){
	if(data == 0){
		return NULL;
	}
	struct node* n = (struct node*)malloc(sizeof(struct node));
	n -> data = data;
	for(int i = 0 ; i < numChildren() ; i++){
		n -> children[i] = newNode(data - 1);
	}
	return n;
}


void printNode(struct node* n){
	printf(YEL "level: " reset "%d", n -> data);
	for(int i = 0 ; i < numChildren() ; i++){
		printf("\n");
		for(int j = 0  ; j < indent(n -> data) ; j++){ printf("+"); }
		if(n -> children[i] == NULL){
			printf("null_child ");
		} else {
			printNode(n -> children[i]);
		}
	}
}


void nodeTest(){
	struct node *n = newNode(levels);
	printNode(n);
}

typedef struct {
	int data;
} STATE;
typedef struct {
	STATE hist[10];
} BOARD;
BOARD* init(){
	BOARD *b = malloc(sizeof(BOARD));
	return b;
}

int testEval(){
	return randInt(-5, 5);
}

int genLeg(int moves[][4]){
	for(int i = 0 ; i < 10 ; i++){
		moves[i][2] = -1;
	}
}

int getPinDir(int kingsq, int pinsq){
	int diff = pinsq - kingsq;
	if(diff > 0){
		if(diff % 11 == 0) return 11;
		if(diff % 10 == 0) return 10;
		if(diff % 9 == 0) return 9;
		if(diff % 1 == 0) return 1;
	} else {
		if(diff % 11 == 0) return -11;
		if(diff % 10 == 0) return -10;
		if(diff % 9 == 0) return -9;
		if(diff % 1 == 0) return -1;
	}
	return 0;
}

void printPinned(U64 pinned){
	for(int i = 0 ; i < 64 ; i++){
		printf("%d ", !!(1ULL << i & pinned));
		if((i + 1) % 8 == 0) printf("\n");
	}
}

int sq120to64(int sq120){
	return sq120 - 17 - 2 * (sq120 - sq120 % 10) / 10;;
}

int main(int argc, char *argv[]){
	initRand();
	// arg stuff
	char inputFEN[99];
	int res = parseArgs(inputFEN, argc, argv);

	int moves[255][4];
	genLeg(moves);

	printf("%d ", sizeof(int));
	printf("%d \n", sizeof(unsigned short));

	int kingsq = 74;
	int pinsq = 83;
	int dir = getPinDir(kingsq, pinsq);
	printf("k: %d\np: %d\ndir: %d\n", kingsq, pinsq, dir);

	U64 pinned = 0ULL;
	pinned |= 1ULL << 25;
	printPinned(pinned);
	int and = 1ULL << sq120to64(52) & pinned;
	printf("%d\n", and);

	return 0;
}
