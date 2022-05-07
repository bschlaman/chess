## Goals
 * improve the performance of my perft
 * explore other board representations

My engine currently uses a 10x12 representation, which is the smallest setup that uses piece offsets with OFFBOARD checks.

### Disecting qperft.c
#### Board Representation
The memory location for the board seems to be the board itself plus capture codes and a delta vector (whatever that is).
```c
#define board      (brd+1)                /* 12 x 16 board: dbl guard band */
#define capt_code  (brd+1+0xBC+0x77)      /* piece type that can reach this*/
#define delta_vec  ((char *) brd+1+0xBC+0xEF+0x77) /* step to bridge certain vector */
```
Ignoring for now that `brd` starts one index earlier than `board`, the board has a length of `0xBC + 0x77 = 0x133` or `188 + 119 = 307`.  The comments in the code indicate that it is a 12 x 16 board representation, but that only comes out to a size of 192.  What's more, following the [0x88 style](https://www.chessprogramming.org/0x88), only the first 185 or 186 squares are useful (depending on where the board is placed within the guard squares).

The next clue is in the `board_init` function.
```c
for(i= -1; i<0xBC; i++) b[i] = (i-0x22)&0x88 ? GUARD : DUMMY;
```
So it appears that the 0xBC portion of the board houses the actual squares.  0x88 shows up here to determine if the square is offboard.  The advantage of an 0x88 board representation is that the 16ths place nibble represents the rank, and the 1s place nibble represents the file.  This means taking the bitwise & operator with a square and 0x88 (1000 1000<sub>2</sub>) will determine if it is OFFBOARD.
There is also an offset of 0x22 here, which indicates that there are two rows of 16 guard squares +2 on the 3rd row.  Using the following code, I've printed the resulting board:
```c
int invertRow(int i){
  // helpful to think of this as:  (top row - i row) + (i - i row)
  //                             (0xB0 - (i-i%0x10)) + (i - (i-i%0x10))
  // get the new row with (0xB0 - (i-i%0x10))
  // then add back in the column with (i - (i-i%0x10))
	return 0xB0 + i - 2 * (i - i % 0x10);
}
for(int i = 0; i < 0xBC + 4; i++){
	if(i && i % 0x10 == 0) printf("\n");
	printf("%c ", (invertRow(i)-0x22)&0x88 ? '-' : 'X');
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

The problem with printing out a board with A1 in the lower left corner is that the for loop will print starting at the top.  I therefore introduce the `invertRow` function that assumes a 12x16 board layout and converts an index to its corresponding index mirrored across the x axis.  Notice also that I've added a `+4` in my iteration as compared to qperft.c; this is simply to avoid 4 missing guard characters (`-`) in the output that would end up in the wrong place anyways due to `invertRow`.  The extra guard charaters on the right side of the board are simply there to allow for the conveniences granted by 0x88 board representations.  Perhaps an idea I'll steal for my own representation!  One open question remains, however: why double guard bands on both sides of the board?  The single left and right guard bands in the 10x12 representation should suffice.
#### Capture Codes
The next 2 lots of 0x77 granted to the board memory addresses are simply H8 - A1.  The purpose is so that one can index by capture vector; that is, each capture from sq1 -> sq2 can be uniquely represented as an index of `capt_code` (order is preserved, so negative indices are required, and the array address space is 15^2).  The lowest index is -0x77, representing a move from H8 to A1, and the highest index is 0x77, a move from A1 to H8.  Beautiful!
To understand the `capt_code` array, I think it's helpful to think of it as completely separate from `board`.  The `board` array ends at 0xBB, with 0xBC (in `board` terms) being the first address of `capt_code`.  But this address is assigned with a 0x77 offset; that is, the first element is accessed like `capt_code[-0x77]`.  `capt_code` takes a sq to sq capture as its index and returns capture codes, which can be used to identify what kinds of pieces can make that capture.  Contact captures are flagged separately from sliding captures; the reason for this will likely become clear later.
When `capt_code` is initialized; only the "elemental" captures are used:
- `C_ORTH`
- `C_DIAG`
- `C_KNIGHT`
- `C_SIDE`
- `C_FORW`
- `C_FDIAG`
- `C_BACKW`
- `C_BDIAG`
"Higher order" capture codes like [ferz](https://en.wikipedia.org/wiki/Ferz) are synthesized from these foundational ones; e.g. `#define C_FERZ    (C_FDIAG|C_BDIAG)`.
#### Piece Representation
There is a pc array of size 4 * 32
#### Move Generation
I will explore the qperft strategy for the following 3 move generation activities:
1. Move serialization
1. Legal move generation
1. Make / Unmake

### My Current Perft Implementation
Perft mode is run by calling my engine with the `-p` flag.  When invoked, the following occurs:
1. FEN is parsed and board is initialized and printed
1. A timer is started using clock\_t clock from time.h
1. perft2 is run.  This function currently resides in eval.c and uses the [chess programming wiki implementation](https://www.chessprogramming.org/Perft)
> I'ts important to understand the difference between perft and perft2: perft2 avoids the last make/unmake call by returning the number of legal moves at depth 1.  I am not yet sure which of these qperft uses, but for my own pride I hope it's using perft2!
  1. Call genLegalMoves
  1. Call makeMove
  1. Recurse
  1. Call undoMove
> In perft2, all three functions are called equally often (gen is called 1 more time than make/undo; think of the first call to gen as the extra one).  In perft, makeMove and undoMove are called more often than genLegalMoves.  This is good because improvements are needed in all 3 functions, and these improvements will be weighted equally in perft2.

At depth 4, the functions are called 9322 (and 9323) times.
#### General improvement ideas
- Avoid accessing memory like `bs -> board`; this is expensive due to potential cache misses
#### genLegalMoves improvements
#### makeMove improvements
#### undoMove improvements

### Performance results
As of 04.05.2022, my peft is about 6.3x slower than qperft.c
On my i7 1.8GHz CPU:
bperft (depth 6): 14.5s
qperft (depth 6):  2.3s
TODO: run compare to results on rpi

### Thoughts as I work
- I have a theory that qperft uses a contiguous segment of memory to store all game data to improve hash performance
- Is there a reason OFFBOARD needs to be -1 in my case, or can it just be a regular enum?
- LastKnight, FirstSlider, etc are probably optimizations for board scanning
- Doesn't seem like first 32 elements of `kind` have been initialized before looping over them on line 178...
- Something worth testing is using separate arrays for pieces, positions, etc. instead of a contiguous chunk of memory impacts performance
- Unused `kind` elements are probably for promotions.  Not sure why the king and two knights are on the left side of these open slots
