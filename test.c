#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colors.h"
#include "defs.h"

int testMoves(){
	BOARD_STATE *tbs = initGame();
	MOVE moves[255];
	parseFEN(tbs, MAXM_FEN);
	int total = genLegalMoves(tbs, moves);
	free(tbs);
	return total == 218;
}

int testHelperFunctions(){
	int pass = false;
	pass = pass && sq64to120(26) == 53;
	pass = pass && sq120to64(76) == 45;
	pass = pass && frToSq64(4, 7) == 51;

	char sqfr[3];
	getAlgebraic(sqfr, 55);
	pass = pass && strcmp(sqfr, "e4");

	char cperms[5];	
	getCastlePermissions(cperms, 0);
	pass = pass && strcmp(cperms, "-");
	getCastlePermissions(cperms, WKCA | BQCA | BKCA);
	pass = pass && strcmp(cperms, "Kkq");

	ASSERT(getType(bB) == BISHOP);
	ASSERT(getColor(wP) == WHITE);
	ASSERT(sq64to120(26) == 53);
	ASSERT(sq120to64(76) == 45);
	ASSERT(frToSq64(4, 7) == 51);
	ASSERT(strcmp(sqfr, "e4"));
	ASSERT(strcmp(cperms, "-"));
	ASSERT(strcmp(cperms, "Kkq"));
	ASSERT(getType(bB) == BISHOP);
	ASSERT(getColor(wP) == WHITE);

	return pass;
}
// TODO: make test that makes and undoes many random moves
