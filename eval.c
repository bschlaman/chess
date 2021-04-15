#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "colors.h"

int pieceMobility(BOARD_STATE *bs, int piece, int sq);

float eval(BOARD_STATE *bs){
	int i, sq;

	int materialEval = 0;
	int mobilityEval = 0;
	int pawnEval = 0;

	int kingWeight = 200;
	int queenWeight = 9;
	int rookWeight = 5;
	int bishopWeight = 3;
	int knightWeight = 3;
	int pawnWeight = 1;

	int pawnPosWeight[8] = {0, 0, 0, 1, 2, 3, 5, 5};

	// float mobilityWeight = 0.1;
	float mobilityWeight = 0.2;
	int mobility = 0;

	int *board = bs -> board;
	int side = bs -> side;
	int kings = 0;
	int queens = 0;
	int rooks = 0;
	int bishops = 0;
	int knights = 0;
	int pawns = 0;
	int piece;
	int factor;
	for(i = 0 ; i < 64 ; i++){
		sq = sq64to120(i);
		piece = board[sq];

		factor = 1 - 2 * (side ^ getColor(piece));
		if(piece == wK || piece == bK) kings += factor;
		if(piece == wQ || piece == bQ) queens += factor;
		if(piece == wR || piece == bR) rooks += factor;
		if(piece == wB || piece == bB) bishops += factor;
		if(piece == wN || piece == bN) knights += factor;
		if(piece == wP || piece == bP) pawns += factor;
		if(piece == wP && i < 40){
			pawnEval += factor * pawnPosWeight[7 - (i - i % 8) / 8];
		}
		if(piece == bP && i > 23){
			pawnEval += factor * pawnPosWeight[(i - i % 8) / 8];
		}
		if(piece != EMPTY && getColor(piece) == side) mobility += pieceMobility(bs, piece, sq);
	}
	materialEval += kings * kingWeight;
	materialEval += queens * queenWeight;
	materialEval += rooks * rookWeight;
	materialEval += bishops * bishopWeight;
	materialEval += knights * knightWeight;
	materialEval += pawns * pawnWeight;
	mobilityEval = mobility * mobilityWeight;
	// avoid checkmate
	// TODO: make this better obviously
	// if(bs -> side == BLACK){
	// 	printf(RED "%d\n" reset, numLegalMoves);
	// 	printf(RED "%d\n" reset, materialEval);
	// } else {
	// 	printf(YEL "%d\n" reset, numLegalMoves);
	// 	printf(YEL "%d\n" reset, materialEval);
	// }
	return materialEval + mobilityEval + pawnEval;
}

int pieceMobility(BOARD_STATE *bs, int piece, int sq){
	int d, cs, cpiece, type, total = 0;
	int *board = bs -> board;
	// pieces
	if(!isPawn[piece]){
		type = getType(piece);
		for(d = 0 ; d < numDirections[type] ; d++){
			cs = sq;
			while((cpiece = board[cs += translation[type][d]]) != OFFBOARD){
				if(cpiece == EMPTY) total++;
				else { break;	}
				if(type == KNIGHT || type == KING){ break; }
			}
		}
	}
	return total;
}

int randInt(int lb, int ub){
	return rand() % (ub - lb + 1) + lb;
}

float treeSearch(BOARD_STATE *bs, int depth){
	if(depth == 0){ return eval(bs); }
	int m;
	int posEval, b = 0;
	MOVE localLM[255];
	int total = genLegalMoves(bs, localLM);

	if(total == 0) return -100000;

	// // debug
	// if(depth == 2){
	//  	printf(BLU "side to move: " reset "%s\n", bs -> side == WHITE ? "white" : "black");
	// 	printLegalMoves(bs);
	// 	// for(m = 0 ; m < 100 ; m++){
	// 	// // for(m = 0 ; localLM[m][2] != -1 ; m++){
	// 	// 	printf(CYN "%d\n" reset, localLM[m][2]);
	// 	// }
	// 	// exit(0);
	// }
	// if(depth == 1 && bs -> board[72] == wQ){
	//  	printf(BLU "side to move: " reset "%s\n", bs -> side == WHITE ? "white" : "black");
	// 	printLegalMoves(bs);
	// }

	float bestScore = -10000;
	for(m = 0 ; m < total ; m++){
		// debug
		// if(depth == 2 && m > 28) return 1;
		// if(depth == 2 && m > 28) return 1;

		makeMove(bs, localLM[m]);
		posEval = -1 * treeSearch(bs, depth - 1);
		undoMove(bs);

		if(posEval > bestScore){
			bestScore = posEval;
			b = m;
			// if(depth == 2){
			// 	printf(RED "move: %d score: %d\n" reset, m, bestScore);
			// }
		}
	}
	// if(depth == 2){
	// 	printf("------------\n");
	// 	printMove(b, localLM[b][0], localLM[b][1], localLM[b][2]);
	// 	printf("------------\n");
	// 	exit(0);
	// }
	return bestScore;
}


U64 perft(BOARD_STATE *bs, int depth){
	MOVE localLM[255];
	int num, m;
	U64 nodes = 0;

	if(depth == 0) return 1ULL;

	num = genLegalMoves(bs, localLM);

  for(m = 0 ; m < num ; m++) {
		makeMove(bs, localLM[m]);
		nodes += perft(bs, depth - 1);
		undoMove(bs);
  }
	return nodes;
}

U64 perft2(BOARD_STATE *bs, int depth){
	MOVE localLM[255];
	int num, m;
	U64 nodes = 0;


	num = genLegalMoves(bs, localLM);

	if(depth == 1) return num;

  for(m = 0 ; m < num ; m++) {
		makeMove(bs, localLM[m]);
		nodes += perft2(bs, depth - 1);
		undoMove(bs);
  }
	return nodes;
}




















