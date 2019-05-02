#pragma once

#include <vector>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "config.h"
#include "evaluation.h"
#include "move.h"
#include "see.h"
#include "util.h"

using namespace Constants;

namespace MoveGen
{
	template <Color toMove, bool quiescence>
	void genMoves(const Board & board, Move *moves, int &size)
	{
		size = 0;

		genPawnMoves<toMove, quiescence>(board, moves, size);
		genPieceMoves<KNIGHT, toMove, quiescence>(board, moves, size);
		genPieceMoves<BISHOP, toMove, quiescence>(board, moves, size);
		genPieceMoves<ROOK, toMove, quiescence>(board, moves, size);
		genPieceMoves<QUEEN, toMove, quiescence>(board, moves, size);
		genPieceMoves<KING, toMove, quiescence>(board, moves, size);

		if (!quiescence)
		{
			genCastle<toMove, KINGSIDE>(board, moves, size);
			genCastle<toMove, QUEENSIDE>(board, moves, size);
		}
	}

	template <Color toMove, bool quiescence>
	void genPawnMoves(const Board &board, Move *moves, int &size)
	{
		Bitboard pawns = board.pieces(toMove, PAWN);

		for (Square from : BitboardIterator<Square>(pawns))
		{
			Bitboard targets = board.attacked(from) & board.occupied(~toMove);
			for (Square to : BitboardIterator<Square>(targets))
			{
				if (SquareBB[to] & Util::backRank<toMove>())
				{
					moves[size++] = Move(PAWN, from, to, QUEEN,  FLAG_CAPTURE);
					moves[size++] = Move(PAWN, from, to, ROOK,   FLAG_CAPTURE);
					moves[size++] = Move(PAWN, from, to, BISHOP, FLAG_CAPTURE);
					moves[size++] = Move(PAWN, from, to, KNIGHT, FLAG_CAPTURE);
				}
				else
					moves[size++] = Move(PAWN, from, to, FLAG_CAPTURE);
			}

			targets = pawnSinglePushTargets<toMove>(SquareBB[from], ~board.occupied());
			if (quiescence)
				targets &= Util::backRank<toMove>();

			for (Square to : BitboardIterator<Square>(targets))
			{
				if (SquareBB[to] & Util::backRank<toMove>())
				{
					moves[size++] = Move(PAWN, from, to, QUEEN);
					moves[size++] = Move(PAWN, from, to, ROOK);
					moves[size++] = Move(PAWN, from, to, BISHOP);
					moves[size++] = Move(PAWN, from, to, KNIGHT);
				}
				else
					moves[size++] = Move(PAWN, from, to);
			}

			if (!quiescence)
			{
				targets = pawnDoublePushTargets<toMove>(SquareBB[from], ~board.occupied());
				for (Square to : BitboardIterator<Square>(targets))
				{
					moves[size++] = Move(PAWN, from, to, FLAG_DOUBLE_PUSH);
				}
			}
		}

		if (board.enPassantTarget() != NO_SQUARE)
		{
			Square to = board.enPassantTarget();
			Bitboard ep_pawns = pawnAttacks<~toMove>(SquareBB[board.enPassantTarget()]) & pawns;

			for (Square from : BitboardIterator<Square>(ep_pawns))
			{
				moves[size++] = Move(PAWN, from, to, FLAG_CAPTURE | FLAG_EN_PASSANT);
			}
		}
	}

	template <PieceType pieceType, Color toMove, bool quiescence>
	void genPieceMoves(const Board &board, Move *moves, int &size)
	{
		for (Square from : BitboardIterator<Square>(board.pieces(toMove, pieceType)))
		{
			Bitboard targets = board.attacked(from) & board.occupied(~toMove);

			for (Square to : BitboardIterator<Square>(targets))
			{
				moves[size++] = Move(pieceType, from, to, FLAG_CAPTURE);
			}

			if (!quiescence)
			{
				Bitboard targets = board.attacked(from) & ~board.occupied();

				for (Square to : BitboardIterator<Square>(targets))
				{
					moves[size++] = Move(pieceType, from, to);
				}
			}
		};
	}

	template <Color toMove, Side side>
	void genCastle(const Board &board, Move *moves, int &size)
	{
		const static Bitboard CantBeAttacked =
			(toMove == WHITE
				? (side == QUEENSIDE ? SquareBB[C1] | SquareBB[D1] : SquareBB[F1] | SquareBB[G1])
				: (side == QUEENSIDE ? SquareBB[C8] | SquareBB[D8] : SquareBB[F8] | SquareBB[G8]));

		const static Bitboard CantBeOccupied = CantBeAttacked |
			(toMove == WHITE
				? (side == QUEENSIDE ? SquareBB[B1] : 0Ull)
				: (side == QUEENSIDE ? SquareBB[B8] : 0Ull));

		if (board.canCastle(toMove, side)
			&& !board.isInCheck(toMove)
			&& !(board.attacked(~toMove) & CantBeAttacked)
			&& !(board.occupied() & CantBeOccupied))
		{
			moves[size++] = Move::castle(toMove, side);
		}
	}
}