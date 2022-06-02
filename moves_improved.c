#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "colors.h"

#define ASSERT(n) \
if(!(n)){ \
    printf(RED "====== ERROR\n" YEL "%s\n" reset, #n); \
    printf(RED "file: " reset "%s ", __FILE__); \
    printf(RED "line: " reset "%d\n", __LINE__); \
    exit(1); \
}

#define BOARD_SIZE 64
// VBOARD: virtual board
#define VBOARD_WIDTH 10
#define VBOARD_HEIGHT 12
#define VBOARD_SIZE 120
#define BOARD_TO_VBOARD_OFFSET 21

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
	int board[VBOARD_SIZE];
	int ply;
	SIDE stm;
	int enPas;
	int castlePermission;
} BOARD_STATE;

void parseFEN(BOARD_STATE *bs, char *fen);
int sq64to120(int sq64);
int sq120to64(int sq120);

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
const char pieceChar[] = ".PNBRQKpnbrqkx";
const char castleChar[] = "KQkq";
const int numDirections[] = {8, 4, 4, 8, 8};
const int translation[][8] = {
	{-21, -12, 8, 19, 21, 12, -8, -19}, // knights 0
	{TSW, TNW, TNE, TSE}, // bishops TRT
	{TDN, TLF, TUP, TRT}, // rooks 2
	{TSW, TNW, TNE, TSE, TDN, TLF, TUP, TRT}, // queens 3
	{TSW, TNW, TNE, TSE, TDN, TLF, TUP, TRT}  // kings 4
};

// print_board opts
enum { OPT_64_BOARD = 1, OPT_BOARD_STATE = 2, OPT_VBOARD = 4, OPT_PINNED = 8 };

char sliders[2][BOARD_SIZE], contact[2][BOARD_SIZE];
// should these be arrays of pointers?
char last_slider[2], last_knight[2];

void print_board(BOARD_STATE *bs, int opt){
	if(opt & OPT_VBOARD){
		printf(YEL " ---- Virtual Board ---- \n" reset);
		for(int i = 0; i < VBOARD_SIZE; i++){
			// same horizontal board flip logic as in invertRows()
			int inverted_index = (VBOARD_SIZE - VBOARD_WIDTH) + i - 2 * (i - i % VBOARD_WIDTH);
			printf("%2d ", bs -> board[inverted_index]);
			if((i + 1) % VBOARD_WIDTH == 0){
				puts("");
			}
		}
		puts("");
	}
}

void init(BOARD_STATE *bs){
	for(int i = 0 ; i < VBOARD_SIZE ; i++)
		bs -> board[i] = OFFBOARD;
	for(int i = 0 ; i < BOARD_SIZE ; i++)
		bs -> board[sq64to120(i)] = EMPTY;
	bs -> ply = 1;
}

void genLegalMoves(BOARD_STATE *bs){
	return;
}

void main(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	char testFEN[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
	parseFEN(bs, testFEN);

	print_board(bs, OPT_VBOARD|OPT_64_BOARD);
}

// UTILS BELOW main
/*
utils like fen parser for temporary use while
i make improvements to move generation
*/

int sq64to120(int sq64){
	return sq64 + 21 + 2 * (sq64 - sq64 % 8) / 8;
}

int sq120to64(int sq120){
	return sq120 - 17 - 2 * (sq120 - sq120 % 10) / 10;;
}

// returns sq64 of index, but starting at the 8th rank
// used for printing board and generating FEN
int invertRows(int i){
	return 56 + i - 2 * (i - i % 8);
}

// ranks and files are 1-8; e.g. a file is 1
int frToSq64(int file, int rank){
	return 8 * (rank - 1) + (file - 1);
}

// copy algebraic notation of sq120 into sqStrPtr
void getAlgebraic(char *sqStrPtr, int sq120){
	int sq64 = sq120to64(sq120);
	char sqAN[] = {(sq64 % 8) + 'a', (sq64 - sq64 % 8) / 8 + '1', '\0'};
	if(sq120 == OFFBOARD){ char *p = sqAN; *p++ = '-' ; *p++ = '\0'; }
	strcpy(sqStrPtr, sqAN);
}

// copy fen notation of castling rights into sqStrPtr
void getCastlePermissions(char *sqStrPtr, int cperm){
	int j = 0;
	if(cperm == 0){
		sqStrPtr[j] = '-';
		sqStrPtr[j+1] = '\0';
	} else {
		for(int i = 0 ; i < 4 ; i++){
			if(1 << (3 - i) & cperm){
				sqStrPtr[j] = castleChar[i];
				j++;
			}
		}
		sqStrPtr[j] = '\0';
	}
}

int getType(int piece){
	return (piece - 2) % 6;
}

// TODO: this is risky for EMPTY squares
bool getColor(int piece){
	return piece >= bP && piece <= bK;
}

bool on2ndRank(int sq, bool color){
	return sq - 30 - 50 * color >= 1 && sq - 30 - 50 * color <= 8;
}
bool on7thRank(int sq, bool color){
	return on2ndRank(sq, !color);
}

// TODO: get rid of this silliness
// returns if the enpassant sq can be captured by the side to move
bool enPasCorrectColor(int enPas, bool stm){
	return enPas - 70 + 30 * stm >= 1 && enPas - 70 + 30 * stm <= 8;
}

void parseFEN(BOARD_STATE *bs, char *fen){
	// starting at a8
	int rank = 8, file = 1;
	while(*fen && rank > 0){
		int num = 1, piece;
		switch(*fen){
			case 'p': piece = bP; break;
			case 'r': piece = bR; break;
			case 'n': piece = bN; break;
			case 'b': piece = bB; break;
			case 'q': piece = bQ; break;
			case 'k': piece = bK; break;
			case 'P': piece = wP; break;
			case 'R': piece = wR; break;
			case 'N': piece = wN; break;
			case 'B': piece = wB; break;
			case 'Q': piece = wQ; break;
			case 'K': piece = wK; break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				piece = EMPTY;
				num = *fen - '0';
				break;

			case '/':
			case ' ':
				rank--;
				file = 1;
				fen++;
				continue;

			default:
				printf(RED "Error with FEN\n" reset);
				exit(1);
		}

		if(piece == wK)
			sliders[WHITE][0] = sq64to120(frToSq64(file, rank));
		if(piece == bK)
			sliders[BLACK][0] = sq64to120(frToSq64(file, rank));

		for(int i = 0 ; i < num ; i++){
			bs -> board[sq64to120(frToSq64(file, rank))] = piece;
			file++;
		}
		fen++;
	}

	// stm
	ASSERT(*fen == 'w' || *fen == 'b');
	bs -> stm = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	// castling
	for(int i = 0 ; i < 4 ; i++){
		if(*fen == ' ') break;
		switch(*fen){
			case 'K': bs -> castlePermission |= WKCA; break;
			case 'Q': bs -> castlePermission |= WQCA; break;
			case 'k': bs -> castlePermission |= BKCA; break;
			case 'q': bs -> castlePermission |= BQCA; break;
			default: break;
		}
		fen++;
	}
	fen++;
	ASSERT(bs -> castlePermission >= 0 && bs -> castlePermission <= 0xF);

	// en passant
	if(*fen != '-'){
		file = fen[0] - 'a' + 1;
		rank = fen[1] - '0';
		ASSERT(file >= 1 && file <= 8);
		ASSERT(rank >= 1 && rank <= 8);
		bs -> enPas = sq64to120(frToSq64(file, rank));
	}
}
