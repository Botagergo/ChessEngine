#include "attacks.h"
#include "board.h"
#include "see.h"

Bitboard _getAttackers(const Board & board, Color to_move, Bitboard occupied, Square square)
{
	Bitboard res = 0;

	if (to_move == WHITE)
		res |= pawnAttacks<BLACK>(Constants::SquareBB[square]) & board.pieces(WHITE, PAWN);
	else
		res |= pawnAttacks<WHITE>(Constants::SquareBB[square]) & board.pieces(BLACK, PAWN);

	res |= knightAttacks(square) & board.pieces(to_move, KNIGHT);
	res |= bishopAttacks(square, occupied) & board.pieces(to_move, BISHOP);
	res |= rookAttacks(square, occupied) & board.pieces(to_move, ROOK);
	res |= queenAttacks(square, occupied) & board.pieces(to_move, QUEEN);
	res |= kingAttacks(square) & board.pieces(to_move, KING);

	return res;
}

Square _getLeastValuableAttacker(const Board & board, Color to_move, Bitboard attackers)
{
	for (PieceType piece_type : PieceTypes)
	{
		Bitboard pieces = board.pieces(to_move, piece_type) & attackers;
		if (pieces)
			return Util::bitScanForward(pieces);
	}

	return NO_SQUARE;
}