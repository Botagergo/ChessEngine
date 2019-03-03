#pragma once

#include <vector>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "evaluation.h"
#include "move.h"
#include "see.h"
#include "util.h"

using namespace Constants;

namespace MoveGen
{
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
				if (SquareBB[to] & Util::backRank<toMove>())
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

					if (SquareBB[to] & Util::backRank<toMove>())
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

	int mvvlva(const Board & board, Move move);

	template <Color toMove>
	int pieceSquareEval(const Board & board, Move move)
	{
		return Evaluation::pieceSquareValue<toMove>(move.pieceType(), move.to()).mg
			- Evaluation::pieceSquareValue<toMove>(move.pieceType(), move.from()).mg;
	}

	template <Color toMove, bool quiescence>
	class MoveGenerator
	{
	public:
		MoveGenerator(const Board & board, const Move hash_move) : _board(board), _hash_move(hash_move), _pos(0)
		{
			_moves.reserve(50);
			if (!_hash_move.isValid())
			{
				genMoves<toMove, quiescence>(_board, _moves);
				_getScores();
				_selectNext();
			}
		}

		Move curr() const
		{
			if (_hash_move.isValid())
				return _hash_move;
			else
				return _moves[_pos];
		}

		void next()
		{
			if (_hash_move.isValid())
			{
				genMoves<toMove, quiescence>(_board, _moves);
				_moves.erase(std::find(_moves.begin(), _moves.end(), _hash_move));
				_hash_move = Move();
				_getScores();
			}
			else
				++_pos;

			if(_pos < _moves.size())
				_selectNext();
		}

		bool end()
		{
			return !_hash_move.isValid() && _pos >= _moves.size()/*
				|| quiescence && _scores[_pos] < 0)*/;
		}

	private:
		void _getScores()
		{
			_scores.resize(_moves.size());
			for (int i = 0; i < _moves.size(); ++i)
			{
				if (_moves[i].isPromotion())
					_scores[i] = Evaluation::PieceValue[_moves[i].promotion()].mg;
				else if (_moves[i].isCapture())
				{
					if (quiescence)
						_scores[i] = mvvlva(_board, _moves[i]);
					else
						_scores[i] = see<toMove>(_board, _moves[i]);
				}
				else
				{
					_scores[i] = pieceSquareEval<toMove>(_board, _moves[i]);
				}
			}
		}

		void _selectNext()
		{
			int max = _pos;
			for (int i = _pos; i < _moves.size(); ++i)
			{
				if (_scores[i] > _scores[max])
					max = i;
			}

			if (max != _pos)
			{
				std::swap(_scores[_pos], _scores[max]);
				std::swap(_moves[_pos], _moves[max]);
			}
		}

		const Board & _board;
		Move _hash_move;
		std::vector<Move> _moves;
		std::vector<int> _scores;
		int _pos;
	};
}