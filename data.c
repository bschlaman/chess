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

// test positions
TEST_POSITION tps[] = {
	{
		.fen = START_FEN,
		.depth = 4,
		.nodes = 197281,
	},
	// maximum legal moves
	{
		.fen = "R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - -",
		.depth = 1,
		.nodes = 218,
	},
	// next 5 positions test en passant pins and check escapes
	{
		.fen = "8/8/8/2k5/2pP4/8/B7/4K3 b - d3",
		.depth = 1,
		.nodes = 8,
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
		.fen = "8/B7/8/8/2pP4/8/8/3K2k1 b - d3",
		.depth = 1,
		.nodes = 6,
	},
	// horizontal en passant pin
	{
		.fen = "8/8/8/8/R1pP2k1/8/8/4K3 b - d3",
		.depth = 1,
		.nodes = 9,
	},
	// white king in check
	{
		.fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -",
		.depth = 1,
		.nodes = 6,
	},
	// castling thru check
	{
		.fen = "r3k2r/1p6/8/8/b4Pp1/8/8/R3K2R w KQkq -",
		.depth = 1,
		.nodes = 21,
	},
	// white pawn capture promotion (pos 5 from wiki)
	{
		.fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
		.depth = 2,
		.nodes = 1486,
	},
	// KiwiPete position (pos 2 from wiki) (depth 1)
	{
		.fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
		.depth = 1,
		.nodes = 48,
	},
	// KiwiPete position (pos 2 from wiki) (depth 4)
	{
		.fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
		.depth = 4,
		.nodes = 4085603,
	},
	// black king in check; can block and capture attacker
	{
		.fen = "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b QK -",
		.depth = 1,
		.nodes = 8,
	},
	{
		.fen = "r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w QqKk -",
		.depth = 1,
		.nodes = 19,
	},
	{
		.fen = "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b QqKk -",
		.depth = 1,
		.nodes = 5,
	},
	{
		.fen = "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b QK -",
		.depth = 1,
		.nodes = 44,
	},
	{
		.fen = "rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w QK -",
		.depth = 1,
		.nodes = 39,
	},
	{
		.fen = "2r5/3pk3/8/2P5/8/2K5/8/8 w - -",
		.depth = 1,
		.nodes = 9,
	},
	{
		.fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
		.depth = 3,
		.nodes = 62379,
	},
	{
		.fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -",
		.depth = 3,
		.nodes = 89890,
	},
	{
		.fen = "3k4/3p4/8/K1P4r/8/8/8/8 b - -",
		.depth = 6,
		.nodes = 1134888,
	},
	{
		.fen = "8/8/4k3/8/2p5/8/B2P2K1/8 w - -",
		.depth = 6,
		.nodes = 1015133,
	},
	{
		.fen = "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3",
		.depth = 6,
		.nodes = 1440467,
	},
	{
		.fen = "5k2/8/8/8/8/8/8/4K2R w K -",
		.depth = 6,
		.nodes = 661072,
	},
	{
		.fen = "3k4/8/8/8/8/8/8/R3K3 w Q -",
		.depth = 6,
		.nodes = 803711,
	},
	{
		.fen = "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq -",
		.depth = 4,
		.nodes = 1274206,
	},
	{
		.fen = "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq -",
		.depth = 4,
		.nodes = 1720476,
	},
	{
		.fen = "2K2r2/4P3/8/8/8/8/8/3k4 w - -",
		.depth = 6,
		.nodes = 3821001,
	},
	{
		.fen = "8/8/1P2K3/8/2n5/1q6/8/5k2 b - -",
		.depth = 5,
		.nodes = 1004658,
	},
	{
		.fen = "4k3/1P6/8/8/8/8/K7/8 w - -",
		.depth = 6,
		.nodes = 217342,
	},
	{
		.fen = "8/P1k5/K7/8/8/8/8/8 w - -",
		.depth = 6,
		.nodes = 92683,
	},
	{
		.fen = "K1k5/8/P7/8/8/8/8/8 w - -",
		.depth = 6,
		.nodes = 2217,
	},
	{
		.fen = "8/k1P5/8/1K6/8/8/8/8 w - -",
		.depth = 7,
		.nodes = 567584,
	},
	{
		.fen = "8/8/2k5/5q2/5n2/8/5K2/8 b - -",
		.depth = 4,
		.nodes = 23527,
	},
};

size_t tpsSize(){
	return sizeof(tps) / sizeof(TEST_POSITION);
}
