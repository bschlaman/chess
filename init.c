#include <stdlib.h>
#include <time.h>
#include "defs.h"

void initRand(){
	time_t t;
	srand((unsigned) time(&t));
}

BOARD_STATE* initGame(){
	BOARD_STATE *bs = malloc(sizeof(BOARD_STATE));
	// must always initialize board!
	resetBoard(bs);
	return bs;
}

