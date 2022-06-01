#include <stdlib.h>

#define BOARD_SIZE 64
#define VIRTUAL_BOARD_SIZE 120

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

char sliders[2][BOARD_SIZE], contact[2][BOARD_SIZE];
// should these be arrays of pointers?
char last_slider[2], last_knight[2];

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
