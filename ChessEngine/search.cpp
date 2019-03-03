#include <chrono>
#include <iostream>
#include <thread>
#include "search.h"

namespace Search
{
	bool debug = false;
	bool stop = false;

	std::thread search_thrd = {};

	void infoThread()
	{
		const static int interval = 500;

		while (true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(interval));

			updateNodesPerSec();

			std::cout << "info hashfull " << (int)(ab_tr_table.usage() * 1000) << std::endl;

			if (stop)
				break;
		}
	}

	void updateNodesPerSec()
	{
		unsigned long long curr_node_count = Stats.alpha_beta_nodes + Stats.quiescence_nodes;
		double node_diff = (double)(curr_node_count - SearchInfo.last_node_count);

		std::chrono::steady_clock::time_point curr_time = std::chrono::steady_clock::now();
		unsigned long long time_diff = std::chrono::duration_cast<std::chrono::microseconds>(curr_time - SearchInfo.last_time).count();

		std::cout << "info nodes " << curr_node_count
			<< " nps " << (int)(node_diff * (1000000.0 / time_diff)) << std::endl;

		SearchInfo.last_node_count = curr_node_count;
		SearchInfo.last_time = curr_time;
	}

	void startSearch(const Board & board, int maxdepth)
	{
		std::thread search_thread = std::thread(Search::search, board, maxdepth);
		search_thread.detach();
	}

	void search(const Board & board, int maxdepth)
	{
		Stats = { 0 };
		std::vector<std::vector<Move>> pv(maxdepth + 1);

		ab_tr_table.clear();
		qs_tr_table.clear();

		stop = false;
		std::thread info(infoThread);
		info.detach();

		int score, depth;
		Move best_move;

		SearchInfo.last_time = std::chrono::steady_clock::now();
		SearchInfo.last_node_count = 0;

		for (depth = 1; depth <= maxdepth; ++depth)
		{
			if (board.toMove() == WHITE)
				score = alphaBeta<WHITE, true, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, depth, pv[depth]);
			else
				score = alphaBeta<BLACK, true, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, depth, depth, pv[depth]);

			updateNodesPerSec();

			if (score == SCORE_INVALID)
				break;

			if (board.toMove() == BLACK)
				score *= -1;

			sendPrincipalVariation(pv[depth], depth, score, false);
			best_move = pv[depth][0];

			if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
				break;
		}

		stop = true;
		sendBestMove(best_move);

		Stats.avg_searched_moves = (float)(Stats.alpha_beta_nodes / (double)Stats._move_gen_count);

		if (debug)
			sendStats();
	}

	void sendBestMove(Move move)
	{
		std::cout << "bestmove " << move.toAlgebraic() << std::endl;
	}

	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score, bool mate)
	{
		if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
		{
			int mate_in = (int)round((SCORE_MAX_MATE - abs(score)) / 2.0);
			if (score < 0)
				mate_in *= -1;
			std::cout << "info depth " << depth << " score mate " << mate_in << " pv";
		}
		else
			std::cout << "info depth " << depth << " score cp " << score << " pv";

		for (int i = 0; i < depth; ++i)
		{
			std::cout << " " << pv[i].toAlgebraic();
		}
		std::cout << std::endl;
	}

	void sendCurrentMove(Move move, int pos)
	{
		std::cout << "info currmove " << move.toAlgebraic() << " currmovenumber " << pos << std::endl;
	}

	void sendStats()
	{
		std::cout << std::endl
			<< "info string " << "\tpv_search_researches:\t" << Search::Stats.pv_search_research_count << std::endl
			<< "info string " << "\thash move cutoffs:\t" << Search::Stats.hash_move_cutoffs << std::endl
			<< "info string " << "\thash score returned:\t" << Search::Stats.hash_score_returned << std::endl
			<< "info string " << "\tavg searched moves:\t" << Search::Stats.avg_searched_moves << std::endl
			<< "info string " << "\talpha-beta nodes:\t" << Search::Stats.alpha_beta_nodes << std::endl
			<< "info string " << "\tquiescence nodes:\t" << Search::Stats.quiescence_nodes << std::endl
			<< "info string " << "\talpha-beta cutoffs:\t" << Search::Stats.alpha_beta_cutoffs << std::endl
			<< "info string " << "\tquiescence cutoffs:\t" << Search::Stats.quiescence_cutoffs << std::endl;
	}

	bool isMateScore(int score)
	{
		return SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE;
	}
}