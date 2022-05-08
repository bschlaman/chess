#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

#define board      (brd+1)                /* 12 x 16 board: dbl guard band */
#define capt_code  (brd+1+0xBC+0x77)      /* piece type that can reach this*/
#define delta_vec  ((char *) brd+1+0xBC+0xEF+0x77) /* step to bridge certain vector */
/* overlays that interleave other piece info in pos[] */
#define kind (pc+1)
#define cstl (pc+1+NUM_PIECES)
#define pos  (pc+1+NUM_PIECES*2)
#define code (pc+1+NUM_PIECES*3)

#define WHITE  0x20 // 32. why not start at 0?
#define BLACK  0x40 // 64
// NumberOfPieces? = 64
#define NUM_PIECES    (2*WHITE)
#define COLOR  (BLACK|WHITE)
#define GUARD  (COLOR|0x80)
#define DUMMY  (WHITE-1+0x80)
#define PAWNS  0x10

// enum { WHITE, BLACK, NEITHER };
// does offboard need to be -1?
enum { OFFBOARD = -1 };
enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK, CANDIDATESQ };
// enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK, CANDIDATESQ, GUARD, DUMMY };
// enum { KNIGHT, BISHOP, ROOK, QUEEN, KING };
#define KING   7
#define QUEEN  6
#define ROOK   5
#define BISHOP 4
#define KNIGHT 3
#define BPAWN  2
#define WPAWN  1

unsigned char
	pc[NUM_PIECES*4+1], /* piece list, equivalenced with various piece info  */
	brd[0xBC+2*0xEF+1],      /* contains play bord and 2 delta boards  */
	CasRights,               /* one bit per castling, clear if allowed */
	/* piece-number assignment of first row, and piece types */
	array[] = {12, 1, 14, 11, 0, 15, 2, 13,
	    ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK},
	capts[] = {0, C_WPAWN, C_BPAWN, C_KNIGHT, C_BISHOP,
                      C_ROOK, C_QUEEN, C_KING};
char
	queen_dirs[]   = {1, -1, 16, -16, 15, -15, 17, -17},
	// why is this different from queen_dirs
	king_rose[]    = {1,17,16,15,-1,-17,-16,-15},
	knight_rose[]  = {18,33,31,14,-18,-33,-31,-14};


void printBoard(unsigned char *b){
	int invertRow(int i){
		return 0xB0 + i - 2 * (i - i % 0x10);
	}
	for(int i = 0; i < 0xBC + 4; i++){
		if(i && i % 0x10 == 0) printf("\n");
		printf("%c ", b[invertRow(i)] == DUMMY ? 'x' : '-');
	}
	puts("");
}

void board_init(unsigned char *b){
	for(int i = -1; i < 0xBC; i++)
		b[i] = (i-0x22)&0x88 ? GUARD : DUMMY;
}

void piece_init(){
	for(int i=0; i<8; i++){
		kind[array[i]+WHITE] = kind[array[i]] = array[i+8];
		kind[i+PAWNS]       = WPAWN;
		kind[i+PAWNS+WHITE] = BPAWN;
		// where they are in board arr
		pos[array[i]]       = i+0x22;
		pos[array[i]+WHITE] = i+0x92;
		pos[i+PAWNS]        = i+0x32;
		pos[i+PAWNS+WHITE]  = i+0x82;
	}
	// i will purposely not address what i think is a
	// bug that kind is not fully initialized above
	for(int i=0; i < NUM_PIECES; i++) code[i] = capts[kind[i]];
}

void setup(){   /* put pieces on the board according to the piece list */
	// WHITE-8 = 24 since he is using those unused pawn sections for other data (code - x)
	for(int i=0; i<WHITE-8; i++){
		// WHITE + i _are the piece types_
		if(pos[i]      ) board[pos[i]]       = WHITE + i;
		if(pos[i+WHITE]) board[pos[i+WHITE]] = BLACK + i;
	}
}

char asc[] = ".+pnbrqkxxxxxxxx.P*NBRQKXXXXXXXX";

void pboard(char *b, int n, int bin)
{   /* print board of n x n, in hex (bin=1) or ascii */
    int i, j, k;

    for(i=n-1; i>=0; i--)
    {
        for(j=0; j<n; j++)
            if(bin) printf(" %2x", b[16*i+j]&0xFF);
            else    printf(" %c", (b[16*i+j]&0xFF)==GUARD ? '-' :
										// seems like no point in &0x7f
                    asc[kind[(b[16*i+j]&0x7F) - WHITE]
											+ ((b[16*i+j] & WHITE)>>1)]);
        printf("\n");
    }
    printf("\n");
}

int main(){
	board_init(board);
	setup();
	pboard(board, 12, 0);
}
