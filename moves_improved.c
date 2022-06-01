#include <stdlib.h>

#define BOARD_SIZE 64
#define VIRTUAL_BOARD_SIZE 120
// for use in sq64to120
#define BOARD_TO_VIRT_BOARD_OFFSET 21

typedef enum { WHITE, BLACK } SIDE;
enum {
	OFFBOARD, EMPTY,
	wP, wN, wB, wR, wQ, wK,
	bP, bN, bB, bR, bQ, bK
};
enum { KNIGHT, BISHOP, ROOK, QUEEN, KING } PIECE_TYPE;
// 0 0 0 0
enum { WKCA = 8, WQCA = 4, BKCA = 2, BQCA = 1 };

typedef struct {
	int board[VIRTUAL_BOARD_SIZE];
	int ply;
	SIDE stm;
	int enPas;
	int castlePermission;
} BOARD_STATE;

// translate up, right, down, left, etc.
enum {
	TUP =  10,
	TRT =   1,
	TDN = -10,
	TLF =  -1,
	TNW =   9,
	TNE =  11,
	TSE =  -9,
	TSW = -11,
};
const int numDirections[] = {8, 4, 4, 8, 8};
const int translation[][8] = {
	{-21, -12, 8, 19, 21, 12, -8, -19}, // knights 0
	{TSW, TNW, TNE, TSE}, // bishops TRT
	{TDN, TLF, TUP, TRT}, // rooks 2
	{TSW, TNW, TNE, TSE, TDN, TLF, TUP, TRT}, // queens 3
	{TSW, TNW, TNE, TSE, TDN, TLF, TUP, TRT}  // kings 4
};

char sliders[2][BOARD_SIZE], contact[2][BOARD_SIZE];
// should these be arrays of pointers?
char last_slider[2], last_knight[2];

int sq64to120(int sq64){
	return sq64 + BOARD_TO_VIRT_BOARD_OFFSET + 2 * (sq64 - sq64 % 8) / 8;
}

void init(BOARD_STATE *bs){
	for(int i = 0 ; i < VIRTUAL_BOARD_SIZE ; i++)
		bs -> board[i] = OFFBOARD;
	for(int i = 0 ; i < BOARD_SIZE ; i++)
		bs -> board[sq64to120(i)] = EMPTY;
	bs -> ply = 1;
}

void genLegalMovesImproved(BOARD_STATE *bs){
	return;
}

extern int parseFEN(BOARD_STATE *bs, char *fen);

void main(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	char testFEN[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
	parseFEN(bs, testFEN);

	// print board
	printBoard(bs, OPT_64_BOARD);
	printBoard(bs, OPT_BOARD_STATE);
	// printBoard(bs, OPT_PINNED);

	// print moves
	// printLegalMoves(bs);
}


