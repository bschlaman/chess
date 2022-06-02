#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "colors.h"

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

// usage:
// unsigned long a = _;
// printBits(sizeof(a), &a);
void printBits(size_t const size, void const * const ptr){
	// casting to uchar so we can iterate by "sizes" (bytes)
	// b[0], b[1], ... are all of size uchar
	unsigned char *b = (unsigned char*) ptr;
	for(int byte = size - 1; byte >= 0; byte--){
		for(int bit = 7; bit >= 0; bit--){
			printf("%u", (b[byte] >> bit) & 1);
		}
	}
	puts("");
}
