#pragma once

#pragma warning( disable : 4715)

#include <assert.h>
#include <cstdlib>

#include "config.h"
#include "constants.h"
#include "types.h"

namespace Util
{
	template <Direction dir>
	Bitboard shift(Bitboard b)
	{
		ASSERT(dir != DIRECTION_NB);

		switch (dir)
		{
		case NORTH:
			return (b << 8);
		case NORTHEAST:
			return (b << 9) & Constants::NotAFileBB;
		case EAST:
			return (b << 1) & Constants::NotAFileBB;
		case SOUTHEAST:
			return (b >> 7) & Constants::NotAFileBB;
		case SOUTH:
			return (b >> 8);
		case SOUTHWEST:
			return (b >> 9) & Constants::NotHFileBB;
		case WEST:
			return (b >> 1) & Constants::NotHFileBB;
		case NORTHWEST:
			return (b << 7) & Constants::NotHFileBB;
		}
	}

	Bitboard verticalFlip(Bitboard bb);

	void clearLSB(Bitboard &bb);

	Bitboard getLSB(Bitboard bb);

	Square bitScanForward(Bitboard bb);

	Square bitScanForwardPop(Bitboard &b);

	Square bitScanReverse(Bitboard bb);

	int popCount(Bitboard b);

	File getFile(Square square);

	Rank getRank(Square square);

	Bitboard northFill(Bitboard bb);

	template <Color color>
	constexpr Rank relativeRank(Rank rank)
	{
		return Rank(rank ^ (color * 7));
	}

	template <Color color, Rank rank>
	constexpr Rank relativeRank()
	{
		return relativeRank<color>(rank);
	}

	template <Color color>
	constexpr Square relativeSquare(Square square)
	{
		return Square(square ^ (56 * color));
	}

	template <Color color, Direction dir>
	constexpr Direction relativeDirection()
	{
		if (color == WHITE)
			return dir;

		switch (dir)
		{
		case EAST:
		case WEST:
			return dir;
		case NORTH:
			return SOUTH;
		case NORTHEAST:
			return SOUTHEAST;
		case SOUTHEAST:
			return NORTHEAST;
		case SOUTHWEST:
			return NORTHWEST;
		case NORTHWEST:
			return SOUTHWEST;
		default:
			ASSERT(false);
		}
	}

	template <Color color>
	constexpr Direction forwardDirection()
	{
		return relativeDirection<color, NORTH>();
	}

	template <Color color>
	constexpr Bitboard backRank()
	{
		return Constants::RankBB[relativeRank<color, RANK_8>()];
	}
}