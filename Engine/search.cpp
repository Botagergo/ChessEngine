#include <chrono>
#include <iostream>
#include <thread>
#include "search.h"

namespace Search
{
	Search::Search() : _infinite(true), _has_maxdepth(false), _has_movetime(false), _ponder(false), _hash_size(DEFAULT_HASH_TABLE_SIZE)
	{
		_resizeHashTable(_hash_size);
	}

	void Search::_infoThread()
	{
		const static int interval = 10;

		Timer.last_time = std::chrono::steady_clock::now();
		Timer.elapsed_time = std::chrono::milliseconds(0);
		Timer.node_count = 0;
		Timer.last_node_count = 0;

		while (!_stop)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			_updateNodesPerSec();
		}
	}

	void Search::_updateNodesPerSec()
	{
		Timer.node_count = stats.alpha_beta_nodes + stats.quiescence_nodes;
		double node_diff = (double)(Timer.node_count - Timer.last_node_count);

		std::chrono::steady_clock::time_point curr_time = std::chrono::steady_clock::now();
		auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - Timer.last_time);

		Timer.nodes_per_sec = (int)(node_diff * (1000.0 / time_diff.count()));
		Timer.time_left -= time_diff;
		Timer.elapsed_time += time_diff;

		Timer.last_node_count = Timer.node_count;
		Timer.last_time = curr_time;
	}

	void Search::startSearch(const Board& board)
	{
		std::thread search_thread = std::thread(&Search::search, this, board, nullptr, false);
		search_thread.detach();
	}

	void Search::search(const Board& board, Move* bestMove, bool quiet)
	{
		memset(&stats, 0, sizeof(stats));

		Timer.has_time_left = true;
		if (hasMoveTime())
			Timer.time_left = getMoveTime();
		else if (hasClock(board.toMove()))
			Timer.time_left = std::chrono::milliseconds((long long)(getClock(board.toMove()).count() * _ThinkTimePercentage));
		else
			Timer.has_time_left = false;

		std::array<std::array<Move, MAX_DEPTH>, MAX_DEPTH> pv;

		_transposition_table.clear();
		_evaluation_table.clear();
		std::fill(_killer_moves, _killer_moves + MAX_DEPTH, std::make_pair(Move(), Move()));

		_passed_maxdepth = false;

		int score;
		int searched_depth = 0;
		_can_stop_search = false;

		_stop = false;
		std::thread info(&Search::_infoThread, this);
		info.detach();

		for (int depth = 1; _ponder || !hasMaxDepth() || depth <= _maxdepth; ++depth)
		{
			if (_has_maxdepth && depth > _maxdepth)
				_passed_maxdepth = true;

			if (board.toMove() == WHITE)
				score = _alphaBeta<WHITE, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, 0, &pv[depth]);
			else
				score = _alphaBeta<BLACK, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, 0, &pv[depth]);

			if (score == SCORE_INVALID)
				break;

			++searched_depth;

			if (onPrincipalVariation)
				onPrincipalVariation(pv[depth], depth, score, false);
			if (onNodeInfo)
				onNodeInfo(Timer.node_count, Timer.nodes_per_sec);
			if (onHashfull)
				onHashfull((int)(_transposition_table.usage() * 1000));

			_can_stop_search = true;

			_updateNodesPerSec();

			if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
				break;
		}

		_stop = true;

		Move ponder_move = Move();
		if (searched_depth >= 2)
			ponder_move = pv[searched_depth][1];

		if (onBestMove)
			onBestMove(pv[searched_depth][0], ponder_move);

		if (bestMove)
			* bestMove = pv[searched_depth][0];

		stats.avg_searched_moves = (float)(stats.alpha_beta_nodes / (double)stats._move_gen_count);
		if (onStats)
			onStats(stats);
	}

	void Search::stopSearch()
	{
		_stop = true;
	}

	bool Search::hasClock(Color color)
	{
		return _has_clock[color];
	}

	std::chrono::milliseconds Search::getClock(Color color)
	{
		return _clock[color];
	}

	void Search::setClock(Color color, std::chrono::milliseconds clock)
	{
		_has_clock[color] = true;
		_clock[color] = clock;
	}

	void Search::unsetClock(Color color)
	{
		_has_clock[color] = false;
	}

	bool Search::hasMaxDepth()
	{
		return _has_maxdepth;
	}

	int Search::getMaxDepth()
	{
		return _maxdepth;
	}

	void Search::setMaxDepth(int maxdepth)
	{
		ASSERT(maxdepth >= 0);
		_has_maxdepth = true;
		_maxdepth = maxdepth;
		_infinite = false;
	}

	void Search::unsetMaxDepth()
	{
		_has_maxdepth = false;
	}

	bool Search::hasMoveTime()
	{
		return _has_movetime;
	}

	std::chrono::milliseconds Search::getMoveTime()
	{
		return _movetime;
	}

	void Search::setMoveTime(std::chrono::milliseconds movetime)
	{
		ASSERT(movetime >= std::chrono::milliseconds(0));
		_has_movetime = true;
		_movetime = movetime;
		_infinite = false;
	}

	void Search::unsetMoveTime()
	{
		_has_movetime = false;
	}

	bool Search::isInfinite()
	{
		return _infinite;
	}

	void Search::setInfinite(bool infinite)
	{
		_infinite = infinite;
		_has_movetime = _has_maxdepth = !_infinite;
	}

	bool Search::getPonder()
	{
		return _ponder;
	}

	void Search::setPonder(bool ponder)
	{
		_ponder = ponder;
	}

	size_t Search::getHashSize()
	{
		return _hash_size;
	}

	void Search::setHashSize(size_t size)
	{
		_hash_size = size;
		_resizeHashTable(_hash_size);
	}

	const Search::Stats& Search::getStats()
	{
		return stats;
	
	}

	bool Search::_isRepetition(u64 hash, int ply)
	{
		for (int i = 0; i < ply; i += 2)
		{
			if (_history[i] == hash)
				return true;
		}
		return false;
	}

	bool Search::_isMateScore(int score)
	{
		return SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE;
	}

	int Search::_razorMargin(int depth)
	{
		return (90 * (depth - 1) + 18);
	}
	bool Search::_passedMaxdepth()
	{
		return _has_maxdepth && _passed_maxdepth;
	}
	bool Search::_timeOut()
	{
		return Timer.has_time_left && Timer.time_left <= std::chrono::milliseconds(0);
	}
	bool Search::_shouldStopSearch()
	{
		return _can_stop_search && (
			_stop
			|| !_ponder && _passedMaxdepth()
			|| _timeOut());
	}

	void Search::_resizeHashTable(size_t size)
	{
		_transposition_table.resize((size_t)(size * 0.75));
		_evaluation_table.resize((size_t)(size * 0.25));
	}

	const int Search::_RFutility_Depth = 3;
	const int Search::_RFutility_Param = 120;
	const int Search::_IID_Depth = 6;
	const int Search::_IID_Reduction = 4;
	const double Search::_ThinkTimePercentage = 0.025;
}