#pragma once

#include <cassert>

#include "bitboard.h"
#include "attack_tables.h"
#include "types.h"
#include "util.h"

template <Direction dir>
Bitboard slidingAttacks(Square square, Bitboard occupied)
{
	Bitboard attacks = SlidingAttackTable[dir][square];
	Bitboard blockers = attacks & occupied;

	const bool up = (dir == NORTHWEST || dir == NORTH || dir == NORTHEAST || dir == EAST);

	if (blockers)
	{
		int s = up ? Util::bitScanForward(blockers) : Util::bitScanReverse(blockers);
		attacks ^= SlidingAttackTable[dir][s];
	}

	return attacks;
}

Bitboard pseudoRookAttacks(Square square);
Bitboard pseudoQueenAttacks(Square square);
Bitboard pseudoBishopAttacks(Square square);

template <Color color>
Bitboard pawnSinglePushTargets(Bitboard pawns, Bitboard empty) {
	if (color == WHITE)
		return Util::shift<NORTH>(pawns) & empty;
	else
		return Util::shift<SOUTH>(pawns) & empty;
}

template <Color color>
Bitboard pawnDoublePushTargets(Bitboard pawns, Bitboard empty) {
	Bitboard res = pawnSinglePushTargets<color>(pawns, empty);
	return pawnSinglePushTargets<color>(res, empty) & (color == WHITE ? RankBB[RANK_4] : RankBB[RANK_5]);
}


template <Color color>
Bitboard pawnPushTargets(Bitboard pawns, Bitboard empty)
{
	return pawnSinglePushTargets<color>(pawns, empty)
		| pawnDoublePushTargets<color>(pawns, empty);
}

template <Color color>
Bitboard pawnAttacks(Bitboard pawns) {
	if (color == WHITE)
		return Util::shift<NORTHWEST>(pawns) | Util::shift<NORTHEAST>(pawns);
	else
		return Util::shift<SOUTHWEST>(pawns) | Util::shift<SOUTHEAST>(pawns);
}

Bitboard knightAttacks(Square knight);

Bitboard bishopAttacks(Square bishop, Bitboard occupied);

Bitboard rookAttacks(Square square, Bitboard occupied);

Bitboard queenAttacks(Square square, Bitboard occupied);

Bitboard kingAttacks(Square king);

Bitboard pieceAttacks(Square square, PieceType piece_type, Bitboard occupied);

template <Color color, PieceType pieceType>
Bitboard pieceAttacks(Bitboard pieces, Bitboard occupied)
{
	if (pieceType == PAWN)
		return pawnAttacks<color>(pieces);
	else
	{
		Bitboard attacked = 0;
		for (Square square : BitboardIterator<Square>(pieces))
		{
			attacked |= pieceAttacks<pieceType>(square, occupied);
		}
		return attacked;
	}
}