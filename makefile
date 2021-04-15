all:
	cc main.c fen.c data.c init.c moves.c eval.c test.c -o out
	./out
fen:
	cc main.c fen.c data.c init.c moves.c eval.c test.c -o out
	./out -f "${FEN}"
perft:
	cc main.c fen.c data.c init.c moves.c eval.c test.c -o out
	./out -p
search:
	cc main.c fen.c data.c init.c moves.c eval.c test.c -o out
	./out -s
only:
	cc main.c fen.c data.c init.c moves.c eval.c test.c -o out
sand:
	cc sand.c data.c -o out_test
	./out_test
