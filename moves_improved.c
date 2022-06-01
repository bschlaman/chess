#include <stdlib.h>

#define BOARD_SIZE 64

typedef enum { WHITE, BLACK } SIDE;
// 0 0 0 0
enum { WKCA = 8, WQCA = 4, BKCA = 2, BQCA = 1 };
enum {
	OFFBOARD, EMPTY,
	wP, wN, wB, wR, wQ, wK,
	bP, bN, bB, bR, bQ, bK
};
enum { KNIGHT, BISHOP, ROOK, QUEEN, KING } TYPE;

typedef struct {
	int board[120];
	int ply;
	SIDE stm;
	int enPas;
	int castlePermission;
} BOARD_STATE;

char sliders[2][BOARD_SIZE], contact[2][BOARD_SIZE];
// should these be arrays of pointers?
char last_slider[2], last_knight[2];

void resetBoard(BOARD_STATE *bs){
	int i;
	for(i = 0 ; i < 120 ; i++){
		bs -> board[i] = OFFBOARD;
	}
	for(i = 0 ; i < 64 ; i++){
		bs -> board[sq64to120(i)] = EMPTY;
	}
	bs -> stm = NEITHER;
	bs -> castlePermission = 0;
	bs -> enPas = OFFBOARD;
	bs -> ply = 1;
	bs -> kingSq[WHITE] = E1;
	bs -> kingSq[BLACK] = E8;
	bs -> pinned = 0ULL;
}
void init(){
	
}

void genLegalMovesImproved(BOARD_STATE *bs){
	return;
}

void main(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	char testFEN[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
	parseFEN(bs, testFEN);

	// print board
	printBoard(bs, OPT_64_BOARD);
	printBoard(bs, OPT_BOARD_STATE);
	// printBoard(bs, OPT_PINNED);

	// print moves
	printLegalMoves(bs);
}
