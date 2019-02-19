#pragma once

#pragma warning( disable : 4715)

#include <cstdlib>

#include "bitboard.h"
#include "constants.h"
#include "types.h"

namespace Util
{
	template <Direction dir>
	Bitboard shift(Bitboard b)
	{
		static_assert(dir != DIRECTION_NB);

		switch (dir)
		{
		case NORTH:
			return (b << 8);
		case NORTHEAST:
			return (b << 9) & NotAFileBB;
		case EAST:
			return (b << 1) & NotAFileBB;
		case SOUTHEAST:
			return (b >> 7) & NotAFileBB;
		case SOUTH:
			return (b >> 8);
		case SOUTHWEST:
			return (b >> 9) & NotHFileBB;
		case WEST:
			return (b >> 1) & NotHFileBB;
		case NORTHWEST:
			return (b << 7) & NotHFileBB;
		}
	}

	Bitboard verticalFlip(Bitboard bb);

	void clearLSB(Bitboard &bb);

	Bitboard isolateLSB(Bitboard b);

	Square bitScanForward(Bitboard bb);

	Square bitScanForwardPop(Bitboard &b);

	Square bitScanReverse(Bitboard bb);

	int popCount(Bitboard b);

	File getFile(Square square);

	template <Color color>
	Square relativeSquare(Square square)
	{
		return Square(square ^ (56 * color));
	}
}