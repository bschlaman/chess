#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colors.h"
#include "defs.h"

bool testUtilFunctions(){
	int pass = true;
	pass = pass && sq64to120(26) == C4;
	pass = pass && sq120to64(F6) == 45;
	pass = pass && frToSq64(4, 7) == 51;

	char sqfr[3];
	getAlgebraic(sqfr, E4);
	pass = pass && strcmp(sqfr, "e4") == 0;

	char cperms[5];	
	getCastlePermissions(cperms, 0);
	pass = pass && strcmp(cperms, "-") == 0;
	getCastlePermissions(cperms, WKCA | BQCA | BKCA);
	pass = pass && strcmp(cperms, "Kkq") == 0;

	pass = pass && getType(bB) == BISHOP;
	pass = pass && getColor(wP) == WHITE;

	// TODO: just make the whole thing ASSERT statements
	ASSERT(getType(bB) == BISHOP);
	ASSERT(getColor(wP) == WHITE);
	ASSERT(sq64to120(26) == C4);
	ASSERT(sq120to64(F6) == 45);
	ASSERT(frToSq64(4, 7) == 51);
	getAlgebraic(sqfr, E4);
	ASSERT(strcmp(sqfr, "e4") == 0);
	getCastlePermissions(cperms, 0);
	ASSERT(strcmp(cperms, "-") == 0);
	getCastlePermissions(cperms, WKCA | BQCA | BKCA);
	ASSERT(strcmp(cperms, "Kkq") == 0);

	ASSERT(on2ndRank(H7, BLACK));
	ASSERT(on7thRank(A7, WHITE));

	return pass;
}
// TODO: make test that makes and undoes many random moves

bool testMoveGenPositions(){
	// create test board state and loop thru test positions in data.c
	BOARD_STATE *tbs = initGame();
	int total;
	for(int i = 0 ; i < tpsSize() ; i++){
		resetBoard(tbs);
		parseFEN(tbs, tps[i].fen);
		printf("pos: %2d, depth: %3d ", i, tps[i].depth, i);
		total = perft2(tbs, tps[i].depth);
		printf("wanted: %8d, got: %8d ", tps[i].nodes, total);
		printf("%s\n" reset, total == tps[i].nodes ? GRN "✓" : RED "X");
		ASSERT(total == tps[i].nodes);
	}
	return true;
}
