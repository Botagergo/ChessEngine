#pragma once

#include "board.h"
#include "piece_square_table.h"
#include "types.h"

namespace Evaluation
{
	const static int PieceValue[PIECE_TYPE_NB] = { 100, 300, 320, 500, 900, 0 };

	template <Color color>
	Score evaluate(const Board & board)
	{
		Score score;

		for (PieceType piece_type : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN})
		{
			score += PieceValue[piece_type] * Util::popCount(board.pieces(color, piece_type));
			score -= PieceValue[piece_type] * Util::popCount(board.pieces(~color, piece_type));
		}

		for (PieceType piece_type : PieceTypes)
		{
			for (Square square : BitboardIterator<Square>(board.pieces(color, piece_type)))
			{
				score += pieceSquareValue<color>(piece_type, square);
			}

			for (Square square : BitboardIterator<Square>(board.pieces(~color, piece_type)))
			{
				score -= pieceSquareValue<~color>(piece_type, square);
			}
		}

		return score;
	}
}