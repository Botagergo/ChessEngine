#pragma once

#include "board.h"
#include "types.h"

namespace MoveGen
{
	template <PieceType pieceType, Color toMove, bool quiescence>
	void genPieceMoves(const Board & board, Bitboard targets, std::vector<Move> & moves)
	{
		Bitboard to = targets & occupied_by_enemy
			for (Square square : BitboardIterator<Square>(pieces))
			{
				moves.push_back(Move)
			}
	}

	template<Color toMove, bool quiescence>
	void gen_legal_moves(const Board & board, std::vector<Move> & moves)
	{
		int check_count = 0;
		Bitboard attacked = 0;
		bool is_checking[5] = { 0 };
		Bitboard attacks[5];

		attacks[PAWN] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());
		attacks[KNIGHT] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());
		attacks[BISHOP] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());
		attacks[ROOK] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());
		attacks[QUEEN] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());
		attacks[KING] = pieceAttacks<~toMove, PAWN>(board.pieces(~toMove, PAWN), board.occupied());

		for (PieceType piece_type : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN})
		{
			if (attacks[piece_type] & board.pieces(toMove, KING))
			{
				is_checking[piece_type] = true;
				++check_count;
			}

			attacked |= attacks;
		}

		if (check_count == 0)
		{

		}
		else if (check_count == 1)
		{
			Bitboard targets = 0;

			genPieceMoves<KING, toMove, quiescence>(board, ~attacked, moves);
			genPieceMoves<PAWN, toMove, quiescence>(board, targets, moves);
			genPieceMoves<KNIGHT, toMove, quiescence>(board, targets, moves);
			genPieceMoves<BISHOP, toMove, quiescence>(board, targets, moves);
			genPieceMoves<ROOK, toMove, quiescence>(board, targets, moves);
			genPieceMoves<QUEEN, toMove, quiescence>(board, targets, moves);
		}
		else
		{
			genPieceMoves<KING, toMove, quiescence>(board, ~attacked, moves);
		}

	}
}

Bitboard clearNextBit(Direction dir, Bitboard &bitboard)
{
	return 0;
}

Bitboard pinnedPiece(Square square, Direction dir, Bitboard occupied, Bitboard king)
{
	if (!SlidingAttackTable[dir][square] & king)
		return 0;

	Bitboard bitboard = SlidingAttackTable[dir][square] & occupied

		Bitboard pinned = clearNextBit(dir, )
}

Bitboard pinnedPieces(const Board & board)
{
	for (PieceType piece_type : {BISHOP, ROOK, QUEEN})
	{

	}
}