#include <intrin.h>

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
		assert(bb != 0);
		unsigned long bit;
#ifdef _M_AMD64
		_BitScanForward64(&bit, bb);
#else
		_BitScanForward(&bit, bb);
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
		assert(bb != 0);
		unsigned long bit;
#ifdef _M_AMD64

		_BitScanReverse64(&bit, bb);
#else
		_BitScanReverse(&bit, bb);
#endif
		return static_cast<Square>(bit);
	}

	int popCount(Bitboard b) {
		return static_cast<int>(_mm_popcnt_u64(b));
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