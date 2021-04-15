#include "defs.h"

const char pieceChar[] = ".PNBRQKpnbrqkx";
const char castleChar[] = "KQkq";
const int isPawn[] = {false, true, false, false, false, false, false, true, false, false, false, false, false, false};
const int isKing[] = {false, false, false, false, false, false, true, false, false, false, false, false, true, false};
const int OFFBOARD = -1;

const int numDirections[] = {8, 4, 4, 8, 8};
const int translation[][8] = {
	{-21, -12, 8, 19, 21, 12, -8, -19}, // knights 0
	{-11, 9, 11, -9}, // bishops 1
	{-10, -1, 10, 1}, // rooks 2
	{-11, 9, 11, -9, -10, -1, 10, 1}, // queens 3
	{-11, 9, 11, -9, -10, -1, 10, 1}  // kings 4
};

int mode = NORMAL_MODE;
int counter = 0;

