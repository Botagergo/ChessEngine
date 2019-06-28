#pragma once

#include <chrono>
#include <thread>
#include <vector>

#include "board.h"
#include "config.h"
#include "evaluation.h"
#include "evaluation_table.h"
#include "movegen.h"
#include "moveselect.h"
#include "transposition_table.h"
#include "types.h"

namespace Search
{
	class Search
	{
	public:
		Search();

		struct Stats
		{
			Stats() { memset(this, 0, sizeof(*this)); }

			u64 alpha_beta_nodes;
			u64 quiescence_nodes;
			u64 alpha_beta_cutoffs;
			u64 quiescence_cutoffs;
			u64 hash_move_cutoffs;
			u64 killer_move_cutoffs;
			u64 hash_score_returned;
			u64 pv_search_research_count;
			float avg_searched_moves;

			u64 _move_gen_count;
			u64 _searched_moves_sum;
		} stats;

		void startSearch(const Board& board);
		void search(const Board& board, Move* bestMove = nullptr, bool quiet = false);

		void stopSearch();

		bool hasClock(Color color);
		std::chrono::milliseconds getClock(Color color);
		void setClock(Color color, std::chrono::milliseconds clock);
		void unsetClock(Color color);

		bool hasMaxDepth();
		int getMaxDepth();
		void setMaxDepth(int maxdepth);
		void unsetMaxDepth();

		bool hasMoveTime();
		std::chrono::milliseconds getMoveTime();
		void setMoveTime(std::chrono::milliseconds movetime);
		void unsetMoveTime();

		bool isInfinite();
		void setInfinite(bool infinite);

		bool getPonder();
		void setPonder(bool ponder);

		// The size of the hash table in megabytes
		size_t getHashSize();
		void setHashSize(size_t size);

		const Stats& getStats();

		void (*onBestMove)(Move move, Move ponder_move);
		void (*onPrincipalVariation)(const std::vector<Move>& pv, int depth, int score, bool mate);
		void (*onCurrentMove)(Move move, int pos);
		void (*onNodeInfo)(u64 node_count, u64 nodes_per_sec);
		void (*onHashfull)(int permill);
		void (*onStats)(const Stats &stats);

	private:
		template <Color toMove, bool pvNode, bool nullMoveAllowed>
		int _alphaBeta(const Board& board, int alpha, int beta, int depthleft, int ply, std::vector<Move>* pv);

		template <Color toMove>
		int _quiescence(const Board& board, int alpha, int beta);

		void _updateNodesPerSec();
		void _infoThread();

		bool _isRepetition(u64 hash, int ply);
		bool _isMateScore(int score);
		int _razorMargin(int depth);

		bool _passedMaxdepth();
		bool _timeOut();
		bool _shouldStopSearch();

		void _resizeHashTable(size_t size);

		TranspositionTable _transposition_table;
		EvaluationTable _evaluation_table;

		size_t _hash_size;

		std::pair<Move, Move> _killer_moves[MAX_DEPTH];
		std::array<u64, MAX_DEPTH> _history;

		std::thread _search_thrd;

		bool _has_clock[COLOR_NB];
		std::chrono::milliseconds _clock[COLOR_NB];

		int _maxdepth;
		bool _has_maxdepth;

		// The exact length of the search
		bool _has_movetime;
		std::chrono::milliseconds _movetime;

		// If true, the search doesn't stop unless manually terminated
		bool _infinite;

		bool _stop;
		bool _ponder;

		// This is needed when pondering, when the search is infinite. If the opponent makes the expected move,
		// we have to switch from pondering to normal search, and consequently stop searching if maxdepth is passed
		bool _passed_maxdepth;

		// Disable stopping the search by command of by timeout on the first stage of the iterative deepening
		bool _can_stop_search;

		struct
		{
			u64 last_node_count;
			u64 node_count;
			std::chrono::steady_clock::time_point last_time;
			std::chrono::milliseconds elapsed_time;
			int nodes_per_sec;
			bool has_time_left;
			std::chrono::milliseconds time_left;
		} Timer;

		// Search parameters
		const static int _RFutility_Depth;
		const static int _RFutility_Param;
		const static int _IID_Depth;
		const static int _IID_Reduction;
		const static double _ThinkTimePercentage;
	};

	template <Color toMove>
	int Search::_quiescence(const Board& board, int alpha, int beta)
	{
		++stats.quiescence_nodes;

		int stand_pat = _evaluation_table.probe(board.hash(), alpha, beta);
		if (stand_pat != SCORE_INVALID)
		{
			stand_pat = Evaluation::evaluate<toMove>(board);
			_evaluation_table.insert(board.hash(), stand_pat, EXACT);
		}

		if (stand_pat >= beta)
			return beta;

		if (stand_pat < alpha - 1000)
			return alpha;

		if (alpha < stand_pat)
			alpha = stand_pat;

		int alpha_orig = alpha;

		MoveSelect::MoveSelector<toMove, true> mg(board, Move());
		for (int i = 1; !mg.end(); ++i, mg.next())
		{
			assert(!mg.curr().isQuiet());

			// Delta pruning
			if (!mg.curr().isPromotion() && stand_pat + Evaluation::PieceValue[toPieceType(board.pieceAt(mg.curr().to()))].mg + 200 < alpha)
				continue;

			Board board_copy = board;
			if (!board_copy.makeMove(mg.curr()))
				continue;

			int score = -_quiescence<~toMove>(board_copy, -beta, -alpha);
			if (score >= beta)
			{
				++stats.quiescence_cutoffs;
				_evaluation_table.insert(board.hash(), score, LOWER_BOUND);
				return beta;
			}
			if (score > alpha)
				alpha = score;
		}

		if (alpha > alpha_orig)
			_evaluation_table.insert(board.hash(), alpha, EXACT);
		else
			_evaluation_table.insert(board.hash(), alpha, UPPER_BOUND);

		return alpha;
	}

	template <Color toMove, bool pvNode, bool nullMoveAllowed>
	int Search::_alphaBeta(const Board & board, int alpha, int beta, int depthleft, int ply, std::vector<Move> * pv)
	{
		ASSERT(depthleft >= 0);

		if (_shouldStopSearch())
			return SCORE_INVALID;

		++stats.alpha_beta_nodes;

		if (stats.alpha_beta_nodes % 30000 == 0)
		{
			if (onNodeInfo)
				onNodeInfo(Timer.node_count, Timer.nodes_per_sec);
			if (onHashfull)
				onHashfull((int)(_transposition_table.usage() * 1000));
		}

		_history[ply] = board.hash();

		Move hash_move = Move();

		// Transposition table lookup
		auto hash = _transposition_table.probe(board.hash(), depthleft, alpha, beta);
		if (hash.first == alpha || hash.first == beta)
		{
			ASSERT(ply);
			++stats.hash_score_returned;
			return hash.first;
		}
		else if (hash.second.isValid())
		{
			ASSERT(board.pieceAt(hash.second.from()) != NO_PIECE);
			hash_move = hash.second;
		}

		if (depthleft == 0)
			return _quiescence<toMove>(board, alpha, beta);

		int eval = Evaluation::evaluate<toMove>(board);

		// Reverse futility pruning
		if (!pvNode
			&& depthleft <= _RFutility_Depth
			&& !board.isInCheck(toMove)
			&& std::abs(alpha) < SCORE_MIN_MATE && std::abs(beta) < SCORE_MIN_MATE
			&& eval - _RFutility_Param * depthleft >= beta
			&& board.allowNullMove())
		{
			return beta;
		}

		// Dynamic null move pruning
		if (nullMoveAllowed
			&& depthleft >= 4
			&& !board.isInCheck(toMove)
			&& board.allowNullMove())
		{
			Board board_copy = board;
			board_copy.makeMove(Move::nullMove());

			int score = -_alphaBeta<~toMove, false, false>(board_copy, -beta, -beta + 1, depthleft - 3, ply + 1, nullptr);
			if (score >= beta)
				return beta;
		}

		// Razoring
		int margin = _razorMargin(depthleft);

		if (!board.isInCheck(toMove)
			&& depthleft <= 3
			&& !pvNode
			&& eval + margin <= alpha)
		{
			int res = _quiescence<toMove>(board, alpha - margin, beta - margin);
			if (res + margin <= alpha)
				depthleft--;

			if (depthleft <= 0)
				return alpha;
		}

		//// Internal iterative deepening
		//if (depthleft >= Params.IID_Depth && !hash_move.isValid() && pvNode)
		//{
		//	Board board_copy = board;
		//	alphaBeta<toMove, true, false>(board_copy, alpha, beta, depthleft - Params.IID_Reduction, ply + 1, nullptr);
		//	hash_move = searchInfo.transposition_table.probe(board.hash(), 0, alpha, beta).second;
		//}

		// Depth extension
		if (board.isInCheck(toMove))
		{
			depthleft++;
		}

		int curr_pos = 0;
		int searched_moves = 0;
		int alpha_orig = alpha;
		int score;
		std::vector<Move> new_pv;
		std::vector<Move> * new_pv_ptr = nullptr;

		if (pvNode)
		{
			new_pv.resize(MAX_DEPTH);
			new_pv_ptr = &new_pv;
		}

		MoveSelect::MoveSelector<toMove, false> mg(board, hash_move, _killer_moves[ply]);
		stats._move_gen_count++;

		for(int i = 1; !mg.end(); ++i, mg.next())
		{
			Board board_copy = board;

			if (!board_copy.makeMove(mg.curr()))
				continue;

			if (_isRepetition(board_copy.hash(), ply))
			{
				score = 0;
				goto SearchEnd;
			}

			if (onCurrentMove && ply == 0)
				onCurrentMove(mg.curr(), searched_moves + 1);

			if (searched_moves < 1)
				score = -_alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
			else
			{
				score = -_alphaBeta<~toMove, false, true>(board_copy, -(alpha + 1), -alpha, depthleft - 1, ply + 1, nullptr);
				if (score > alpha)
				{
					++stats.pv_search_research_count;
					score = -_alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
				}
			}

			if (score == -SCORE_INVALID)
				return SCORE_INVALID;

SearchEnd:

			++searched_moves;
			stats._searched_moves_sum++;

			if (score > alpha && score < beta)
			{
				if (SCORE_MIN_MATE <= score && score <= SCORE_MAX_MATE)
					score -= 1;
				else if (-SCORE_MAX_MATE <= score && score <= -SCORE_MIN_MATE)
					score += 1;
			}

			if (score >= beta)
			{
				++stats.alpha_beta_cutoffs;

				if (mg.curr() == hash_move)
					++stats.hash_move_cutoffs;

				if (mg.curr().isQuiet())
				{
					if (mg.curr() == _killer_moves[ply].first || mg.curr() == _killer_moves[ply].second)
						++stats.killer_move_cutoffs;

					_killer_moves[ply].second = _killer_moves[ply].first;
					_killer_moves[ply].first = mg.curr();
				}

				assert(mg.curr() != Move());
				_transposition_table.insert(board.hash(), depthleft, beta, mg.curr(), LOWER_BOUND);
				return beta;
			}

			if (score > alpha)
			{
				alpha = score;

				if (pv)
				{
					assert(new_pv_ptr != nullptr && pv != nullptr);

					pv->resize(MAX_DEPTH);

					(*pv)[0] = mg.curr();
					std::copy(new_pv.begin(), new_pv.begin() + depthleft - 1, pv->begin() + 1);
				}

				if (alpha == SCORE_MAX_MATE - 1)
					break;
			}
		}

		if (searched_moves == 0)
		{
			if (board.isInCheck(toMove))
				return -SCORE_MAX_MATE;
			else
				return SCORE_DRAW;
		}

		if (alpha == alpha_orig)
		{
			_transposition_table.insert(board.hash(), depthleft, alpha, Move(), UPPER_BOUND);
		}
		else if (pv)
		{
			assert((*pv)[0] != Move());
			_transposition_table.insert(board.hash(), depthleft, alpha, (*pv)[0], EXACT);
		}

		return alpha;
	}
}