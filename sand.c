#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>	
#include <time.h>
#include <limits.h>
#include "colors.h"
#include "defs.h"

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

void printBits(size_t const size, void const * const ptr){
	// casting to uchar so we can iterate by "sizes" (bytes)
	// b[0], b[1], ... are all of size uchar
	unsigned char *b = (unsigned char*) ptr;
	for(int byte = size - 1; byte >= 0; byte--){
		for(int bit = 7; bit >= 0; bit--){
			printf("%u", (b[byte] >> bit) & 1);
		}
	}
	puts("");
}

int sq120to64(int sq120){
	return sq120 - 17 - 2 * (sq120 - sq120 % 10) / 10;;
}

int invertRows(int i){
	return 0xB0 + i - 2 * (i - i % 0x10);
}

#define capt_code  (board+0xBC+0x77)      /* piece type that can reach this*/
#define delta_vec  ((char *) board+0xBC+0xEF+0x77) /* step to bridge certain vector */
/* capture codes */
#define C_ORTH    1
#define C_DIAG    2
#define C_KNIGHT  4
#define C_SIDE    8
#define C_FORW    0x10
#define C_FDIAG   0x20
#define C_BACKW   0x40
#define C_BDIAG   0x80

int main(){
	initRand();
	// make and print board
	unsigned char b[0xBC];
	for(int i = 0; i<0xBC; i++) b[i] = (i-0x22)&0x88 ? '-' : 'X';
	for(int i = 0; i<0xBC + 4; i++){
		if(i%0x10 == 0) printf("\n");
		printf("%c ", (invertRows(i)-0x22)&0x88 ? '-' : 'X');
	}
	printf("\n================\n");

	// =========================
	// improving move generation
	// =========================
	// set up the board
	unsigned char board[0xBC];
	for(int i = 0; i<0xBC; i++) board[i] = (i-0x22)&0x88 ? '-' : 'X';
	// constants for this 16x12 board
	char
		queen_dir[]   = {1, -1, 16, -16, 15, -15, 17, -17},
		king_rose[]   = {1,17,16,15,-1,-17,-16,-15},
		knight_rose[] = {18,33,31,14,-18,-33,-31,-14};
	// start the clock
	clock_t t = clock();
	/* contact captures (cannot be blocked) */
	capt_code[ 15] = capt_code[ 17] = C_FDIAG;
	capt_code[-15] = capt_code[-17] = C_BDIAG;
	capt_code[  1] = capt_code[ -1] = C_SIDE;
	capt_code[ 16] = C_FORW;
	capt_code[-16] = C_BACKW;
	for(int i = 0 ; i < 8 ; i++){
		capt_code[knight_rose[i]] = C_KNIGHT;
		delta_vec[knight_rose[i]] = knight_rose[i];
	}
	printf("size of size_t: %d\n", sizeof(unsigned char));
	for(int i = -0x77; i <= 0x77; i++){
		printf("%d ", capt_code[i]);
	}
	puts("");
	printf("size of char: %d\n", sizeof(char));
	printf("size of uchar: %d\n", sizeof(unsigned char));
	char a = -3;
	printf("a: %u\n", (unsigned char)a);
}
