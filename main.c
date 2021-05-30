#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>	
#include "defs.h"
#include "colors.h"


void saveMove(MOVE moves[], int i, MOVE move);
void printBoard(BOARD_STATE *bs, int options);
int pieceMoves(BOARD_STATE *bs, int piece, int sq, MOVE moves[], int offset);
int pieceCheckMoves(BOARD_STATE *bs, int piece, int sq, MOVE moves[], int offset);
int genRandomMove(BOARD_STATE *bs);
void printPieceMoves(BOARD_STATE *bs);
int checkDir(int *board, int kingsq, bool color);


void saveMove(MOVE moves[], int i, MOVE move){
	moves[i] = move;
}

int genLegalMoves(BOARD_STATE *bs, MOVE moves[]){
	int i, total = 0;
	int sq, piece, stm;
	stm = bs -> stm;
	int kingsq = bs -> kingSq[stm];
	int dir = checkDir(bs -> board, kingsq, stm);

	if(dir == -1){
		// if king not in check
		for(i = 0 ; i < 64 ; i++){
			sq = sq64to120(i);
			piece = bs -> board[sq];
			if(piece != EMPTY && getColor(piece) == stm){
				total += pieceMoves(bs, piece, sq, moves, total);
			}
		}
	} else {
		// in check from 1 piece
		// option 1: move king
		// option 2: capture the piece
		// option 3: block piece
		for(i = 0 ; i < 64 ; i++){
			sq = sq64to120(i);
			piece = bs -> board[sq];
			if(piece != EMPTY && getColor(piece) == stm){
				// move king
				if(sq == kingsq) total += pieceMoves(bs, bs -> board[kingsq], kingsq, moves, total);
				// capture piece or block
				else { total += pieceCheckMoves(bs, piece, sq, moves, total); }
			}
		}
	}
	return total;
}

int genRandomMove(BOARD_STATE *bs){
	MOVE moves[255];
	int total = genLegalMoves(bs, moves);
	if(total > 0){
		int r = rand() % total;
		return r;
	} else {
		// TODO: stalemate
		return -1;
	}
}

void printMove(int m, MOVE move){
	char sqfr[3];
	int from = getFrom(move);
	int to = getTo(move);
	int moveType = getMType(move);

	printf(YEL "      move %d: " reset, m);
	getAlgebraic(sqfr, from);
	printf("from: %s ", sqfr);
	getAlgebraic(sqfr, to);
	printf("to: %s ", sqfr);
	printf("moveType: %d\n", moveType);
}

// BOARD_STATE arg needed for eval
void printLegalMoves(BOARD_STATE *bs){
	int m, from, to, moveType;
	char sqfr[3];
	MOVE moves[255];
	int total = genLegalMoves(bs, moves);
	for(m = 0 ; m < total ; m++){
		from = getFrom(moves[m]);
		to = getTo(moves[m]);
		moveType = getMType(moves[m]);

		printf(YEL "      move %d: " reset, m);
		getAlgebraic(sqfr, from);
		printf("from: %s ", sqfr);
		getAlgebraic(sqfr, to);
		printf("to: %s ", sqfr);
		printf("moveType: %d ", moveType);
		// evaluate position after move is made
		makeMove(bs, moves[m]);
		printf("eval for opponent after move: %d\n", eval(bs));
		undoMove(bs);
	}
	printf(BLU "total moves in pos: " reset "%d\n", total);
}

// TODO: this is broken currently
void printPieceMoves(BOARD_STATE *bs){
	int i, m, piece, sq, total = 0;
	bool stm = bs -> stm;
	int from, to, moveType;
	MOVE moves[255];

	for(i = 0 ; i < 64 ; i++){
		sq = sq64to120(i);
		piece = bs -> board[sq];
		if(piece != EMPTY && getColor(piece) == stm){
			printf(GRN " == PIECE: %c\n" reset, pieceChar[piece]);
			total += pieceMoves(bs, piece, sq, moves, total);
			printLegalMoves(bs);
		}
	}
	printf(BLU "total moves in pos: " reset "%d\n", total);
}

U64 updatePinned(){
	return 0ULL;
}
// returns the direction, 0 -> 15
// - 8 - O -
// 9 0 4 3 N
// - 5 k 7 -
// J 1 6 2 M
// - K - L -
int checkDir(int *board, int kingsq, bool color){
	int d, cpiece, cs;

	// knights
	for(d = 0 ; d < 8 ; d++){
		if((cpiece = board[kingsq + translation[0][d]]) != OFFBOARD \
			&& getType(cpiece) == KNIGHT \
			&& color != getColor(cpiece)){
			return d + 8; break;
		}
	}
	// pawns
	if     (color == BLACK && board[kingsq + TSW] == wP) return 1;
	else if(color == BLACK && board[kingsq + TSE] == wP) return 2;
	else if(color == WHITE && board[kingsq + TNE] == bP) return 3;
	else if(color == WHITE && board[kingsq + TNW] == bP) return 0;
	// ray
	for(d = 0 ; d < 8 ; d++){
		cs = kingsq;
		while((cpiece = board[cs += translation[KING][d]]) != OFFBOARD){
			if(cpiece != EMPTY){
				if(getColor(cpiece) != color){
					if(d < 4){
						if(getType(cpiece) == BISHOP || getType(cpiece) == QUEEN) return d;
					} else if(d >= 4 && d < 8){
						if(getType(cpiece) == ROOK || getType(cpiece) == QUEEN) return d;
					}
				}
				break;
			}
		}
	}
	// now not needed, since this is an invalid board state
	// kings
	for(d = 0 ; d < 8 ; d++){
		// TODO: don't need to check if this is OFFBOARD
		// but may run into a -1 index if using isKing
		if((cpiece = board[kingsq + translation[KING][d]]) != OFFBOARD){
			// TODO: Assert the color?
			if(isKing[cpiece]) return d;
		}
	}

	return -1;
}

// TODO: if it's none of the directions, it will always default to TLF and TRT
int getPinDir(int kingsq, int pinsq){
	int diff = pinsq - kingsq;
	if(diff > 0){
		if(diff % TNE == 0) return TNE;
		if(diff % TUP == 0) return TUP;
		if(diff % TNW == 0) return TNW;
		if(diff % TRT == 0) return TRT;
	} else {
		if(diff % TSW == 0) return TSW;
		if(diff % TDN == 0) return TDN;
		if(diff % TSE == 0) return TSE;
		if(diff % TLF == 0) return TLF;
	}
	return 0;
}

int epCheck(BOARD_STATE *bs, int sq, int es){
	// es = ep square
	int *board = bs -> board;
	bool stm = bs -> stm;
	int kingsq = bs -> kingSq[stm];
	// ASSERT(dir == getPinDir(kingsq, es));
	int captureSq = es - (1 - 2 * stm) * 10;
	int capturedPiece = board[captureSq];
	board[es] = board[sq];
	board[sq] = EMPTY;
	board[captureSq] = EMPTY;
	int check = checkDir(board, kingsq, stm) >= 0;
	board[sq] = board[es];
	board[es] = EMPTY;
	board[captureSq] = capturedPiece;
	return check;
}

int newBoardCheck(BOARD_STATE *bs, int sq, int cs){
	int *board = bs -> board;
	int check = false;
	int capturedPiece = board[cs];
	bool stm = bs -> stm;
	int kingsq = bs -> kingSq[stm];

	board[cs] = board[sq];
	board[sq] = EMPTY;
	if(kingsq == sq) kingsq = cs;
	check = checkDir(board, kingsq, stm) >= 0;
	board[sq] = board[cs];
	board[cs] = capturedPiece;
	return check;
}

int pieceCheckMoves(BOARD_STATE *bs, int piece, int sq, MOVE moves[], int offset){
	int i = 0, cs = sq, cs2, enPasCaptureFromSq = OFFBOARD, total = 0, d, dir, type, cpiece;
	int *board = bs -> board;

	if(!isPawn[piece]){
		type = getType(piece);

		// pinned pieces
		if(1ULL << sq120to64(sq) & bs -> pinned) return 0;

		// for each direction
		for(d = 0 ; d < numDirections[type] ; d++){
			cs = sq;
			// while sq not offboard
			while((cpiece = board[cs += translation[type][d]]) != OFFBOARD){
				//if(!newBoardCheck(bs, sq, cs)){
				if(cpiece == EMPTY){
					if(!newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				} else {
					if(getColor(piece) != getColor(cpiece)){
						if(!newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++; }
					}
					break;
				}
				// stop if piece is a knight or king
				if(type == KNIGHT) break;
			}
		}
	} else {
		// pawns =================

		// pinned pawns
		if(1ULL << sq120to64(sq) & bs -> pinned){
			dir = getPinDir(bs -> kingSq[bs -> stm], sq);
			// horizontal
			if(abs(dir) == 1) return 0;
			// vertical
			else if(abs(dir) == 10){
				// forward 1
				cs = sq + (1 - 2 * getColor(piece)) * 10;
				if(board[cs] == EMPTY && !newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				// forward 2
				if(on2ndRank(sq, getColor(piece))){
					cs = sq + (1 - 2 * getColor(piece)) * 20;
					cs2 = sq + (1 - 2 * getColor(piece)) * 10;
					if(board[cs] == EMPTY && board[cs2] == EMPTY && !newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				}
			}
			// diagonal
			else {
				cs = sq + dir;
				if(board[cs] != EMPTY && !newBoardCheck(bs, sq, cs)){
					if(on7thRank(sq, getColor(piece))){
						saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
					} else {
						saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
					}
				}
			}
			return total;
		}

		// forward 1
		// mapping {0,1} -> {-1,1} -> {-10,10}
		cs = sq + (1 - 2 * getColor(piece)) * 10;
		if(board[cs] == EMPTY && !newBoardCheck(bs, sq, cs)){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 8)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 9)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 10)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 11)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++;
			}
		}
		// forward 2
		if(on2ndRank(sq, getColor(piece))){
			cs = sq + (1 - 2 * getColor(piece)) * 20;
			cs2 = sq + (1 - 2 * getColor(piece)) * 10;
			if(board[cs] == EMPTY && board[cs2] == EMPTY && !newBoardCheck(bs, sq, cs)){
				saveMove(moves, total + offset, buildMove(sq, cs, 1)); total++;
			}
		}
		// captures
		cs = sq + (1 - 2 * getColor(piece)) * 10 + 1;
		if(board[cs] != EMPTY && board[cs] != OFFBOARD \
			&& getColor(piece) != getColor(board[cs]) \
			&& !newBoardCheck(bs, sq, cs)){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
			}
		}
		cs = sq + (1 - 2 * getColor(piece)) * 10 - 1;
		if(board[cs] != EMPTY && board[cs] != OFFBOARD \
			&& getColor(piece) != getColor(board[cs]) \
			&& !newBoardCheck(bs, sq, cs)){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
			}
		}
		// enPas
		cs = bs -> enPas;
		if(cs != OFFBOARD){
			// enPasCaptureFromSq is the same as what it would look like to captrue TO that sq
			enPasCaptureFromSq = cs + (1 - 2 * !getColor(piece)) * 10 + 1;
			if(sq == enPasCaptureFromSq \
				&& enPasCorrectColor(cs, getColor(piece)) \
				&& !epCheck(bs, sq, cs)){
				saveMove(moves, total + offset, buildMove(sq, cs, 5)); total++;
			}
			enPasCaptureFromSq = cs + (1 - 2 * !getColor(piece)) * 10 - 1;
			if(sq == enPasCaptureFromSq \
				&& enPasCorrectColor(cs, getColor(piece)) \
				&& !epCheck(bs, sq, cs)){
				saveMove(moves, total + offset, buildMove(sq, cs, 5)); total++;
			}
		}
	}
	return total;
}

int pieceMoves(BOARD_STATE *bs, int piece, int sq, MOVE moves[], int offset){
	// plan here is to use the 120 sq board
	// and move in particular "directions"
	// until the piece is OFFBOARD
	// cs = candidate square
	// cs2 = candidate square 2 (used for double pawn push)
	// note that newBoardCheck() is called every time there is a new cs
	// This means that I check it prematurely in some cases
	// but it's cleaner for now
	// moves[total+offset] = buildMove(from, to, 0);
	// TODO: should I get color from the BOARD_STATE?
	int i = 0, cs = sq, cs2, enPasCaptureFromSq = OFFBOARD, total = 0, d, dir, type, cpiece;
	int *board = bs -> board;
	ASSERT(sq >= 0 && sq <= 120 && board[sq] != OFFBOARD);

	// move generation
	if(isKing[piece]){
		// don't move king into check
		for(d = 0 ; d < numDirections[KING] ; d++){
			cs = sq;
			if((cpiece = board[cs += translation[KING][d]]) != OFFBOARD){
				if(cpiece == EMPTY){
					if(!newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				} else {
					if(getColor(piece) != getColor(cpiece)){
						if(!newBoardCheck(bs, sq, cs)){ saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++; }
					}
				}
			}
		}
	} else if(!isPawn[piece]){
		// type = proper index for translation[][]
		type = getType(piece);

		// pinned pieces
		if(1ULL << sq120to64(sq) & bs -> pinned){
			if(type == KNIGHT) return 0;
			else {
				dir = getPinDir(bs -> kingSq[bs -> stm], sq);
				if(type == ROOK && (abs(dir)==11||abs(dir)==9)) return 0;
				else if(type == BISHOP && (abs(dir)==10||abs(dir)==1)) return 0;
				else {
					cs = sq;
					while((cpiece = board[cs += dir]) == EMPTY){
						saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++;
					}
					saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
					cs = sq;
					while((cpiece = board[cs -= dir]) == EMPTY){
						saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++;
					}
				}
				return total;
			}
		}

		// for each direction
		for(d = 0 ; d < numDirections[type] ; d++){
			cs = sq;
			// while sq not offboard
			while((cpiece = board[cs += translation[type][d]]) != OFFBOARD){
				if(cpiece == EMPTY){
					saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++;
				} else {
					if(getColor(piece) != getColor(cpiece)){
						saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
					}
					break;
				}
				// stop if piece is a knight or king
				// won't ever be a king, since I take care of that earlier
				if(type == KNIGHT || type == KING){ break; }
			}
		}
	} else {
		// pawns =================

		// pinned pawns
		if(1ULL << sq120to64(sq) & bs -> pinned){
			dir = getPinDir(bs -> kingSq[bs -> stm], sq);
			// horizontal
			if(abs(dir) == 1) return 0;
			// vertical
			else if(abs(dir) == 10){
				// forward 1
				cs = sq + (1 - 2 * getColor(piece)) * 10;
				if(board[cs] == EMPTY){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				// forward 2
				if(on2ndRank(sq, getColor(piece))){
					cs = sq + (1 - 2 * getColor(piece)) * 20;
					cs2 = sq + (1 - 2 * getColor(piece)) * 10;
					if(board[cs] == EMPTY && board[cs2] == EMPTY){ saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++; }
				}
			}
			// diagonal
			else {
				cs = sq + dir;
				if(board[cs] != EMPTY){
					if(on7thRank(sq, getColor(piece))){
						saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
						saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
					} else {
						saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
					}
				}
			}
			return total;
		}

		// forward 1
		// mapping {0,1} -> {-1,1} -> {-10,10}
		cs = sq + (1 - 2 * getColor(piece)) * 10;
		if(board[cs] == EMPTY){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 8)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 9)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 10)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 11)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 0)); total++;
			}
		}
		// forward 2
		if(on2ndRank(sq, getColor(piece))){
			cs = sq + (1 - 2 * getColor(piece)) * 20;
			cs2 = sq + (1 - 2 * getColor(piece)) * 10;
			if(board[cs] == EMPTY && board[cs2] == EMPTY){
				saveMove(moves, total + offset, buildMove(sq, cs, 1)); total++;
			}
		}
		// captures
		cs = sq + (1 - 2 * getColor(piece)) * 10 + 1;
		if(board[cs] != EMPTY && board[cs] != OFFBOARD \
			&& getColor(piece) != getColor(board[cs])){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
			}
		}
		cs = sq + (1 - 2 * getColor(piece)) * 10 - 1;
		if(board[cs] != EMPTY && board[cs] != OFFBOARD \
			&& getColor(piece) != getColor(board[cs])){
			if(on7thRank(sq, getColor(piece))){
				saveMove(moves, total + offset, buildMove(sq, cs, 12)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 13)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 14)); total++;
				saveMove(moves, total + offset, buildMove(sq, cs, 15)); total++;
			} else {
				saveMove(moves, total + offset, buildMove(sq, cs, 4)); total++;
			}
		}
		// enPas
		cs = bs -> enPas;
		if(cs != OFFBOARD){
			// enPasCaptureFromSq is the same as what it would look like to captrue TO that sq
			enPasCaptureFromSq = cs + (1 - 2 * !getColor(piece)) * 10 + 1;
			if(sq == enPasCaptureFromSq \
				&& enPasCorrectColor(cs, getColor(piece)) \
				&& !epCheck(bs, sq, cs)){
				saveMove(moves, total + offset, buildMove(sq, cs, 5)); total++;
			}
			enPasCaptureFromSq = cs + (1 - 2 * !getColor(piece)) * 10 - 1;
			if(sq == enPasCaptureFromSq \
				&& enPasCorrectColor(cs, getColor(piece)) \
				&& !epCheck(bs, sq, cs)){
				saveMove(moves, total + offset, buildMove(sq, cs, 5)); total++;
			}
		}
	}

	// castling
	int cperm = bs -> castlePermission;
	if(piece == wK && checkDir(board, sq, WHITE) == -1){
		// if cperm exists, not in check and not thru check and not thru piece
		if(cperm & WKCA \
			&& board[F1] == EMPTY && board[G1] == EMPTY \
			&& !newBoardCheck(bs, sq, sq + 1) \
			&& !newBoardCheck(bs, sq, sq + 2)){
			ASSERT(sq == E1);
			saveMove(moves, total + offset, buildMove(sq, sq + 2, 2)); total++;
		}
		if(cperm & WQCA \
			&& board[B1] == EMPTY && board[C1] == EMPTY && board[D1] == EMPTY \
			&& !newBoardCheck(bs, sq, sq - 1) \
			&& !newBoardCheck(bs, sq, sq - 2)){
			ASSERT(sq == E1);
			saveMove(moves, total + offset, buildMove(sq, sq - 2, 3)); total++;
		}
	}
	else if(piece == bK && checkDir(board, sq, BLACK) == -1){
		if(cperm & BKCA \
			&& board[F8] == EMPTY && board[G8] == EMPTY \
			&& !newBoardCheck(bs, sq, sq + 1) \
			&& !newBoardCheck(bs, sq, sq + 2)){
			ASSERT(sq == E8);
			saveMove(moves, total + offset, buildMove(sq, sq + 2, 2)); total++;
		}
		if(cperm & BQCA \
			&& board[B8] == EMPTY && board[C8] == EMPTY && board[D8] == EMPTY \
			&& !newBoardCheck(bs, sq, sq - 1) \
			&& !newBoardCheck(bs, sq, sq - 2)){
			ASSERT(sq == E8);
			saveMove(moves, total + offset, buildMove(sq, sq - 2, 3)); total++;
		}
	}
	return total;
}

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

void printBoard(BOARD_STATE *bs, int option){
	int i, index120, rank, file, piece, sq64;

	if(option == OPT_120_BOARD){
		printf(YEL " ---- 120 Board ---- \n" reset);
		for(i = 0 ; i < 120 ; i++){
			// same horizontal board flip logic as in invertRows()
			index120 = 110 + i - 2 * (i - i % 10);
			printf("%2d ", bs -> board[index120]);
			if((i + 1) % 10 == 0){
				printf("\n");
			}
		}
		printf("\n");
	}

	if(option == OPT_64_BOARD){
		printf(YEL " ---- Game Board ---- \n" reset);
		for(rank = 8 ; rank >= 1 ; rank--){
			printf(RED "%d " reset, rank);
			for(file = 1 ; file <= 8 ; file++){
				piece = bs -> board[sq64to120(frToSq64(file, rank))];
				printf("%2c", pieceChar[piece]);
			}
			printf("\n");
		}
		// print the files
		printf("\n  ");
		for(file = 0 ; file < 8 ; file++){
			printf(RED "%2c" reset, file + 'a');
		}
		printf("\n");
	}

	if(option == OPT_BOARD_STATE){
		char sqAN[3];
		char cperms[5];
		char boardFEN[99];
		printf(BLU "side to move: " reset "%s\n", bs -> stm == WHITE ? "white" : "black");
		printf(BLU "ply: " reset "%d\n", bs -> ply);
		getAlgebraic(sqAN, bs -> enPas);
		getCastlePermissions(cperms, bs -> castlePermission);
		printf(BLU "en passant sq: " reset "%s\n", sqAN);
		printf(BLU "castlePerms: " reset "%s\n", cperms);
		printf(BLU "white in check: " reset "%s\n", checkDir(bs -> board, bs -> kingSq[WHITE], WHITE) >= 0 ? "true" : "false");
		printf(BLU "black in check: " reset "%s\n", checkDir(bs -> board, bs -> kingSq[BLACK], BLACK) >= 0 ? "true" : "false");
		printf(BLU "white kingSq: " reset "%d\n", bs -> kingSq[WHITE]);
		printf(BLU "black kingSq: " reset "%d\n", bs -> kingSq[BLACK]);
		printf(BLU "eval: " reset "%d\n", eval(bs));
		bs -> stm = !(bs -> stm);
		// TODO: the num legal moves might be innacurate
		// if just switching stms due to en passant?
		// "eval for opponent" doesn't really make sense since it's not their turn
		printf(BLU "eval for opponent: " reset "%d\n", eval(bs));
		bs -> stm = !(bs -> stm);
		genFEN(bs, boardFEN);
		printf(BLU "fen: " reset "%s\n", boardFEN);
	}

	if(option == OPT_PINNED){
		printf(YEL " ---- Absolute Pins ---- \n" reset);
		for(int i = 0 ; i < 64 ; i++){
			printf("%d ", !!(1ULL << i & bs -> pinned));
			if((i + 1) % 8 == 0) printf("\n");
		}
	}
}

int parseArgs(char *inputFEN, int argc, char *argv[]){
	int c;
  while((c = getopt(argc, argv, "sptf:")) != -1){
		switch(c){
			case 'f':
				strcpy(inputFEN, optarg);
				return FEN_MODE;
				break;
			case 't':
				return TEST_MODE;
				break;
			case 'p':
				return PERFT_MODE;
				break;
			case 's':
				return SEARCH_MODE;
				break;
			case '?':
				if(optopt == 'f')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return -1;
			default:
				fprintf(stderr, "Error with arg parsing\n", optopt);
				exit(1);
		}
	}
	// default to NORMAL_MODE
	return NORMAL_MODE;
}

int main(int argc, char *argv[]){
	initRand();
	BOARD_STATE *bs = initGame();

	// declaring here so that parseArgs can strcpy into inputFEN
	char inputFEN[99];
	char outputFEN[99];

	mode = parseArgs(inputFEN, argc, argv);

	// NORMAL_MODE - print out all legal moves of a pos
	if(mode == NORMAL_MODE){
		char testFEN[] = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -";
		parseFEN(bs, testFEN);

		// print board
		printBoard(bs, OPT_64_BOARD);
		printBoard(bs, OPT_BOARD_STATE);
		// printBoard(bs, OPT_PINNED);

		// print moves
		printLegalMoves(bs);
	}

	// FEN_MODE - return fen of best move
	else if(mode == FEN_MODE){
		MOVE myMoves[255];
		int evals[255];
		parseFEN(bs, inputFEN);
		int total = genLegalMoves(bs, myMoves);
		int best = -11111;
		int b = -1;
		for(int m = 0 ; m < total ; m++){
			makeMove(bs, myMoves[m]);
			evals[m] = -1 * treeSearch(bs, 3);
			undoMove(bs);
			if(evals[m] > best){
				best = evals[m];
				b = m;
			}
		}
		makeMove(bs, myMoves[b]);
		genFEN(bs, outputFEN);
		printf("%s\n", outputFEN);
	}

	// TEST_MODE - perform tests in test.c
	else if(mode == TEST_MODE){
		printf("Checking board initialization...\n");
		ASSERT(bs -> castlePermission == 0 && bs -> enPas == OFFBOARD);
		printf("Checking util functions...\n");
		ASSERT(testUtilFunctions());
		printf("Checking perft positions...\n");
		ASSERT(testMoveGenPositions());
	}

	// PERFT_MODE - checks number of positions
	else if (mode == PERFT_MODE){
		char testFEN[] = "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b QK -";
		parseFEN(bs, START_FEN);
		printBoard(bs, OPT_64_BOARD);
		printBoard(bs, OPT_BOARD_STATE);

		int tot = (int)perft2(bs, 6);
		printf(RED "total: " reset "%i\n", tot);
	}

	// SEARCH_MODE - output first layer of search
	else if(mode == SEARCH_MODE){
		parseFEN(bs, START_FEN);

		MOVE myMoves[255];
		int evals[255];
		int total, randint;

		// for(int m = 0 ; m < total ; m++){
		// 	printf("evaluating: ");
		// 	printMove(m, myMoves[m]);
		// 	makeMove(bs, myMoves[m]);
		// 	// evals[m] = -1 * treeSearch(bs, 2);
		// 	t = (int)perft(bs, 1);
		// 	printf(YEL "\t\tTotal moves: " reset "%d\n", t);
		// 	undoMove(bs);
		// }

		for(int m = 0 ; m < 300 ; m++){
			total = genLegalMoves(bs, myMoves);
			if(total == 0){
				printBoard(bs, OPT_64_BOARD);
				printf("\nCHECKMATE ======================= \n");
				genFEN(bs, outputFEN);
				printf("%s\n", outputFEN);
				exit(0);
			}
			randint = rand() % total;
			makeMove(bs, myMoves[randint]);
		}
		printBoard(bs, OPT_64_BOARD);
		printBoard(bs, OPT_BOARD_STATE);

		for(int m = 0 ; m < 300 ; m++){
			undoMove(bs);
		}

		printf(RED "\n====== AFTER UNDOS ========\n" reset);
		printBoard(bs, OPT_64_BOARD);
		printBoard(bs, OPT_BOARD_STATE);
	}
}
