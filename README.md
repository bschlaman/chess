## Goals
 * improve the performance of my perft
 * explore other board representations

My engine currently uses a 10x12 representation, which is the smallest setup that uses piece offsets with OFFBOARD checks

## Disecting qperft
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
So it appears that the 0xBC portion of the board houses the actual squares.
