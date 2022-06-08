#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "data_improved.c"
#include "colors.h"

#define ASSERT(n) \
if(!(n)){ \
    printf(RED "====== assert error\n" YEL "%s\n" reset, #n); \
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
#define MAX_VBOARD_DISTANCE 77
#define BLACK_RANK_OFFSET 70
#define MOVE_TO_MASK 0x3FFF

// TODO: test if movegen works if these two are flipped
typedef enum { WHITE, BLACK, NEITHER } SIDE;
enum {
	OFFBOARD, EMPTY,
	wP, wN, wB, wR, wQ, wK,
	bP, bN, bB, bR, bQ, bK,
};
typedef enum { KNIGHT, BISHOP, ROOK, QUEEN, KING, PAWN, NONETYPE } PIECE_TYPE;
// 0 0 0 0
enum { WKCA = 8, WQCA = 4, BKCA = 2, BQCA = 1 };

typedef struct {
	int board[VBOARD_SIZE];
	int ply;
	SIDE stm;
	int ep_sq;
	int castlePermission;
} BOARD_STATE;

// util functions
void parseFEN(BOARD_STATE *bs, char *fen);
int sq64to120(int sq64);
int sq120to64(int sq120);
int frToSq64(int file, int rank);
void print_board(BOARD_STATE *bs, int opt);
char *get_algebraic(int sq120);
SIDE getColor(int piece);
bool on2ndRank(int sq, bool color);

// elemental attack types
enum {
	A_CONTACT_FORW      = 0x01,
	A_CONTACT_BCKW      = 0x02,
	A_CONTACT_FORW_DIAG = 0x04,
	A_CONTACT_BCKW_DIAG = 0x08,
	A_CONTACT_SIDE      = 0x10,
	A_KNIGHT            = 0x20,
	A_DISTANT_ORTH      = 0x40,
	A_DISTANT_DIAG      = 0x80,

	A_WPAWN   = A_CONTACT_FORW_DIAG,
	A_BPAWN   = A_CONTACT_BCKW_DIAG,
	A_FERZ    = A_CONTACT_FORW_DIAG | A_CONTACT_BCKW_DIAG,
	A_WAZIR   = A_CONTACT_SIDE | A_CONTACT_FORW | A_CONTACT_BCKW,
	A_KING    = A_FERZ | A_WAZIR,
	A_BISHOP  = A_DISTANT_DIAG | A_FERZ,
	A_ROOK    = A_DISTANT_ORTH | A_WAZIR,
	A_QUEEN   = A_BISHOP | A_ROOK,
	A_CONTACT = A_KNIGHT | A_KING,
	A_DISTANT = A_DISTANT_ORTH | A_DISTANT_DIAG,
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
const char pieceChar[] = "-.PNBRQKpnbrqk";
const char castleChar[] = "KQkq";
const int numDirections[] = {8, 4, 4, 8, 8};
const int translation[][8] = {
	{-21, -12, 8, 19, 21, 12, -8, -19}, // knights 0
	{TNE, TSE, TSW, TNW}, // bishops 1
	{TUP, TRT, TDN, TLF}, // rooks 2
	{TUP, TRT, TDN, TLF, TNE, TSE, TSW, TNW}, // queens 3
	{TUP, TRT, TDN, TLF, TNE, TSE, TSW, TNW},  // kings 4
};
const int attack_map[] = {
	0, 0,
	A_WPAWN, A_KNIGHT, A_BISHOP, A_ROOK, A_QUEEN, A_KING,
	A_BPAWN, A_KNIGHT, A_BISHOP, A_ROOK, A_QUEEN, A_KING,
};
// TODO: check if I ever actually use this; delete if not
const int color_map[] = {
	NEITHER, NEITHER,
	WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
	BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
};
const PIECE_TYPE type_map[] = {
	NONETYPE, NONETYPE,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
};

// print_board opts
enum { OPT_64_BOARD = 1, OPT_BOARD_STATE = 2, OPT_VBOARD = 4, OPT_PINNED = 8 };

enum {
	CHECK_RAY     = 1,
	CHECK_KING_ROSE = 2,
	CHECK_KNIGHT  = 4,
};

// 00000000000000 00000000000000 0000
// from           to             moveType
typedef unsigned int MOVE;
MOVE move_stack[1024];
int move_stack_idx = 0;

// random primes; making sure my logic doesn't rely on these enums being 0
#define CAPTURED 17
#define PINNED   23
int
	sliders[2][BOARD_SIZE],
	contact[2][BOARD_SIZE],
	pawns[2][BOARD_SIZE];
int
	sliders_idx[2],
	contact_idx[2] = {1, 1},
	pawns_idx[2];

// used primarily for pintests
// const int max_distance = H8-A1;
int
	i_v[1 + 2 * MAX_VBOARD_DISTANCE],
	a_t[1 + 2 * MAX_VBOARD_DISTANCE];
// allow for negative indices
#define increment_vector (i_v + MAX_VBOARD_DISTANCE)
#define attack_type      (a_t + MAX_VBOARD_DISTANCE)

void init_vectors(){
	attack_type[TUP] = A_CONTACT_FORW;
	attack_type[TDN] = A_CONTACT_BCKW;
	attack_type[TRT] = A_CONTACT_SIDE;
	attack_type[TLF] = A_CONTACT_SIDE;
	attack_type[TNE] = A_CONTACT_FORW_DIAG;
	attack_type[TNW] = A_CONTACT_FORW_DIAG;
	attack_type[TSE] = A_CONTACT_BCKW_DIAG;
	attack_type[TSW] = A_CONTACT_BCKW_DIAG;
	for(int i = 0; i < 8; i++){
		attack_type[translation[KNIGHT][i]] = A_KNIGHT;
		increment_vector[translation[KNIGHT][i]] = translation[KNIGHT][i];
		int sq_offset = 0;
		int increment = translation[QUEEN][i];
		int distant_attack_type = i < 4 ? A_DISTANT_ORTH : A_DISTANT_DIAG;
		for(int j = 0; j < 7; j++){
			increment_vector[sq_offset+=increment] = increment;
			if(j) attack_type[sq_offset] = distant_attack_type;
		}
	}
}

void init_board(BOARD_STATE *bs){
	for(int i = 0 ; i < VBOARD_SIZE ; i++)
		bs -> board[i] = OFFBOARD;
	for(int i = 0 ; i < BOARD_SIZE ; i++)
		bs -> board[sq64to120(i)] = EMPTY;
	bs -> ply = 1;
}

void init_pieces(BOARD_STATE *bs){
	ASSERT(sliders_idx[BLACK] == 0);
	ASSERT(sliders_idx[WHITE] == 0);
	ASSERT(contact_idx[BLACK] == 1);
	ASSERT(contact_idx[WHITE] == 1);
	ASSERT(pawns_idx[BLACK] == 0);
	ASSERT(pawns_idx[WHITE] == 0);
	for(int i = 0; i < BOARD_SIZE; i++){
		int sq120i = sq64to120(i);
		// TODO: I could add breaks here but I don't need to optomize this function
		switch(bs -> board[sq120i]){
			case bP:
				pawns[BLACK][pawns_idx[BLACK]++] = sq120i; break;
			case wP:
				pawns[WHITE][pawns_idx[WHITE]++] = sq120i; break;
			case bN:
				contact[BLACK][contact_idx[BLACK]++] = sq120i; break;
			case wN:
				contact[WHITE][contact_idx[WHITE]++] = sq120i; break;
			case bB:
			case bR:
			case bQ:
				sliders[BLACK][sliders_idx[BLACK]++] = sq120i; break;
			case wB:
			case wR:
			case wQ:
				sliders[WHITE][sliders_idx[WHITE]++] = sq120i; break;
			case bK:
				contact[BLACK][0] = sq120i; break;
			case wK:
				contact[WHITE][0] = sq120i; break;
		}
	}
	ASSERT(bs -> board[contact[BLACK][0]] == bK);
	ASSERT(bs -> board[contact[WHITE][0]] == wK);
}

void print_move_stack(BOARD_STATE *bs){
	for(int i = 0; i < move_stack_idx; i++){
		int from = move_stack[i] >> 18;
		int to = (move_stack[i] >> 4) & MOVE_TO_MASK;
		printf(BLU "MOVE " reset "%c: %s -> %s (%d)\n",
			pieceChar[bs -> board[from]],
			get_algebraic(from),
			get_algebraic(to),
			to
		);
	}
	printf(YEL "total moves:" reset " %d\n", move_stack_idx);
}

void create_move(int from, int to, int piece){
	MOVE move = from << 18 | to << 4;
	move_stack[move_stack_idx++] = move;
}

void gen_legal_moves(BOARD_STATE *bs){
	int *b = bs -> board;
	SIDE side = bs -> stm;
	int ep_sq = bs -> ep_sq;
	int ksq = contact[side][0];
	int start_move_stack_idx = move_stack_idx;
	int check = 0, check_vector, opp_checker_sq;

	int fwd = side == WHITE ? TUP : TDN;
	int csq;
	int piece;

	// TODO: im trying out pointers here over indexes
	// decide if i want to switch later
	int pin_pieces[8], pin_sqs[8];
	int pin_idx = 0;

	// pintest and distant check test
	for(int i = 0; i < sliders_idx[!side]; i++){
		int opp_slider_sq = sliders[!side][i];
		if(opp_slider_sq == CAPTURED) continue;
		// check if this piece can attack our king square
		int vector = increment_vector[opp_slider_sq - ksq];
		if(attack_type[opp_slider_sq - ksq] & attack_map[b[opp_slider_sq]] & A_DISTANT){
			// opponent slider is aimed at king
			printf("slider aimed at king: %s\n", get_algebraic(opp_slider_sq));
			// TODO: use a different name than sq_offset
			int sq_offset = ksq;
			while((piece = b[sq_offset+=vector]) == EMPTY);
			if(sq_offset == opp_slider_sq){
				check |= CHECK_RAY;
				check_vector = vector;
				opp_checker_sq = opp_slider_sq;
			} else if(color_map[piece] == side){
				// note: we dont know if its actually pinned yet
				int pin_sq = sq_offset;
				while(b[sq_offset+=vector] == EMPTY);
				if(sq_offset == opp_slider_sq){
					// pinned at pin_sq
					// avoid duplicate arrays with a pin struct?
					pin_pieces[pin_idx] = piece;
					pin_sqs[pin_idx++] = pin_sq;
					// running into a problem here.  I need to know
					// which of our pieces to mark as pinned
					// can i just mark the board with a special marker?
					// TODO 1: this is not ideal - I don't want to touch the board array
					b[pin_sq] = PINNED;
					if(piece == wP || piece == bP){
						csq = pin_sq + fwd;
						if(vector == TUP || vector == TDN){
							if(b[csq] == EMPTY){
								// push pawn 1 sq
								create_move(pin_sq, csq, piece);
								// push pawn 2 sq
								if(on2ndRank(pin_sq, side) && b[csq+=fwd] == EMPTY)
									create_move(pin_sq, csq, piece);
							}
						} else {
							// diagonal pawn pin
							if(csq + TLF == opp_slider_sq)
								create_move(pin_sq, csq + TLF, piece);
							if(csq + TRT == opp_slider_sq)
								create_move(pin_sq, csq + TRT, piece);
						}
					} else if(attack_map[piece] & attack_map[b[opp_slider_sq]]){
						// slider moves along pin vector
						csq = pin_sq;
						while((csq+=vector) != opp_slider_sq)
							create_move(pin_sq, csq, piece);
						csq = pin_sq;
						while((csq-=vector) != ksq)
							create_move(pin_sq, csq, piece);
					}
				}
			}
		}
	} // done with pintest & distant check test
	// check for contact check by knight
	// TODO: this could be expensive - can we infer this from previous move?
	for(int i = 1; i < contact_idx[!side]; i++){
		csq = contact[!side][i];
		if(csq == CAPTURED) continue;
		if(attack_type[csq - ksq] & A_KNIGHT){
			check |= CHECK_KNIGHT;
			opp_checker_sq = csq;
			check_vector = increment_vector[csq - ksq];
			goto AFTER_CHECKS;
		}
	}
	// check for contact check by other
	for(int i = 0; i < numDirections[KING]; i++){
		csq = ksq + translation[KING][i];
		if(csq == EMPTY || getColor(b[csq]) == side) continue;
		if(attack_type[csq - ksq] & attack_map[b[csq]]){
			check |= CHECK_KING_ROSE;
			opp_checker_sq = csq;
			check_vector = increment_vector[csq - ksq];
			goto AFTER_CHECKS;
		}
	}
	AFTER_CHECKS:
	if(check){
		// if in check, ignore pin ray moves
		move_stack_idx = start_move_stack_idx;
		// double check
		if(check & CHECK_RAY & CHECK_KNIGHT) goto KING_MOVES;
		// checker is a pawn capturable en passant
		if(opp_checker_sq == ep_sq - fwd) goto EP_MOVES;
	} else {
		// castling
		// TODO: castling through check
		int cperm = bs -> castlePermission;
		int k_perm = side == WHITE ? WKCA : BKCA;
		int q_perm = side == WHITE ? WQCA : BQCA;
		int rank_offset = side == WHITE ? 0 : BLACK_RANK_OFFSET;
		if(cperm & k_perm \
			&& b[F1+rank_offset] == EMPTY \
			&& b[G1+rank_offset] == EMPTY){
			create_move(ksq, ksq + TRT + TRT, b[ksq]);
		}
		if(cperm & q_perm \
			&& b[B1+rank_offset] == EMPTY \
			&& b[C1+rank_offset] == EMPTY \
			&& b[D1+rank_offset] == EMPTY){
			create_move(ksq, ksq + TLF + TLF, b[ksq]);
		}
	}
	EP_MOVES:
	piece = side == WHITE ? wP : bP;
	if(b[ep_sq - fwd + TLF] == piece)
		create_move(ep_sq - fwd + TLF, ep_sq, piece);
	if(b[ep_sq - fwd + TRT] == piece)
		create_move(ep_sq - fwd + TRT, ep_sq, piece);
	NORMAL_MOVES:
	if(check & CHECK_KING_ROSE){
		// can only move the king or capture the checker
		// pawn capture
		piece = side == WHITE ? wP : bP;
		if(b[opp_checker_sq - fwd + TLF] == piece)
			create_move(opp_checker_sq - fwd + TLF, opp_checker_sq, piece);
		if(b[opp_checker_sq - fwd + TRT] == piece)
			create_move(opp_checker_sq - fwd + TRT, opp_checker_sq, piece);
		// knight capture; look through side's knights
		for(int i = 1; i < contact_idx[side]; i++){
			csq = contact[side][i];
			if(csq == CAPTURED) continue;
			if(attack_type[csq - opp_checker_sq] & A_KNIGHT)
				create_move(csq, opp_checker_sq, b[csq]);
		}
		// TODO: this is a bit sloppy (redeclaring vector, accessing slider a lot)
		for(int i = 1; i < sliders_idx[side]; i++){
			csq = sliders[side][i];
			if(csq == CAPTURED) continue;
			if(attack_type[csq - opp_checker_sq] & attack_map[b[csq]]){
				int vector = increment_vector[csq - opp_checker_sq];
				while((csq+=vector) == EMPTY);
				if(csq == opp_checker_sq)
					create_move(sliders[side][i], opp_checker_sq, b[sliders[side][i]]);
			}
		}
		// slider capture; look through side's slider pieces
		// move king: after below else clause
	} else {
		// non-check moves
		// pawns
		// TODO: promotions
		for(int i = 0; i < pawns_idx[side]; i++){
			int pawn_sq = pawns[side][i];
			// TODO 1: do I check for pins here? i.e. b[pawn_sq] == PINNED
			if(pawn_sq == CAPTURED || b[pawn_sq] == PINNED) continue;
			// captures
			csq = pawn_sq + fwd + TLF;
			if(b[csq] != OFFBOARD && color_map[b[csq]] == !side)
				create_move(pawn_sq, csq, b[pawn_sq]);
			csq = pawn_sq + fwd + TRT;
			if(b[csq] != OFFBOARD && color_map[b[csq]] == !side)
				create_move(pawn_sq, csq, b[pawn_sq]);
			// pushes
			csq = pawn_sq + fwd;
			if(b[csq] == EMPTY){
				create_move(pawn_sq, csq, b[pawn_sq]);
				if(on2ndRank(pawn_sq, side) && b[csq+=fwd] == EMPTY)
					create_move(pawn_sq, csq, b[pawn_sq]);
			}
		}
		// knights
		for(int i = 1; i < contact_idx[side]; i++){
			int knight_sq = contact[side][i];
			if(knight_sq == CAPTURED) continue;
			for(int d = 0; d < numDirections[KNIGHT]; d++){
				csq = knight_sq + translation[KNIGHT][d];
				// TODO 1: now we have to check for pin here which seems clunky
				if(b[csq] == OFFBOARD || color_map[b[csq]] == side || b[csq] == PINNED) continue;
				create_move(knight_sq, csq, b[knight_sq]);
			}
		}
		// sliders
		for(int i = 0; i < sliders_idx[side]; i++){
			int slider_sq = sliders[side][i];
			piece = b[slider_sq];
			if(slider_sq == CAPTURED) continue;
			PIECE_TYPE type = type_map[piece];
			for(int d = 0; d < numDirections[type]; d++){
				csq = slider_sq;
				// TODO: translation array accessed many times... vector = translation...?
				while(b[csq+=translation[type][d]] == EMPTY){
					create_move(slider_sq, csq, piece);
				}
			}
		}
		if(check & CHECK_RAY){
			// remove moves that dont block or capture a distant check
			// if move.to is not on the check ray, remove it
			// if it is on the check ray. make sure it is in between
			// checker and king (including opp_checker_sq itself (capture))
			for(int i = start_move_stack_idx; i < move_stack_idx; i++){
				int to = (move_stack[i] >> 4) & MOVE_TO_MASK;
				int vector = increment_vector[to - ksq];
				// need to make sure that the vector is the same as check_vector
				// since all vectors of non-slider accessible square parings are all 0
				// i.e. comparing increment_vector[opp_checker_sq - to]
				// and increment_vector[to - ksq] does not work until
				// we are sure that vector == check_vector
				if(vector != check_vector){
					move_stack[i--] = move_stack[--move_stack_idx];
				} else {
					if(to == opp_checker_sq) continue; // capture
					if(increment_vector[opp_checker_sq - to] == vector) continue; // block
					move_stack[i--] = move_stack[--move_stack_idx];
				}
			}
		}
	}
	KING_MOVES:
	for(int i = 0; i < numDirections[KING]; i++){
		csq = ksq + translation[KING][i];
		if(b[csq] == OFFBOARD || color_map[b[csq]] == side || b[csq] == PINNED) continue;
		create_move(ksq, csq, b[ksq]);
	}
	while(pin_idx > 0){
		b[pin_sqs[pin_idx]] = pin_pieces[pin_idx];
		pin_idx--;
	}
}

void main(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	char testFEN1[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
	char testFEN2[] = "rnbqk1nr/pppppppp/8/b7/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	char testFEN[] = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";
	// TODO: i dont like having to parseFEN between these init steps
	init_board(bs);
	parseFEN(bs, testFEN);
	init_pieces(bs);
	init_vectors();

	print_board(bs, OPT_VBOARD|OPT_64_BOARD);
	gen_legal_moves(bs);
	print_move_stack(bs);
}

// UTILS BELOW main
/*
utils like fen parser for temporary use while
i make improvements to move generation
*/

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
	if(opt & OPT_64_BOARD){
		printf(YEL " ---- Game Board ---- \n" reset);
		for(int rank = 8; rank >= 1; rank--){
			printf(RED "%d " reset, rank);
			for(int file = 1; file <= 8; file++){
				int piece = bs -> board[sq64to120(frToSq64(file, rank))];
				printf("%2c", pieceChar[piece]);
			}
			puts("");
		}
		// print the files
		printf("\n  ");
		for(int file = 0; file < 8; file++){
			printf(RED "%2c" reset, file + 'a');
		}
		puts("");
	}
}

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

char *get_algebraic(int sq120){
	int sq64 = sq120to64(sq120);
	char *sqAN = malloc(3 * sizeof(char));
	sqAN[0] = (sq64 % 8) + 'a';
	sqAN[1] = (sq64 - sq64 % 8) / 8 + '1';
	sqAN[2] = '\0';
	if(sq120 == OFFBOARD){ char *p = sqAN; *p++ = '-'; *p++ = '\0'; }
	return sqAN;
}

// copy algebraic notation of sq120 into sqStrPtr
void getAlgebraic(char *sqStrPtr, int sq120){
	int sq64 = sq120to64(sq120);
	char sqAN[] = {(sq64 % 8) + 'a', (sq64 - sq64 % 8) / 8 + '1', '\0'};
	if(sq120 == OFFBOARD){ char *p = sqAN; *p++ = '-'; *p++ = '\0'; }
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
SIDE getColor(int piece){
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

		// TODO: stick to one ordering of black and white
		if(piece == wK)
			contact[WHITE][0] = sq64to120(frToSq64(file, rank));
		if(piece == bK)
			contact[BLACK][0] = sq64to120(frToSq64(file, rank));

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
		bs -> ep_sq = sq64to120(frToSq64(file, rank));
	}
}
