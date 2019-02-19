#pragma once

#include "board.h"

namespace Zobrist
{
	void initZobristHashing();
	unsigned long long getBoardHash(const Board & board);

	extern unsigned long long PiecePositionHash[COLOR_NB][PIECE_NB][SQUARE_NB];
	extern unsigned long long BlackMovesHash;
	extern unsigned long long CastlingRightsHash[16];
	extern unsigned long long EnPassantFileHash[FILE_NB];
}