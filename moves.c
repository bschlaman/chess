#include <stdio.h>
#include "defs.h"
#include "colors.h"

int getFrom(MOVE m){
	return sq64to120(m >> 10);
}
int getTo(MOVE m){
	return sq64to120((m >> 4) & 63);
}
int getMType(MOVE m){
	return m & 15;
}
MOVE buildMove(int from, int to, int moveType){
	return 0 | sq120to64(from) << 10 | sq120to64(to) << 4 | moveType;
}
// TODO: get rid of moveType and only use the 16-bit fromto
// should split this into reversible and irreversible aspects
// so that the reversible aspects can be used
// both in make and unmake

void updatePins(BOARD_STATE *bs, bool side){
	int kingsq = bs -> kingSq[side];
	int *board = bs -> board;
	int d, cs, cpiece;
	bool ownPieceSeen;
	for(d = 0 ; d < numDirections[KING] ; d++){
		cs = kingsq;
		ownPieceSeen = false;
		while((cpiece = board[cs += translation[KING][d]]) != OFFBOARD){
			if(cpiece == EMPTY) continue;
			else if(getColor(cpiece) != side && !ownPieceSeen) break;
			else if(getColor(cpiece) == side && ownPieceSeen) break;
			else if(getColor(cpiece) == side && !ownPieceSeen) ownPieceSeen = cs;
			// opposing piece, and ownPieceSeen
			else {
				if(d < 4){
					if(getType(cpiece) == BISHOP || getType(cpiece) == QUEEN) bs -> pinned |= 1ULL << sq120to64(ownPieceSeen);
				} else if(d >= 4 && d < 8){
					if(getType(cpiece) == ROOK || getType(cpiece) == QUEEN) bs -> pinned |= 1ULL << sq120to64(ownPieceSeen);
				}
				break;
			}
		}
	}
}
void updatePins2(BOARD_STATE *bs, int asdf, int side){
	int kingsq = bs -> kingSq[side];
	int *board = bs -> board;
	int d, cs, cpiece;
	bool ownPieceSeen;
	for(d = 0 ; d < numDirections[KING] ; d++){
		cs = kingsq;
		ownPieceSeen = false;
		while((cpiece = board[cs += translation[KING][d]]) != OFFBOARD){
			if(cpiece == EMPTY) continue;
			else if(getColor(cpiece) != side && !ownPieceSeen) break;
			else if(getColor(cpiece) == side && ownPieceSeen) break;
			else if(getColor(cpiece) == side && !ownPieceSeen) ownPieceSeen = cs;
			// opposing piece, and ownPieceSeen
			else {
				if(d < 4){
					if(getType(cpiece) == BISHOP || getType(cpiece) == QUEEN) bs -> pinned |= 1ULL << sq120to64(ownPieceSeen);
				} else if(d >= 4 && d < 8){
					if(getType(cpiece) == ROOK || getType(cpiece) == QUEEN) bs -> pinned |= 1ULL << sq120to64(ownPieceSeen);
				}
				break;
			}
		}
	}
}

void makeMove(BOARD_STATE *bs, MOVE move){
	// TODO: looking up the board is very expensive!!!
	// TODO: I think the logic for 3) is wonky...
	// I should be using nested if statements in a for loop
	// for(i = 1 ; i < 16 ; i = i * 2)
	// and maybe capture will only be set on ms
	// if moveType == 4

	// 1) update the move stack with info about current pos
	// TODO: change ms to mi or something
	MOVE_IRREV *ms = &(bs -> history[bs -> ply]);
	int capturedPiece;
	int from = getFrom(move);
	int to = getTo(move);
	int moveType = getMType(move);
	int cperm = bs -> castlePermission;
	ms -> move = move;
	ms -> enPas = bs -> enPas;
	ms -> castlePermission = cperm;
	ms -> pinned = bs -> pinned;
	if(moveType == 5){
		capturedPiece = bs -> board[to - (1 - 2 * bs -> stm) * 10];
	} else { capturedPiece = bs -> board[to]; }
	ms -> capturedPiece = capturedPiece;
	// 2) increment ply (index)
	bs -> ply++;

	// 3) update board with the move
	// ----------------------
	int *board = bs -> board;
	int piece = board[from];

	// special stuff
	switch(moveType){
		case 0: break;
		case 1:
			// setting enPas
			if(isPawn[piece] && abs(to - from) == 20){
				bs -> enPas = to - (1 - 2 * getColor(piece)) * 10;
			}
			break;
		case 2:
		case 3:
			// TODO: put some of his in case 2:
			// TODO: assert during FEN parse that castleperm implies rook and king location
			// i.e. KQkq -> rooks in the corners and kings on the proper squares
			// castling, move the rook
			switch(to){
				case G1: board[H1] = EMPTY; board[F1] = wR; break;
				case C1: board[A1] = EMPTY; board[D1] = wR; break;
				case G8: board[H8] = EMPTY; board[F8] = bR; break;
				case C8: board[A8] = EMPTY; board[D8] = bR; break;
				default:
					printf(RED "Error castling: %d\n" reset, from);
					printf(RED "Error castling: %d\n" reset, board[from]);
					printf(RED "Error castling: %d\n" reset, bs -> castlePermission);
					exit(1);
			}
			break;
		case 4: break;
		case 5:
			// en passant
			// capture the enPas pawn
			board[to - (1 - 2 * getColor(piece)) * 10] = EMPTY;
			break;
		case 8:
			piece = getColor(piece) ? bN : wN; break;
		case 9:
			piece = getColor(piece) ? bB : wB; break;
		case 10:
			piece = getColor(piece) ? bR : wR; break;
		case 11:
			piece = getColor(piece) ? bQ : wQ; break;
		case 12:
			piece = getColor(piece) ? bN : wN; break;
		case 13:
			piece = getColor(piece) ? bB : wB; break;
		case 14:
			piece = getColor(piece) ? bR : wR; break;
		case 15:
			piece = getColor(piece) ? bQ : wQ; break;
		default:
			printf(RED "Error with moveType on makeMove\n" reset);
			exit(1);
	}

	// TODO: these need to be elsewhere
	// Rook moves, cperm check at the end is only for efficiency
	if(piece == wR){
		if(from == H1 && cperm & WKCA){ bs -> castlePermission &= 7; }
		if(from == A1 && cperm & WQCA){ bs -> castlePermission &= 11; }
	}
	else if(piece == bR){
		if(from == H8 && cperm & BKCA){ bs -> castlePermission &= 13; }
		if(from == A8 && cperm & BQCA){ bs -> castlePermission &= 14; }
	}
	if(capturedPiece == wR){
		if(to == H1 && cperm & WKCA){ bs -> castlePermission &= 7; }
		if(to == A1 && cperm & WQCA){ bs -> castlePermission &= 11; }
	}
	else if(capturedPiece == bR){
		if(to == H8 && cperm & BKCA){ bs -> castlePermission &= 13; }
		if(to == A8 && cperm & BQCA){ bs -> castlePermission &= 14; }
	}
	// unset en passant sq
	if(moveType != 1){
		bs -> enPas = OFFBOARD;
	}
	// unset cperms on king moves, castling or not
	if(isKing[piece]){
		if(getColor(piece) == BLACK){
			ASSERT(piece == bK);
			// TODO: put these numbers in terms of WKCA, etc.
			bs -> castlePermission &= 12;
		} else {
			ASSERT(piece == wK);
			bs -> castlePermission &= 3;
		}
		bs -> kingSq[bs -> stm] = to;
	}

	// setting the pieces, switching side, and updating pins
	board[to] = piece;
	board[from] = EMPTY;
	// TODO: if a king moves, only need to update that side
	bs -> pinned = 0ULL;
	updatePins(bs, WHITE);
	updatePins(bs, BLACK);
	// is this the best way to switch sides?
	bs -> stm = !(bs -> stm);
}

void undoMove(BOARD_STATE *bs){
	int from, to, moveType, stm;

	// 1) decrement ply
	bs -> ply--;

	MOVE_IRREV *ms = &(bs -> history[bs -> ply]);
	int capturedPiece = ms -> capturedPiece;
	MOVE move = ms -> move;
	from = getFrom(move);
	to = getTo(move);
	moveType = getMType(move);

	// 2) restore reversible
	int *board = bs -> board;
	stm = bs -> stm;
	// setting the pieces and switching side
	board[from] = board[to];
	board[to] = EMPTY;
	if(isKing[board[from]]) bs -> kingSq[!stm] = from;
	// is this the best way to switch sides?
	bs -> stm = !stm;

	// 3) restore irreversible
	// 3.1) restore non-board stuff
	bs -> enPas = ms -> enPas;
	bs -> castlePermission = ms -> castlePermission;
	capturedPiece = ms -> capturedPiece;
	bs -> pinned = ms -> pinned;

	// 3.2) restore board
	// regular captures
	if(moveType & 4 == 0){
		board[to] = EMPTY;
	}
	if(moveType == 4 || (moveType >= 12 && moveType <= 15)){
		board[to] = capturedPiece;
	}
	// en passant capture
	if(moveType == 5){
		board[to] = EMPTY;
		// TODO: I use getColor elsewhere
		board[to + (1 - 2 * getColor(capturedPiece)) * 10] = capturedPiece;
	}
	// promotions
	if(moveType >= 8 && moveType <= 15){
		board[from] = bs -> stm ? bP : wP;
	}
	// uncastling, move the rook
	if(moveType == 2 || moveType == 3){
		switch(to){
			case G1: board[H1] = wR; board[F1] = EMPTY; break;
			case C1: board[A1] = wR; board[D1] = EMPTY; break;
			case G8: board[H8] = bR; board[F8] = EMPTY; break;
			case C8: board[A8] = bR; board[D8] = EMPTY; break;
			default:
				printf(RED "Error uncastling: %d\n" reset, board[from]);
				printf(RED "Error uncastling: %d\n" reset, board[to]);
				exit(1);
		}
	}
}

