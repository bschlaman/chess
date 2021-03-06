## Goals
 * improve the performance of my perft
 * explore other board representations

My engine currently uses a 10x12 representation, which is the smallest setup that uses piece offsets with OFFBOARD checks.

## Disecting qperft.c
The memory location for the board seems to be the board itself plus capture codes and a delta vector (whatever that is).
```c
#define board      (brd+1)                /* 12 x 16 board: dbl guard band */
#define capt_code  (brd+1+0xBC+0x77)      /* piece type that can reach this*/
#define delta_vec  ((char *) brd+1+0xBC+0xEF+0x77) /* step to bridge certain vector */
```
Ignoring for now that `brd` starts earlier than `board`, the board has a length of `0xBC + 0x77 = 0x133` or `188 + 119 = 307`.  The comments in the code indicate that it is a 12 x 16 board representation, but that only comes out to a size of 192.  What's more, following the 0x88 style, only the first 185 or 186 squares are useful (depending on where the board is placed within the guard squares).

The next clue is in the `board_init` function.
```c
for(i= -1; i<0xBC; i++) b[i] = (i-0x22)&0x88 ? GUARD : DUMMY;
```
So it appears that the 0xBC portion of the board houses the actual squares.  0x88 shows up here to determine if the square is offboard.  The advantage of an 0x88 board representation is that the 16ths place nibble represents the rank, and the 1s place nibble represents the file.  This means taking the bitwise & operator with a square and 0x88 (1000 1000<sub>2</sub>) will determine if it is OFFBOARD.
There is also an offset of 0x22 here, which indicates that there are two rows of 16 guard squares +2 on the 3rd row.  Using the following code, I've printed the resulting board:
```c
int invertRows(int i){
	return 0xB0 + i - 2 * (i - i % 0x10);
}
for(int i = 0; i < 0xBC + 4; i++){
	if(i%0x10 == 0) printf("\n");
	printf("%c ", (invertRows(i)-0x22)&0x88 ? '-' : 'X');
}
```
Output:
```
- - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - X X X X X X X X - - - - - -
- - - - - - - - - - - - - - - -
- - - - - - - - - - - - - - - -
```

The problem with printing out a board with A1 in the lower left corner is that the for loop will print starting at the top.  I therefore introduce the `invertRows` function that assumes a 12x16 board layout and converts an index to its corresponding index mirrored across the y axis.
At this point, I'm still unsure what purpose the extra 0x77 of allocated space for the board will be used for.  Perhaps a color indicator?
I figured out where the 0x77 comes from!  it's simply H8 - A1!  The purpose is so that one can index by capture vector!  The lowest index will be -0x77, representing a move from H8 to A1, and the highest index will be 0x77, a move from A1 to H8.
Now, I turn my attention to move generation.  There are 3 areas I need to explore:
1. Method for storing and saving moves
1. Move generation
1. Make / Unmake
_Note: Current perft(6) ~ 40 sec_


