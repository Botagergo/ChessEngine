#include <chrono>
#include <iostream>
#include <thread>
#include "search.h"

namespace Search
{
	std::thread search_thrd = {};

	SearchInfo searchInfo;

	void infoThread()
	{
		const static int interval = 500;

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			updateNodesPerSec();

			if (searchInfo.stop)
				break;
		}
	}

	void updateNodesPerSec()
	{
		searchInfo.NodeCountInfo.node_count = searchInfo.Stats.alpha_beta_nodes + searchInfo.Stats.quiescence_nodes;
		double node_diff = (double)(searchInfo.NodeCountInfo.node_count - searchInfo.NodeCountInfo.last_node_count);

		std::chrono::steady_clock::time_point curr_time = std::chrono::steady_clock::now();
		u64 time_diff = std::chrono::duration_cast<std::chrono::microseconds>(curr_time - searchInfo.NodeCountInfo.last_time).count();

		searchInfo.NodeCountInfo.nodes_per_sec = (int)(node_diff * (1000000.0 / time_diff));

		searchInfo.NodeCountInfo.last_node_count = searchInfo.NodeCountInfo.node_count;
		searchInfo.NodeCountInfo.last_time = curr_time;
	}

	void startSearch(const Board & board, int maxdepth)
	{
		std::thread search_thread = std::thread(Search::search, board, maxdepth, nullptr);
		search_thread.detach();
	}

	void search(const Board & board, int maxdepth, Move *bestMove)
	{
		searchInfo.Stats = { 0 };
		std::vector<std::vector<Move>> pv(MAX_DEPTH);

		searchInfo.transposition_table.clear();
		searchInfo.evaluation_table.clear();
		std::fill(searchInfo.killer_moves, searchInfo.killer_moves + MAX_DEPTH, std::make_pair(Move(), Move()));

		searchInfo.passed_maxdepth = false;

		int score;
		int searched_depth = 0;

		searchInfo.NodeCountInfo.last_time = std::chrono::steady_clock::now();
		searchInfo.NodeCountInfo.last_node_count = 0;

		searchInfo.stop = false;
		std::thread info(infoThread);
		info.detach();

		for (int depth = 1; searchInfo.ponder || depth <= maxdepth; ++depth)
		{
			if (depth > maxdepth)
				searchInfo.passed_maxdepth = true;

			if (board.toMove() == WHITE)
				score = alphaBeta<WHITE, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, 0, &pv[depth]);
			else
				score = alphaBeta<BLACK, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, 0, &pv[depth]);

			if (searchInfo.sendOutput)
				updateNodesPerSec();

			if (score == SCORE_INVALID)
				break;

			++searched_depth;

			if (board.toMove() == BLACK)
				score *= -1;

			if (searchInfo.sendOutput)
				sendPrincipalVariation(pv[depth], depth, score, false);

			if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
				break;
		}

		searchInfo.stop = true;

		Move ponder_move = Move();
		if (searched_depth >= 2)
			ponder_move = pv[searched_depth][1];

		if (searchInfo.sendOutput)
			sendBestMove(pv[searched_depth][0], ponder_move);

		if (bestMove)
			*bestMove = pv[searched_depth][0];

		searchInfo.Stats.avg_searched_moves = (float)(searchInfo.Stats.alpha_beta_nodes / (double)searchInfo.move_gen_count);

		if (searchInfo.debug && searchInfo.sendOutput)
			sendStats();
	}

	bool isRepetition(u64 hash, int ply)
	{
		for (int i = 0; i < ply; i += 2)
		{
			if (searchInfo.history[i] == hash)
				return true;
		}
		return false;
	}

	void sendBestMove(Move move, Move ponder_move)
	{
		std::cout << "bestmove " << move.toAlgebraic();
		if (ponder_move != Move())
			std::cout << " ponder " << ponder_move.toAlgebraic();
		std::cout << std::endl;
	}

	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score, bool mate)
	{
		int moves_nb = depth;

		if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
		{
			int moves_nb = SCORE_MAX_MATE - abs(score);
			int mate_in  = (int)round(moves_nb / 2.0);
			if (score < 0)
				mate_in *= -1;
			std::cout << "info depth " << depth << " score mate " << mate_in << " pv";
		}
		else
			std::cout << "info depth " << depth << " score cp " << score << " pv";

		for (int i = 0; i < moves_nb; ++i)
		{
			std::cout << " " << pv[i].toAlgebraic();
		}
		std::cout << std::endl;
	}

	void sendCurrentMove(Move move, int pos)
	{
		std::cout << "info currmove " << move.toAlgebraic() << " currmovenumber " << pos << std::endl;
	}

	void sendNodeInfo(u64 node_count, u64 nodes_per_sec)
	{
		std::cout << "info nodes " << node_count
			<< " nps " << nodes_per_sec << std::endl;
	}

	void sendHashfull(int permill)
	{
		std::cout << "info hashfull " << permill << std::endl;
	}

	void sendStats()
	{
		std::cout << std::endl
			<< "info string " << "\thash table - failed inserts:\t" << searchInfo.transposition_table.getStats()->failed_inserts << std::endl
			<< "info string " << "\tpv_search_researches:\t" << searchInfo.Stats.pv_search_research_count << std::endl
			<< "info string " << "\tkiller move cutoffs:\t" << searchInfo.Stats.killer_move_cutoffs << std::endl
			<< "info string " << "\thash move cutoffs:\t" << searchInfo.Stats.hash_move_cutoffs << std::endl
			<< "info string " << "\thash score returned:\t" << searchInfo.Stats.hash_score_returned << std::endl
			<< "info string " << "\tavg searched moves:\t" <<searchInfo.Stats.avg_searched_moves << std::endl
			<< "info string " << "\talpha-beta nodes:\t" << searchInfo.Stats.alpha_beta_nodes << std::endl
			<< "info string " << "\tquiescence nodes:\t" << searchInfo.Stats.quiescence_nodes << std::endl
			<< "info string " << "\talpha-beta cutoffs:\t" << searchInfo.Stats.alpha_beta_cutoffs << std::endl
			<< "info string " << "\tquiescence cutoffs:\t" << searchInfo.Stats.quiescence_cutoffs << std::endl;
	}

	bool isMateScore(int score)
	{
		return SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE;
	}
}