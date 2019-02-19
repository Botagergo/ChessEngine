#pragma once

#include <vector>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "evaluation.h"
#include "move.h"
#include "util.h"

namespace Move_Gen
{
	//int getScore(const Board & board, const Move & move);

	template <Color toMove, bool quiescence>
	void genMoves(const Board & board, std::vector<Move> & moves)
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

		for (Square from : BitboardIterator<Square>(pawns))
		{
			Bitboard targets = board.attacked(from) & board.occupied(~toMove);
			for (Square to : BitboardIterator<Square>(targets))
			{
				if (SquareBB[to] & backRank(toMove))
				{
					moves.push_back(Move(PAWN, from, to, QUEEN,  FLAG_CAPTURE));
					moves.push_back(Move(PAWN, from, to, ROOK,   FLAG_CAPTURE));
					moves.push_back(Move(PAWN, from, to, BISHOP, FLAG_CAPTURE));
					moves.push_back(Move(PAWN, from, to, KNIGHT, FLAG_CAPTURE));
				}
				else
					moves.push_back(Move(PAWN, from, to, FLAG_CAPTURE));
			}

			if (!quiescence)
			{
				Bitboard targets = pawnSinglePushTargets<toMove>(SquareBB[from], ~board.occupied());

				if (targets)
				{
					Square to = Util::bitScanForward(targets);

					if (SquareBB[to] & backRank(toMove))
					{
						moves.push_back(Move(PAWN, from, to, QUEEN));
						moves.push_back(Move(PAWN, from, to, ROOK));
						moves.push_back(Move(PAWN, from, to, BISHOP));
						moves.push_back(Move(PAWN, from, to, KNIGHT));
					}
					else
						moves.push_back(Move(PAWN, from, to));
				}

				targets = pawnDoublePushTargets<toMove>(SquareBB[from], ~board.occupied());
				if (targets)
				{
					Square to = Util::bitScanForward(targets);
					moves.push_back(Move(PAWN, from, to, FLAG_DOUBLE_PUSH));
				}
			}
		}

		if (board.enPassantTarget() != NO_SQUARE)
		{
			Square to = board.enPassantTarget();
			Bitboard ep_pawns = pawnAttacks<~toMove>(SquareBB[board.enPassantTarget()]) & pawns;

			for (Square from : BitboardIterator<Square>(ep_pawns))
			{
				moves.push_back(Move(PAWN, from, to, FLAG_CAPTURE | FLAG_EN_PASSANT));
			}
		}
	}

	template <PieceType pieceType, Color toMove, bool quiescence>
	void genPieceMoves(const Board &board, std::vector<Move> &moves)
	{
		for (Square from : BitboardIterator<Square>(board.pieces(toMove, pieceType)))
		{
			Bitboard targets = board.attacked(from) & board.occupied(~toMove);

			for (Square to : BitboardIterator<Square>(targets))
			{
				moves.push_back(Move(pieceType, from, to, FLAG_CAPTURE));
			}

			if (!quiescence)
			{
				Bitboard targets = board.attacked(from) & ~board.occupied();

				for (Square to : BitboardIterator<Square>(targets))
				{
					moves.push_back(Move(pieceType, from, to));
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
			moves.push_back(Move::castle(toMove, side));
		}
	}

	template <Color toMove, bool quiescence>
	class MoveGenerator
	{
	public:
		MoveGenerator(const Board & board, const Move * hash_move) : _board(board), _hash_move(hash_move), _stage(HASH_MOVE), _pos(0)
		{
			if (hash_move == nullptr)
				next();
		}

		const Move & curr() const
		{
			if (_stage == HASH_MOVE)
				return *_hash_move;
			else
				return _moves[_pos];
		}

		void next()
		{
			if (_stage == HASH_MOVE)
			{
				genMoves<toMove, quiescence>(_board, _moves);

				std::vector<int> scores;
				scores.resize(_moves.size());

				for (int i = 0; i < _moves.size(); ++i)
				{
					if (_moves[i].isPromotion())
						scores[i] = Evaluation::PieceValue[_moves[i].promotion()];
					else
					{
						PieceType victim = toPieceType(_board.pieceAt(_moves[i].to()));
						PieceType attacker = toPieceType(_board.pieceAt(_moves[i].from()));
						scores[i] = Evaluation::PieceValue[victim] - Evaluation::PieceValue[attacker];
					}
				}

				for (int i = 0; i < (int)_moves.size() - 1; ++i)
				{
					int max = 0;
					for (int j = i; j < _moves.size(); ++j)
					{
						if (scores[j] > scores[max])
							max = j;
					}

					std::swap(scores[i], scores[max]);
					std::swap(_moves[i], _moves[max]);
				}

				if (_hash_move != nullptr)
				{
					auto it = std::find(_moves.begin(), _moves.end(), *_hash_move);
					if (it != _moves.end())
						_moves.erase(it);
				}

				_stage = OTHER;
			}
			else
				++_pos;
		}

		bool end() const
		{
			return _stage != HASH_MOVE && _pos >= _moves.size();
		}

	private:
		enum Stage
		{
			HASH_MOVE,
			OTHER
		};

		const Board & _board;
		const Move *_hash_move;
		int _stage;
		std::vector<Move> _moves;
		int _pos;
	};
}