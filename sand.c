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

/* capture codes */
#define C_ORTH    1
#define C_DIAG    2
#define C_KNIGHT  4
#define C_SIDE    8
#define C_FORW    0x10
#define C_FDIAG   0x20
#define C_BACKW   0x40
#define C_BDIAG   0x80
#define C_FERZ    (C_FDIAG|C_BDIAG)
#define C_WAZIR   (C_SIDE|C_FORW|C_BACKW)
#define C_KING    (C_FERZ|C_WAZIR)
#define C_BISHOP  (C_FERZ|C_DIAG)
#define C_ROOK    (C_WAZIR|C_ORTH)
// white pawn
#define C_WPAWN   (C_FDIAG)
// black pawn
#define C_BPAWN   (C_BDIAG)
#define C_QUEEN   (C_BISHOP|C_ROOK)
#define C_CONTACT (C_KING|C_KNIGHT)
#define C_DISTANT (C_ORTH|C_DIAG)

#define capt_code  (board+0xBC+0x77)
#define delta_vec  ((char *) board+0xBC+0xEF+0x77)

// set up the board
// board of size 0xBC + 2*0xEF for the delta_vec
unsigned char board[0xBC + 2*0xEF];

// constants for this 16x12 board
char
	queen_dir[]   = {1, -1, 16, -16, 15, -15, 17, -17},
	king_rose[]   = {1,17,16,15,-1,-17,-16,-15},
	knight_rose[] = {18,33,31,14,-18,-33,-31,-14};

void delta_init(){
	/* contact captures (cannot be blocked) */
	capt_code[ 15] = capt_code[ 17] = C_FDIAG;
	capt_code[-15] = capt_code[-17] = C_BDIAG;
	capt_code[  1] = capt_code[ -1] = C_SIDE;
	capt_code[ 16] = C_FORW;
	capt_code[-16] = C_BACKW;
	// so i can see where the middle is
	capt_code[0] = 3;
	for(int i = 0 ; i < 8 ; i++){
		capt_code[knight_rose[i]] = C_KNIGHT;
		delta_vec[knight_rose[i]] = knight_rose[i];
		int scan_direction = queen_dir[i];
		int dist_capt_code = i < 4 ? C_ORTH : C_DIAG;
		int offset = 0;
		for(int j = 0; j < 7; j++){
			delta_vec[offset+=scan_direction] = scan_direction;
			if(j) capt_code[offset] = dist_capt_code;
		}
	}
}

int main(){
	// =========================
	// improving move generation
	// =========================
	// init board
	for(int i = 0; i<0xBC; i++) board[i] = (i-0x22)&0x88 ? '-' : 'X';

	delta_init();

	puts("capt_code: ");
	for(int i = -0x77; i <= 0x77; i++){
		if ((i + 0x77) % 16 == 0) puts("");
		// printf("%c ", capt_code[i] ? '.' : 'X');
		printf("%d ", capt_code[i] % 10);
	}
	puts("");

	puts("delta_vec: ");
	for(int i = -0x77; i <= 0x77; i++){
		if ((i + 0x77) % 16 == 0) puts("");
		printf("%2d ", delta_vec[i]);
	}
	puts("");
}
