#include <intrin.h>
#include <algorithm>

#include "config.h"
#include "util.h"

#pragma warning (disable : 4146)

namespace Util
{
	Bitboard verticalFlip(Bitboard bb) {
		return _byteswap_uint64(bb);
	}

	void clearLSB(Bitboard &bb)
	{
		bb &= (bb - 1);
	}

	Bitboard getLSB(Bitboard b)
	{
		return b & -b;
	}

	Square bitScanForward(Bitboard bb)
	{
		ASSERT(bb != 0);
		unsigned long bit;
#ifdef _M_AMD64
		_BitScanForward64(&bit, bb);
#else
		_BitScanForward(&bit, bb & ((1l << 32) - 1)) || _BitScanForward(&bit, bb >> 32);
#endif
		return static_cast<Square>(bit);
	}

	Square bitScanForwardPop(Bitboard &bb)
	{
		Square square = bitScanForward(bb);
		clearLSB(bb);
		return square;
	}

	Square bitScanReverse(Bitboard bb)
	{
		ASSERT(bb != 0);
		unsigned long bit;
#ifdef _M_AMD64

		_BitScanReverse64(&bit, bb);
#else
		_BitScanReverse(&bit, bb >> 32) || _BitScanReverse(&bit, bb & ((1l << 32) - 1));
#endif
		return static_cast<Square>(bit);
	}

	int popCount(Bitboard bb) {
#ifdef _M_AMD64
		return static_cast<int>(_mm_popcnt_u64(bb));
#else
		return static_cast<int>(_mm_popcnt_u32(bb & ((1l << 32) - 1)) + _mm_popcnt_u32(bb >> 32));
#endif

	}

	File getFile(Square square)
	{
		return (File)(square % 8);
	}

	Rank getRank(Square square)
	{
		return Rank(square >> 3);
	}

	Bitboard northFill(Bitboard bb)
	{
		bb |= bb >> 8;
		bb |= bb >> 16;
		bb |= bb >> 32;
		return bb;
	}
}