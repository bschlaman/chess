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

#define WHITE  0x20     // 32    0b 0010 0000
#define BLACK  0x40     // 64    0b 0100 0000
#define COLOR  (BLACK|WHITE)  // 0b 0110 0000
#define GUARD  (COLOR|0x80)   // 0b 1110 0000
#define DUMMY  (WHITE-1+0x80) // 0b 1001 1111
#define PAWNS  0x10           // 0b 0001 0000
// NumberOfPieces? = 64
#define NUM_PIECES    (2*WHITE)

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

#define PUSH(A,B) stack[msp++] = (A)+(B);
// MoveStack i presume?
int stack[1024],
	// MoveStackPointer
	msp,
	ep1, ep2,
	Promo,
	epSqr;


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

void board_init(char *b){
	for(int i = -1; i < 0xBC; i++)
		b[i] = (i-0x22)&0x88 ? GUARD : DUMMY;
}

void piece_init(){
	for(int i = 0; i < 8; i++){
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
	for(int i = 0; i < WHITE - 8; i++){
		// i is the index of kind
		// when kind is used, WHITE will be subtracted
		if(pos[i]      ) board[pos[i]]       = WHITE + i;
		if(pos[i+WHITE]) board[pos[i+WHITE]] = BLACK + i;
	}
}

char asc[] = ".+pnbrqkxxxxxxxx.P*NBRQKXXXXXXXX";

void pboard(char *b, int n, int bin)
{   /* print board of n x n, in hex (bin=1) or ascii */
    int i, j, k;

		// i is row; j is column (weird)
		// also why have n be variable here?
    for(i=n-1; i>=0; i--)
    {
        for(j=0; j<n; j++)
						// &0xFF chops off anything outside a byte
						// this is unnecessary since it was declared as uchar
            if(bin) printf(" %2x", b[16*i+j]&0xFF);
            else    printf(" %c", (b[16*i+j]&0xFF) == GUARD ? '-' :
							// seems like no point in &0x7f
							// middle squares will be DUMMY
							// DUMMY: 0b 1001 1111
							// DUMMY & WHITE = 0
							// sometimes the kind index will be -1 in case of DUMMY... what?
							// luckily kind[-1] = pc[0] = 0... assuming thats now pc was initialized...
                    asc[
											// subtracting WHITE gives (0-31) for white pcs
											// and (32-63) for black pieces
											kind[ (b[16*i+j]&0x7F) - WHITE ]
											// outer parens needed for order of ops i guess
											// b[x] & WHITE is 0x00 for black pieces
											// b[x] & WHITE is 0x20 for white pieces
											// (b[x] & WHITE)>>1 = 0x10 for white
											+ ( (b[16*i+j] & WHITE)>>1 )
										]);

        printf("\n");
    }
    printf("\n");
}

void move_gen(int color, int lastply, int d){
	// color = WHITE or BLACK
	int k, p, forward;
	int first_move = msp;

	k = pos[color-WHITE];   /* position of my King */
	// 16, -16
	forward = 48 - color;            /* forward step */
	rank = 0x58 - (forward>>1);     /* 4th/5th rank */
	prank = 0xD0 - 5*(color>>1);    /* 2nd/7th rank */
	// lastply stores ep square and checks
	ep_flag = lastply>>24&0xFF;
	// msp = MoveStackPointer??
	ep1 = ep2 = msp; Promo = 0;

	// starts with pintest
	for(p=FirstSlider[COLOR-color]; p<COLOR-WHITE+16-color; p++){
		j = pos[p]; /* enemy slider */
		if(j==0) continue;  /* currently captured */
	}

}

int main(){
	board_init(board);
	piece_init();
	setup();
	pboard(board, 12, 1);
}
