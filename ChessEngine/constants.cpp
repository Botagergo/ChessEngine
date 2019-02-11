#include "constants.h"
#include "types.h"

Bitboard SquareBB[SQUARE_NB] = {};

void initSquareBB() {
	Bitboard bb = 1;
	for (int i = 0; i < SQUARE_NB; ++i, bb <<= 1)
	{
		SquareBB[i] = bb;
	}
}