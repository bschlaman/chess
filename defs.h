#include <stdlib.h>

#define DEBUG
#ifndef DEBUG
#define ASSERT(n)
#else
#define ASSERT(n) \
if(!(n)){ \
    printf(RED "====== ERROR\n" YEL "%s\n" reset, #n); \
    printf(RED "file: " reset "%s ", __FILE__); \
    printf(RED "line: " reset "%d\n", __LINE__); \
    exit(1); \
}
#endif

typedef enum { false, true } bool;

#define START_FEN  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

enum {
    A1 = 21, B1, C1, D1, E1, F1, G1, H1,
    A2 = 31, B2, C2, D2, E2, F2, G2, H2,
    A3 = 41, B3, C3, D3, E3, F3, G3, H3,
    A4 = 51, B4, C4, D4, E4, F4, G4, H4,
    A5 = 61, B5, C5, D5, E5, F5, G5, H5,
    A6 = 71, B6, C6, D6, E6, F6, G6, H6,
    A7 = 81, B7, C7, D7, E7, F7, G7, H7,
    A8 = 91, B8, C8, D8, E8, F8, G8, H8, NO_SQ
};

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

// 0 0 0 0
enum { WKCA = 8, WQCA = 4, BKCA = 2, BQCA = 1 };
enum { WHITE, BLACK, NEITHER };
enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK, CANDIDATESQ };
enum { KNIGHT, BISHOP, ROOK, QUEEN, KING };
// move encoding, using the chess programming wiki method
// 0  0	0	0	0	quiet moves
// 1  0	0	0	1	double pawn push
// 2  0	0	1	0	king castle
// 3  0	0	1	1	queen castle
// 4  0	1	0	0	captures
// 5  0	1	0	1	ep-capture
// 8  1	0	0	0	knight-promotion
// 9  1	0	0	1	bishop-promotion
// 10	1	0	1	0	rook-promotion
// 11	1	0	1	1	queen-promotion
// 12	1	1	0	0	knight-promo capture
// 13	1	1	0	1	bishop-promo capture
// 14	1	1	1	0	rook-promo capture
// 15	1	1	1	1	queen-promo capture
enum { PROMOTION = 8, CAPTURE = 4, SPECIAL1 = 2, SPECIAL2 = 1 };

// 000000 000000 0000
// from   to     moveType
typedef unsigned short MOVE;

typedef unsigned long long U64;

typedef struct {
	MOVE move;
	int enPas;
	int castlePermission;
	int capturedPiece;
	// TODO: this is not irreversible
	U64 pinned;
} MOVE_IRREV;

typedef struct {
	// rename to pieces?
	int board[120];
	int ply;

	// side to move
	bool stm;
	int enPas;
	int castlePermission;

	int kingSq[2];

	// Hash key, unique representation of board
	U64 posKey;

	U64 pinned;

	MOVE_IRREV history[500];
} BOARD_STATE;

typedef struct {
	char fen[99];
	int depth;
	long nodes;
} TEST_POSITION;

// global mode
enum { NORMAL_MODE, FEN_MODE, TEST_MODE, PERFT_MODE, SEARCH_MODE };
// printBoard opts
enum { OPT_64_BOARD, OPT_BOARD_STATE, OPT_120_BOARD, OPT_PINNED };

/* MACROS */
/* GLOBALS */
extern const char pieceChar[];
extern const char castleChar[];
extern const int isPawn[];
extern const int isKing[];
extern const int OFFBOARD;
extern const int numDirections[];
extern const int translation[][8];
extern int counter;
extern int mode;
extern TEST_POSITION tps[];
/* FUNCTIONS */
// data.c
extern size_t tpsSize();
// fen.c
extern int parseFEN(BOARD_STATE *bs, char *fen);
extern int genFEN(BOARD_STATE *bs, char *fen);
// utils.c
extern int sq64to120(int sq64);
extern int sq120to64(int sq120);
extern int boardIndexFlip(int i);
extern int frToSq64(int file, int rank);
extern void getAlgebraic(char *sqfr, int sq120);
extern void getCastlePermissions(char *sqStrPtr, int cperm);
extern int getType(int piece);
extern bool getColor(int piece);
extern bool on2ndRank(int sq, bool color);
extern bool on7thRank(int sq, bool color);
extern bool enPasCorrectColor(int enPas, bool stm);
// main.c
extern void resetBoard(BOARD_STATE *bs);
extern int genLegalMoves(BOARD_STATE *bs, MOVE moves[]);
extern int newBoardCheck(BOARD_STATE *bs, int sq, int cs);
extern void printMove(int m, MOVE move);
extern void printLegalMoves(BOARD_STATE *bs);
// init.c
extern void initRand();
extern BOARD_STATE* initGame();
extern void initLegalMoves();
// moves.c
extern int getFrom(MOVE m);
extern int getTo(MOVE m);
extern int getMType(MOVE m);
extern MOVE buildMove(int from, int to, int movetype);
extern void updatePins(BOARD_STATE *bs, bool side);
extern void makeMove(BOARD_STATE *bs, MOVE move);
extern void undoMove(BOARD_STATE *bs);
// eval.c
extern int randInt(int lb, int ub);
extern int negaMax(BOARD_STATE *bs, int depth);
extern float eval(BOARD_STATE *bs);
extern float treeSearch(BOARD_STATE *bs, int depth);
extern U64 perft(BOARD_STATE *bs, int depth);
extern U64 perft2(BOARD_STATE *bs, int depth);
// test.c
extern bool testUtilFunctions();
extern bool testMoveGenPositions();
