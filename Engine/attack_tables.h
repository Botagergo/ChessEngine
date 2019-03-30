#pragma once

#include "bitboard.h"
#include "constants.h"
#include "types.h"

extern Bitboard KnightAttackTable[SQUARE_NB];
extern Bitboard KingAttackTable[SQUARE_NB];
extern Bitboard SlidingAttackTable[DIRECTION_NB][SQUARE_NB];
extern Bitboard ObstructedTable[SQUARE_NB][SQUARE_NB];

void initAttackTables();
void initObstructedTable();