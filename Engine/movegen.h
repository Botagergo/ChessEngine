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

			if (!quiescence)
			{
				Bitboard targets = pawnSinglePushTargets<toMove>(SquareBB[from], ~board.occupied());

				if (targets)
				{
					Square to = Util::bitScanForward(targets);

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

				targets = pawnDoublePushTargets<toMove>(SquareBB[from], ~board.occupied());
				if (targets)
				{
					Square to = Util::bitScanForward(targets);
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
		MoveGenerator(const Board & board, Move hash_move = Move(), const std::pair<Move, Move> &killer_moves = std::make_pair(Move(), Move()))
			: _board(board), _hash_move(hash_move), _killer_moves(killer_moves), _pos(0), _curr_hash_move(true)
		{
			if (!_hash_move.isValid())
			{
				genMoves<toMove, quiescence>(_board, _moves, _move_count);
				_getScores();
				_selectNext();
				_curr_hash_move = false;
			}
		}

		Move curr() const
		{
			if (_curr_hash_move)
				return _hash_move;
			else
				return _moves[_pos];
		}

		void next()
		{
			if (_curr_hash_move)
			{
				genMoves<toMove, quiescence>(_board, _moves, _move_count);
				_curr_hash_move = false;
				_getScores();
			}
			else
				++_pos;

			if(_pos < _move_count)
				_selectNext();

			if (_moves[_pos] == _hash_move)
				next();
		}

		bool end()
		{
			return !_curr_hash_move && _pos >= _move_count;
		}

	private:
		void _getScores()
		{
			_scores.resize(_move_count);
			for (int i = 0; i < _move_count; ++i)
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
				else if (!quiescence && (_moves[i] == _killer_moves.first || _moves[i] == _killer_moves.second))
				{
					_scores[i] = SCORE_KILLER;
				}
				else
				{
					_scores[i] = -10;
				}
			}
		}

		void _selectNext()
		{
			int max = _pos;
			for (int i = _pos; i < _move_count; ++i)
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
		const std::pair<Move, Move> &_killer_moves;
		Move _moves[MAX_MOVES];
		int _move_count;
		std::vector<int> _scores;
		int _pos;
		bool _curr_hash_move;
	};
}