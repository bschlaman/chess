#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// MACROS
#define ABS(N) ((N<0)?(-N):(N))

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

/* Piece counts hidden in the unused Pawn section, indexed by color */
#define LastKnight  (code-4)
#define FirstSlider (code-3)
#define FirstPawn   (code-2)

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

void delta_init(){
	/* contact captures (cannot be blocked) */
	capt_code[ 15] = capt_code[ 17] = C_FDIAG;
	capt_code[-15] = capt_code[-17] = C_BDIAG;
	capt_code[  1] = capt_code[ -1] = C_SIDE;
	capt_code[ 16] = C_FORW;
	capt_code[-16] = C_BACKW;
	for(int i = 0; i < 8; i++){
		capt_code[knight_rose[i]] = C_KNIGHT;
		delta_vec[knight_rose[i]] = knight_rose[i];
		int scan_direction = queen_dirs[i];
		int dist_capt_code = i < 4 ? C_ORTH : C_DIAG;
		int offset = 0;
		for(int j = 0; j < 7; j++){
			delta_vec[offset+=scan_direction] = scan_direction;
			if(j) capt_code[offset] = dist_capt_code;
		}
	}
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
	/* set castling spoilers (King and both original Rooks) */
	cstl[0]        = WHITE;
	cstl[12]       = WHITE>>2;
	cstl[13]       = WHITE>>4;
	cstl[0 +WHITE] = BLACK;
	cstl[12+WHITE] = BLACK>>2;
	cstl[13+WHITE] = BLACK>>4;

	/* piece counts (can change when we compactify lists, or promote) */
	LastKnight[WHITE]  =  2;
	FirstSlider[WHITE] = 11;
	FirstPawn[WHITE]   = 16;
	LastKnight[BLACK]  =  2+WHITE;
	FirstSlider[BLACK] = 11+WHITE;
	FirstPawn[BLACK]   = 16+WHITE;
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
	int first_move = msp;
	int pstack[12], ppos[12], psp=0;

	int k = pos[color-WHITE];   /* position of my King */
	// 16, -16
	int forward = 48 - color;            /* forward step */
	int rank = 0x58 - (forward>>1);     /* 4th/5th rank */
	int prank = 0xD0 - 5*(color>>1);    /* 2nd/7th rank */
	// lastply stores ep square and checks
	int ep_flag = lastply>>24&0xFF;
	// msp = MoveStackPointer??
	ep1 = ep2 = msp; Promo = 0;

	int piece; // piece = board[i] (WPAWN + color, KING + color, etc.)

	// starts with pintest
	for(int i = FirstSlider[COLOR-color]; i < 16+color-WHITE; i++){
		int opp_slider_pos = pos[i]; /* enemy slider */
		if(opp_slider_pos==0) continue;  /* currently captured */
		// the utility of capture codes:
		// capt_code[delta]: what type of piece can capture
		// code[i]: the capture type of piece at pos[i]
		// C_DISTANT: it is a distant check, not C_ORTH | C_DIAG
		if(capt_code[opp_slider_pos - k] & code[i] & C_DISTANT){   /* slider aimed at our king */
			// vector is normalized to magnitude 1sq!
			int vector = delta_vec[opp_slider_pos - k];
			int offset = k;     /* trace ray from our King */
			while((piece = board[offset+=vector]) == DUMMY);
			if(offset == opp_slider_pos){ // distance check
				in_check += 2;
				opp_checker_pos = opp_slider_pos;
				check_vector = vector;
			} else if(piece & color){
				// if this is our piece
				// csq: candidate square
				int csq = offset;
				while(board[csq+=vector] == DUMMY);
				if(csq == opp_slider_pos){
					// pinned at offset
					/* remove our piece from piece list */
					/* and put on pin stack             */
					piece -= WHITE; // hmmm why not piece -= color?
					ppos[psp] = pos[piece];
					pstack[psp++] = piece;
					pos[piece] = 0;
					z = offset<<8;
					if(kind[piece] == BPAWN || kind[piece] == WPAWN){
						csq = offset + forward;
						if(ABS(vector) == 16){ // pawn along file
							if(!(board[csq] & COLOR)){
								PUSH(z, csq);
								csq += forward; Promo++;
								if(!(board[csq] & COLOR | (rank^csq))) PUSH(z, csq|csq<<24);
							}
						} else {
							// diagnonal pin
							if(csq+1==opp_slider_pos) { Promo++; PUSH(z,csq+1) }
							if(csq-1==opp_slider_pos) { Promo++; PUSH(z,csq-1) }
						}
					} else if(code[piece]&capt_code[opp_slider_pos-k]&C_DISTANT) {
						// not a pawn; slider can move along ray
						csq = offset;
						do {
							csq += vector;
							PUSH(z, csq);
						} while(csq != opp_slider_pos);
						csq = offset;
						while((csq -= vector) != k) PUSH(z, csq);
					}
				}
			}
		}
	} // done with pin test
	/* determine if opponent's move put us in contact check */
	// remember contact check includes knights
	csq = lastply&0xFF;
	if(capt_code[csq - k] & code[board[csq]-WHITE] & C_CONTACT){
		opp_checker_pos = csq; in_check++;
		check_vector = delta_vec[opp_slider_pos-k];
	}
	/* determine how to proceed based on check situation    */
	if(in_check){
		/* purge moves with pinned pieces if in check   */
		msp = first_move;
		if(in_check > 2) goto King_Moves; /* double check */
		if(opp_checker_pos == ep_flag) { ep1 = msp; goto ep_Captures; }
		goto Regular_Moves;
	}
	/* generate castlings */
	ep1 = msp;
	if(!(color&CasRights)){
		if(!(board[k+1]^DUMMY|board[k+2]^DUMMY|
					CasRights&color>>4))
			PUSH(k<<8,k+2+0xB0000000+0x3000000)
				if(!(board[k-1]^DUMMY|board[k-2]^DUMMY|board[k-3]^DUMMY|
							CasRights&color>>2))
					PUSH(k<<8,k-2+0xB0000000-0x4000000)
	}
}

int main(){
	board_init(board);
	delta_init();
	piece_init();
	setup();
	pboard(board, 12, 0);
	pboard(board, 12, 1);
}
