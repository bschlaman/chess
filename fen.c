#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "colors.h"

int parseFEN(char *fen, BOARD_STATE *bs){
	// ranks start canonically at 8
	// but files are letters so who cares
	int i, num, piece, rank = 8, file = 0, sq64;
	while(*fen && rank > 0){
		num = 1;
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
				file = 0;
				fen++;
				continue;

			default:
				printf(RED "Error with FEN\n" reset);
				exit(1);
				return -1;

		}

		if(piece == wK){ bs -> kingSq[WHITE] = sq64to120(frToSq64(file, rank)); }
		if(piece == bK){ bs -> kingSq[BLACK] = sq64to120(frToSq64(file, rank)); }

		for(i = 0 ; i < num ; i++){
			bs -> board[sq64to120(frToSq64(file, rank))] = piece;
			file++;
		}
		fen++;
	}

	// pinned
	bs -> pinned = 0ULL;
	updatePins(bs, WHITE);
	updatePins(bs, BLACK);

	// side
	bs -> side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	// castling
	for(i = 0 ; i < 4 ; i++){
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
	ASSERT(bs -> castlePermission >= 0 && bs -> castlePermission <= 15);

	// en passant
	if(*fen != '-'){
		file = fen[0] - 'a';
		rank = fen[1] - '0';
		ASSERT(file >= 0 && file <= 8);
		ASSERT(rank >= 0 && rank <= 8);
		bs -> enPas = sq64to120(frToSq64(file, rank));
	}

	return 0;
}

int genFEN(char *fen, BOARD_STATE *bs){
	// f is the FEN string index
	// TODO: I think I don't have to track f
	// and can just advance the pointer?
	int i, f, piece, p, num;
	int *board = bs -> board;
	// pieces
	for(i = 0, f = 0, num = 0; i < 64 ; i++){
		piece = board[sq64to120(i)];
		p = pieceChar[piece];
		ASSERT(piece >= 0 && piece <= 12);
		if(piece == EMPTY){
			num++;	
		} else {
			if(num > 0){
				fen[f] = num + '0';	
				f++;
			}
			num = 0;
			fen[f] = p;
			f++;
		}
		if((i + 1) % 8 == 0){
			if(num > 0){
				fen[f] = num + '0';	
				f++;
			}
			fen[f] = '/';
			f++;
			num = 0;
		}
	}
	fen[f - 1] = ' ';
	fen[f] = bs -> side ? 'b' : 'w';
	fen[f + 1] = ' ';
	f += 2;
	
	// castle permission
	if(bs -> castlePermission == 0){
		fen[f] = '-';
		f++;
	} else {
		ASSERT(bs -> castlePermission >= 0 && bs -> castlePermission <= 15);
		for(i = 0 ; i < 4 ; i++){
			if(1 << (3 - i) & bs -> castlePermission){
				fen[f] = castleChar[i];
				f++;
			}
		}
	}
	fen[f] = ' ';
	
	// en passant
	char sqfr[2];
	getAlgebraic(sqfr, bs -> enPas);
	fen[f + 1] = sqfr[0];
	fen[f + 2] = sqfr[1];
	fen[f + 3] = '\0';
	return 0;
}
