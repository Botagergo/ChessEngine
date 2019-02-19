#include <intrin.h>

#include "util.h"

#pragma warning (disable : 4146)

Bitboard Util::verticalFlip(Bitboard bb) {
	return _byteswap_uint64(bb);
}

void Util::clearLSB(Bitboard &bb)
{
	bb &= (bb - 1);
}

Bitboard Util::isolateLSB(Bitboard b)
{
	return b & -b;
}

Square Util::bitScanForward(Bitboard bb)
{
	unsigned long bit;
#ifdef _M_AMD64
	_BitScanForward64(&bit, bb);
#else
	_BitScanForward(&bit, bb);
#endif
	return static_cast<Square>(bit);
}

Square Util::bitScanForwardPop(Bitboard &bb)
{
	Square square = bitScanForward(bb);
	clearLSB(bb);
	return square;
}

Square Util::bitScanReverse(Bitboard bb)
{
	if (bb == 0)
		return NO_SQUARE;

	unsigned long bit;
#ifdef _M_AMD64

	_BitScanReverse64(&bit, bb);
#else
	_BitScanReverse(&bit, bb);
#endif
	return static_cast<Square>(bit);
}

int Util::popCount(Bitboard b) {
	return static_cast<int>(_mm_popcnt_u64(b));
}

File Util::getFile(Square square)
{
	return (File)(square % 8);
}