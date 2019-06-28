#pragma once

#include "board.h"
#include "config.h"
#include "movegen.h"

#include <vector>

namespace MoveSelect
{
	int mvvlva(const Board & board, Move move);

	template <Color toMove>
	int pieceSquareEval(Move move)
	{
		return Evaluation::pieceSquareValue<toMove>(move.pieceType(), move.to()).mg
			- Evaluation::pieceSquareValue<toMove>(move.pieceType(), move.from()).mg;
	}

	template <Color toMove, bool quiescence>
	class MoveSelector
	{
	public:
		MoveSelector(const Board & board, Move hash_move = Move(), const std::pair<Move, Move> &killer_moves = std::make_pair(Move(), Move()))
			: _board(board), _hash_move(hash_move), _killer_moves(killer_moves), _pos(0), _curr_hash_move(true)
		{
			if (!_hash_move.isValid())
			{
				MoveGen::genMoves<toMove, quiescence>(_board, _moves, _move_count);
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
				MoveGen::genMoves<toMove, quiescence>(_board, _moves, _move_count);
				_curr_hash_move = false;
				_getScores();
			}
			else
				++_pos;

			if (_pos < _move_count)
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
			for (int i = 0; i < _move_count; ++i)
			{
				if (_moves[i].isPromotion())
					_scores[i] = Evaluation::PieceValue[_moves[i].promotion()].mg * 10;
				else if (_moves[i].isCapture())
				{
					if (quiescence)
						_scores[i] = mvvlva(_board, _moves[i]) * 10;
					else
						_scores[i] = see<toMove>(_board, _moves[i]) * 10;
				}
				else if (!quiescence && (_moves[i] == _killer_moves.first || _moves[i] == _killer_moves.second))
				{
					_scores[i] = SCORE_KILLER;
				}
				else
				{
					_scores[i] = pieceSquareEval<toMove>(_moves[i]);
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
		int _scores[MAX_MOVES];
		int _move_count;
		int _pos;
		bool _curr_hash_move;
	};
}