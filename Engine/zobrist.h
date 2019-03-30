#pragma once

#include "board.h"

namespace Zobrist
{
	void initZobristHashing();
	u64 getBoardHash(const Board & board);

	extern u64 PiecePositionHash[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
	extern u64 BlackMovesHash;
	extern u64 CastlingRightsHash[16];
	extern u64 EnPassantFileHash[FILE_NB];
}