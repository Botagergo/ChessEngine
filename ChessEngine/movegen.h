#pragma once

#include <vector>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "move.h"
#include "util.h"

namespace Move_Gen
{
	template <Color toMove, bool quiescence>
	void genMoves(const Board &board, std::vector<Move> &moves)
	{
		genPawnMoves<toMove, quiescence>(board, moves);
		genPieceMoves<KNIGHT, toMove, quiescence>(board, moves);
		genPieceMoves<BISHOP, toMove, quiescence>(board, moves);
		genPieceMoves<ROOK, toMove, quiescence>(board, moves);
		genPieceMoves<QUEEN, toMove, quiescence>(board, moves);
		genPieceMoves<KING, toMove, quiescence>(board, moves);

		if (!quiescence)
		{
			genCastle<toMove, KINGSIDE>(board, moves);
			genCastle<toMove, QUEENSIDE>(board, moves);
		}
	}

	template <Color toMove, bool quiescence>
	void genPawnMoves(const Board &board, std::vector<Move> &moves)
	{
		Bitboard pawns = board.pieces(toMove, PAWN);
		Move move(PAWN, NO_SQUARE, NO_SQUARE);

		for (Square from : BitboardIterator<Square>(pawns))
		{
			move.from = from;
			move.flags = CAPTURE;

			Bitboard targets = board.attacked(from) & board.occupied(~toMove);
			for (Square to : BitboardIterator<Square>(targets))
			{
				move.to = to;

				if (SquareBB[to] & backRank(toMove))
					genPromotions(move, moves);
				else
					moves.push_back(move);
			}

			if (!quiescence)
			{
				move.flags = 0;
				Bitboard targets = pawnSinglePushTargets<toMove>(SquareBB[from], ~board.occupied());

				if (targets)
				{
					move.to = Util::bitScanForward(targets);

					if (SquareBB[move.to] & backRank(toMove))
						genPromotions(move, moves);
					else
						moves.push_back(move);
				}

				targets = pawnDoublePushTargets<toMove>(SquareBB[from], ~board.occupied());
				if (targets)
				{
					move.to = Util::bitScanForward(targets);
					move.flags = DOUBLE_PUSH;
					moves.push_back(move);
				}
			}
		}

		if (board.enPassantTarget() != NO_SQUARE)
		{
			move.to = board.enPassantTarget();
			move.flags = CAPTURE | EN_PASSANT;
			Bitboard ep_pawns = pawnAttacks<~toMove>(SquareBB[board.enPassantTarget()]) & pawns;

			for (Square from : BitboardIterator<Square>(ep_pawns))
			{
				move.from = from;
				moves.push_back(move);
			}
		}
	}

	void genPromotions(Move move, std::vector<Move> &moves)
	{
		for (Piece piece : {KNIGHT, BISHOP, ROOK, QUEEN})
		{
			move.promotion = piece;
			moves.push_back(move);
		}

		move.promotion = NO_PIECE;
	}

	template <Piece pieceType, Color toMove, bool quiescence>
	void genPieceMoves(const Board &board, std::vector<Move> &moves)
	{
		Move move(pieceType, NO_SQUARE, NO_SQUARE);

		for (Square from : BitboardIterator<Square>(board.pieces(toMove, pieceType)))
		{
			Bitboard targets = board.attacked(from) & board.occupied(~toMove);

			move.from = from;
			move.flags = CAPTURE;

			for (Square to : BitboardIterator<Square>(targets))
			{
				move.to = to;
				moves.push_back(move);
			}

			if (!quiescence)
			{
				Bitboard targets = board.attacked(from) & ~board.occupied();

				move.flags = 0;

				for (Square to : BitboardIterator<Square>(targets))
				{
					move.to = to;
					moves.push_back(move);
				}
			}
		};
	}

	template <Color toMove, Side side>
	void genCastle(const Board &board, std::vector<Move> &moves)
	{
		const static Bitboard CantBeAttacked =
			(toMove == WHITE
				? (side == QUEENSIDE ? SquareBB[C1] | SquareBB[D1] : SquareBB[F1] | SquareBB[G1])
				: (side == QUEENSIDE ? SquareBB[C8] | SquareBB[D8] : SquareBB[F8] | SquareBB[G8]));

		const static Bitboard CantBeOccupied = CantBeAttacked |
			(toMove == WHITE
				? (side == QUEENSIDE ? SquareBB[B1] : C64(0))
				: (side == QUEENSIDE ? SquareBB[B8] : C64(0)));

		if (board.canCastle(toMove, side)
			&& !board.isInCheck(toMove)
			&& !(board.attacked(~toMove) & CantBeAttacked)
			&& !(board.occupied() & CantBeOccupied))
		{
			moves.push_back(Move(KING, NO_SQUARE, NO_SQUARE, side == KINGSIDE ? KINGSIDE_CASTLE : QUEENSIDE_CASTLE));
		}
	}
}