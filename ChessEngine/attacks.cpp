#include "attacks.h"

Bitboard knightAttacks(Square knight)
{
	return KnightAttackTable[knight];
}

Bitboard bishopAttacks(Square bishop, Bitboard occupied)
{
	return slidingAttacks<NORTHEAST>(bishop, occupied)
		| slidingAttacks<SOUTHEAST>(bishop, occupied)
		| slidingAttacks<SOUTHWEST>(bishop, occupied)
		| slidingAttacks<NORTHWEST>(bishop, occupied);
}

Bitboard rookAttacks(Square rook, Bitboard occupied)
{
	return slidingAttacks<NORTH>(rook, occupied)
		| slidingAttacks<EAST>(rook, occupied)
		| slidingAttacks<SOUTH>(rook, occupied)
		| slidingAttacks<WEST>(rook, occupied);
}

Bitboard queenAttacks(Square queen, Bitboard occupied)
{
	return bishopAttacks(queen, occupied)
		| rookAttacks(queen, occupied);
}


Bitboard kingAttacks(Square king)
{
	return KingAttackTable[king];
}

Bitboard pieceAttacks(Square square, Piece piece_type, Bitboard occupied)
{
	switch (piece_type)
	{
	case KNIGHT:
		return knightAttacks(square);
	case BISHOP:
		return bishopAttacks(square, occupied);
	case ROOK:
		return rookAttacks(square, occupied);
	case QUEEN:
		return queenAttacks(square, occupied);
	case KING:
		return kingAttacks(square);
	default:
		assert(0 == 1);
	}
}