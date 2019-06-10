#include "attacks.h"
#include "board.h"
#include "see.h"

Bitboard _getAttackers(const Board & board, Color to_move, Bitboard occupied, Square square)
{
	Bitboard res = 0;

	if (to_move == WHITE)
		res |= Attacks::pawnAttacks<BLACK>(Constants::SquareBB[square]) & board.pieces(WHITE, PAWN);
	else
		res |= Attacks::pawnAttacks<WHITE>(Constants::SquareBB[square]) & board.pieces(BLACK, PAWN);

	res |= Attacks::knightAttacks(square) & board.pieces(to_move, KNIGHT);
	res |= Attacks::bishopAttacks(square, occupied) & board.pieces(to_move, BISHOP);
	res |= Attacks::rookAttacks(square, occupied) & board.pieces(to_move, ROOK);
	res |= Attacks::queenAttacks(square, occupied) & board.pieces(to_move, QUEEN);
	res |= Attacks::kingAttacks(square) & board.pieces(to_move, KING);

	return res;
}

Square _getLeastValuableAttacker(const Board & board, Color to_move, Bitboard attackers)
{
	for (PieceType piece_type = PAWN; piece_type < PIECE_TYPE_NB; ++piece_type)
	{
		Bitboard pieces = board.pieces(to_move, piece_type) & attackers;
		if (pieces)
			return Util::bitScanForward(pieces);
	}

	return NO_SQUARE;
}