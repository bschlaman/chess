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
#define MOVE_FLAG_MASK 0x0F
#define MOVE_FLAG_NUM_BITS 4
#define MOVE_TO_NUM_BITS 14
#define MOVE_FROM_OFFSET 18
#define MAX_PLY 1000

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
bool square_attacked_by_side(int *b, int sq, int ignore_sq, int ep_captured_sq, SIDE side);
void test_move_gen();
void test_board_rep();

enum {
	// elemental attack types
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

// TODO: create a better system for this
typedef enum {
	M_PROMOTION = 8,
	M_CAPTURE   = 4,
	M_BIT2      = 2,
	M_BIT1      = 1,

	M_QUIET        = 0,
	M_CSTL_KING    = M_BIT1,
	M_CSTL_QUEEN   = M_BIT1 | M_BIT2,
	M_CAPT         = M_CAPTURE,
	M_CAPT_EP      = M_CAPTURE | M_BIT1,
	M_PROMO_N      = M_PROMOTION,
	M_PROMO_B      = M_PROMOTION | M_BIT1,
	M_PROMO_R      = M_PROMOTION | M_BIT2,
	M_PROMO_Q      = M_PROMOTION | M_BIT1 | M_BIT2,
	M_CAPT_PROMO_N = M_PROMOTION | M_CAPTURE,
	M_CAPT_PROMO_B = M_PROMOTION | M_CAPTURE | M_BIT1,
	M_CAPT_PROMO_R = M_PROMOTION | M_CAPTURE | M_BIT2,
	M_CAPT_PROMO_Q = M_PROMOTION | M_CAPTURE | M_BIT1 | M_BIT2,
} MOVE_TYPE;

// print_board opts
enum { OPT_64_BOARD = 1, OPT_BOARD_STATE = 2, OPT_VBOARD = 4 };

enum {
	CHECK_SLIDER_DISTANT = 1,
	CHECK_KING_ROSE      = 2,
	CHECK_KNIGHT         = 4,
};

// 00000000000000 00000000000000 0000
// from           to             moveType
typedef unsigned int MOVE;
MOVE move_stack[MAX_PLY];
int move_stack_idx = 0;

// NOTE: official chess rules has a different definition of "irreversible"
// "irreversible" moves reset the halfmove clock (50 move rule)
// captures and pawn moves only; castling does not count
typedef struct {
	int ep_sq;
	int castle_perms;
	int captured_piece;
} MOVE_IRREV;
MOVE_IRREV move_stack_irrev[MAX_PLY];

// random primes; making sure my logic doesn't rely on these enums being 0
#define CAPTURED 17
#define PINNED   233
int
	sliders[2][BOARD_SIZE],
	contact[2][BOARD_SIZE],
	pawns[2][BOARD_SIZE];
int
	sliders_idx[2],
	// TODO: store king squares as a BOARD_STATE var
	// and make this an array of just knights
	contact_idx[2] = {1, 1},
	pawns_idx[2];

typedef unsigned int PIECE;
// 0000000000000000000000 00000000 00
//         piece list idx     type color
#define PLI_OFFSET 10
#define tEMPTY  0x00
#define tWHITE  0x01
#define tBLACK  0x02
#define tGUARD  0x03
#define tPAWN   0x04
#define tKNIGHT 0x08
#define tBISHOP 0x10
#define tROOK   0x20
#define tQUEEN  0x40
#define tKING   0x80
#define NUM_NON_KING_TYPES 5
// TODO: extracting color from PIECE looks like
// piece & COLOR_MASK - 1 which is gross
#define COLOR_MASK 0x03
int pieces[2][1 + BOARD_SIZE * NUM_NON_KING_TYPES];

// used primarily for pintests
// const int max_distance = H8-A1;
int
	i_v[1 + 2 * MAX_VBOARD_DISTANCE],
	a_t[1 + 2 * MAX_VBOARD_DISTANCE];
// allow for negative indices
// TODO: use increment_vector consistently! from - to or to - from?
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
	bs -> ep_sq = OFFBOARD;
}

void init_pieces(BOARD_STATE *bs){
	memset(sliders_idx, 0, sizeof(sliders_idx));
	contact_idx[BLACK] = contact_idx[WHITE] = 1;
	memset(pawns_idx, 0, sizeof(pawns_idx));
	ASSERT(sliders_idx[BLACK] == 0);
	ASSERT(sliders_idx[WHITE] == 0);
	ASSERT(contact_idx[BLACK] == 1);
	ASSERT(contact_idx[WHITE] == 1);
	ASSERT(pawns_idx[BLACK] == 0);
	ASSERT(pawns_idx[WHITE] == 0);

	move_stack_idx = 0;

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
		int from = move_stack[i] >> MOVE_FROM_OFFSET;
		int to = (move_stack[i] >> MOVE_FLAG_NUM_BITS) & MOVE_TO_MASK;
		printf(CYN "MOVE " reset "%c: %s -> %s\n",
			pieceChar[bs -> board[from]],
			get_algebraic(from),
			get_algebraic(to),
			to
		);
	}
	printf(YEL "total moves:" reset " %d\n", move_stack_idx);
}

// TODO: move type not used yet
void create_move(int from, int to, MOVE_TYPE mtype){
	MOVE move = from << MOVE_FROM_OFFSET | to << MOVE_FLAG_NUM_BITS;
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
				check |= CHECK_SLIDER_DISTANT;
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
		if(attack_type[ksq - csq] & A_KNIGHT){
			check |= CHECK_KNIGHT;
			opp_checker_sq = csq;
			check_vector = increment_vector[ksq - csq];
			goto AFTER_CHECKS;
		}
	}
	// check for contact check by other
	for(int i = 0; i < numDirections[KING]; i++){
		csq = ksq + translation[KING][i];
		if(color_map[b[csq]] != !side) continue;
		if(attack_type[ksq - csq] & attack_map[b[csq]]){
			check |= CHECK_KING_ROSE;
			opp_checker_sq = csq;
			check_vector = increment_vector[ksq - csq];
			goto AFTER_CHECKS;
		}
	}
	AFTER_CHECKS:
	if(check){
		// if in check, ignore pin ray moves
		move_stack_idx = start_move_stack_idx;
		// double check
		if(check & CHECK_SLIDER_DISTANT & CHECK_KNIGHT) goto KING_MOVES;
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
			&& b[G1+rank_offset] == EMPTY \
			&& !square_attacked_by_side(b, F1+rank_offset, ksq, OFFBOARD, !side)){
			create_move(ksq, ksq + TRT + TRT, b[ksq]);
		}
		if(cperm & q_perm \
			&& b[B1+rank_offset] == EMPTY \
			&& b[C1+rank_offset] == EMPTY \
			&& b[D1+rank_offset] == EMPTY \
			&& !square_attacked_by_side(b, D1+rank_offset, ksq, OFFBOARD, !side)){
			create_move(ksq, ksq + TLF + TLF, b[ksq]);
		}
	}

	EP_MOVES:
	if(ep_sq == OFFBOARD) goto NORMAL_MOVES;
	piece = side == WHITE ? wP : bP;
	csq = ep_sq	- fwd + TLF;
	if(b[csq] == piece \
		&& !square_attacked_by_side(b, ksq, csq, ep_sq - fwd, !side))
		create_move(ep_sq - fwd + TLF, ep_sq, piece);
	csq = ep_sq	- fwd + TRT;
	if(b[csq] == piece \
		&& !square_attacked_by_side(b, ksq, csq, ep_sq - fwd, !side))
		create_move(ep_sq - fwd + TLF, ep_sq, piece);

	NORMAL_MOVES:
	if(check & (CHECK_KNIGHT | CHECK_KING_ROSE)){
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
		for(int i = 0; i < sliders_idx[side]; i++){
			csq = sliders[side][i];
			if(csq == CAPTURED) continue;
			if(attack_type[csq - opp_checker_sq] & attack_map[b[csq]]){
				int vector = increment_vector[csq - opp_checker_sq];
				while(b[csq+=vector] == EMPTY);
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
				int vector = translation[type][d];
				// move along ray
				while(b[csq+=vector] == EMPTY)
					create_move(slider_sq, csq, piece);
				// capture
				if(color_map[b[csq]] == !side)
					create_move(slider_sq, csq, piece);
			}
		}
		if(check & CHECK_SLIDER_DISTANT){
			// purge moves that dont block or capture a distant check
			// if move.to is not on the check ray, remove it
			// if it is on the check ray. make sure it is in between
			// checker and king (including opp_checker_sq itself (capture))
			for(int i = start_move_stack_idx; i < move_stack_idx; i++){
				int to = (move_stack[i] >> 4) & MOVE_TO_MASK;
				if(to == opp_checker_sq) continue;
				int vector = increment_vector[to - ksq];
				// need to make sure that the vector is the same as check_vector
				// since all vectors of non-slider accessible square parings are all 0
				// i.e. comparing increment_vector[opp_checker_sq - to]
				// and increment_vector[to - ksq] does not work until
				// we are sure that vector == check_vector
				if(vector != check_vector){
					move_stack[i--] = move_stack[--move_stack_idx];
				} else {
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
		if(square_attacked_by_side(b, csq, ksq, OFFBOARD, !side)) continue;
		create_move(ksq, csq, b[ksq]);
	}
	while(pin_idx > 0)
		b[pin_sqs[--pin_idx]] = pin_pieces[pin_idx];
}

// TODO: probably a better name than ignore_sq
bool square_attacked_by_side(int *b, int sq, int ignore_sq, int ep_captured_sq, SIDE side){
	// note: this doesn't take pinned pieces into account!
	// used for check detection
	int fwd = side == WHITE ? TUP : TDN;
	int csq;
	// pawns
	int piece = side == WHITE ? wP : bP;
	csq = sq - fwd + TLF;
	if(b[csq] == piece && csq != ep_captured_sq) return true;
	csq = sq - fwd + TRT;
	if(b[csq] == piece && csq != ep_captured_sq) return true;
	// knights
	for(int i = 1; i < contact_idx[side]; i++){
		csq = contact[side][i];
		if(csq == CAPTURED) continue;
		if(attack_type[csq - sq] & A_KNIGHT) return true;
	}
	// sliders
	for(int i = 0; i < sliders_idx[side]; i++){
		csq = sliders[side][i];
		if(csq == CAPTURED) continue;
		if(attack_type[csq - sq] & attack_map[b[csq]]){
			int vector = increment_vector[csq - sq];
			int sq_offset = sq;
			while(b[sq_offset+=vector] == EMPTY \
				|| sq_offset == ignore_sq || sq_offset == ep_captured_sq);
			if(sq_offset == csq) return true;
		}
	}
	return false;
}

void make_move(BOARD_STATE *bs, MOVE move){
	int move_flag = move & MOVE_FLAG_MASK;
	int to = (move >> MOVE_FLAG_NUM_BITS) & MOVE_TO_MASK;
	int from = move >> MOVE_FROM_OFFSET;

	int *b = bs -> board;
	SIDE side = bs -> stm;
	int fwd = side == WHITE ? TUP : TDN;
	int captured_piece = move_flag == M_CAPT_EP ? b[to - fwd] : b[to];
	// another problem here (similar to line 321).
	// how do I update the piece lists
	// if pieces of the same type are not differentiated?
	int piece = b[from];

	// TODO: is there a better way?  also, can I do move_stack this way?
	// 1) save irreversible aspects of the move
	MOVE_IRREV *mi = &move_stack_irrev[bs -> ply];
	mi -> ep_sq = bs -> ep_sq;
	mi -> castle_perms = bs -> castlePermission;
	mi -> captured_piece = captured_piece;

	// 2) increment ply
	bs -> ply++;

	// 3) update the board and piece lists
	b[to] = b[from];
	b[from] = EMPTY;
	// special cases
	switch(move_flag){
		case M_QUIET: break;
		case M_CSTL_KING: break;
	}
}

void undo_move(){
	puts("");
}

void main(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	// char testFEN[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
	char testFEN[] = "8/8/8/3k4/2pP4/8/B7/4K3 b - d3";
	// TODO: i dont like having to parseFEN between these init steps
	init_board(bs);
	parseFEN(bs, testFEN);
	init_pieces(bs);
	init_vectors();

	print_board(bs, OPT_VBOARD|OPT_64_BOARD|OPT_BOARD_STATE);
	gen_legal_moves(bs);
	print_move_stack(bs);
	test_move_gen();
	test_board_rep();
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
	if(opt & OPT_BOARD_STATE){
		printf(BLU "side to move: " reset "%s\n", bs -> stm == WHITE ? "white" : "black");
		printf(BLU "ply: " reset "%d\n", bs -> ply);
		printf(BLU "en passant sq: " reset "%s\n", get_algebraic(bs -> ep_sq));
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

void test_board_rep(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	// kiwipete position
	char testFEN[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";

	init_board(bs);
	parseFEN(bs, testFEN);
	init_pieces(bs);
	init_vectors();

	int *b = bs -> board;

	ASSERT(b[E1] & tKING & tWHITE);
	ASSERT(b[E8] & tKING & tBLACK);
	ASSERT(b[G7] & tBISHOP & tBLACK);
	ASSERT(b[A7] & tPAWN & tBLACK);
	ASSERT(b[D4] & tEMPTY);
	ASSERT(b[0] & tGUARD);
	ASSERT(b[103] & tGUARD);
	ASSERT(b[89] & tGUARD);

	PIECE piece = b[B6];
	int piece_list_index = piece >> PLI_OFFSET;
	SIDE side = piece & COLOR_MASK - 1;
	ASSERT(pieces[side][piece_list_index] == B6);
}

typedef struct {
	char fen[99];
	int depth;
	long nodes;
} TEST_POSITION;

TEST_POSITION tps[] = {
	{
		.fen = "R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - -",
		.depth = 1,
		.nodes = 218,
	},
	{
		// kiwipete
		.fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
		.depth = 1,
		.nodes = 48,
	},
	{
		.fen = "8/8/8/2k5/2pP4/8/B7/4K3 b - d3",
		.depth = 1,
		.nodes = 8,
	},
	{
		.fen = "3r4/8/8/3Pp3/3K3k/8/8/8 w - d6",
		.depth = 1,
		.nodes = 7,
	},
	{
		.fen = "8/8/8/3k4/2pP4/8/B7/4K3 b - d3",
		.depth = 1,
		.nodes = 5,
	},
	{
		.fen = "8/8/8/4k3/2pP4/8/1B6/4K3 b - d3",
		.depth = 1,
		.nodes = 7,
	},
	{
		.fen = "8/8/8/8/R1pP2k1/8/8/4K3 b - d3",
		.depth = 1,
		.nodes = 9,
	},
	{
		.fen = "8/1K6/8/8/1kpP4/8/8/4Q3 w - d3",
		.depth = 1,
		.nodes = 4,
	},
	{
		.fen = "6k1/8/8/2K5/5pP1/8/8/6Q1 w - g3",
		.depth = 1,
		.nodes = 7,
	},
};

size_t tpsSize(){
	return sizeof(tps) / sizeof(TEST_POSITION);
}

void test_move_gen(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	for(int i = 0; i < tpsSize(); i++){
		init_board(bs);
		parseFEN(bs, tps[i].fen);
		init_pieces(bs);
		init_vectors();

		gen_legal_moves(bs);
		int num_legal_moves = move_stack_idx;
		if(tps[i].nodes == 5){
			print_board(bs, OPT_BOARD_STATE|OPT_VBOARD|OPT_64_BOARD);
			print_move_stack(bs);
		}

		printf("pos: %2d, depth: %3d ", i, tps[i].depth, i);
		printf("wanted: %8d, got: %8d ", tps[i].nodes, num_legal_moves);
		printf("%s\n" reset, num_legal_moves == tps[i].nodes ? GRN "✓" : RED "X");
		ASSERT(tps[i].nodes == num_legal_moves);
	}
}
