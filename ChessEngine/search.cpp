#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include "search.h"

bool Search::debug = false;
bool Search::stop = false;

std::thread Search::search_thrd = {};

void Search::infoThread()
{
	const static int interval = 500;
	unsigned long long last_node_count = 0;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(interval));

		unsigned long long curr_node_count = Stats.alpha_beta_nodes + Stats.quiescence_nodes;
		double node_diff = (double)(curr_node_count - last_node_count);

		std::cout << "info nodes " << curr_node_count
			<< " nps " << (int)(node_diff * (1000.0 / interval)) << std::endl;

		last_node_count = curr_node_count;

		if (Search::stop)
			break;
	}
}

void Search::startSearch(const Board & board, int maxdepth)
{
	std::thread search_thread = std::thread(Search::search, board, maxdepth);
	search_thread.detach();
}

void Search::search(const Board & board, int maxdepth)
{
	Search::Stats = { 0 };
	std::vector<std::vector<Move>> pv(maxdepth + 1);

	Search::stop = false;
	std::thread info(infoThread);
	info.detach();

	int score, depth;

	for (depth = 1; depth <= maxdepth; ++depth)
	{
		if (board.toMove() == WHITE)
			score = alphaBeta<WHITE, true, true>(board, -MAX_SCORE, MAX_SCORE, depth, maxdepth, pv[depth]);
		else
			score = alphaBeta<BLACK, true, true>(board, -MAX_SCORE, MAX_SCORE, depth, maxdepth, pv[depth]);

		if (score == INVALID_SCORE)
			break;


		if (board.toMove() == BLACK)
			score *= -1;

		sendPrincipalVariation(pv[depth], depth, score);
	}

	Search::stop = true;
	sendBestMove(pv[depth - 1][0]);

	Stats.avg_searched_moves = (float)(Stats.alpha_beta_nodes / (double)Stats._move_gen_count);

	if (debug)
		Search::sendStats();
}

void Search::sendBestMove(Move move)
{
	std::cout << "bestmove " << move.toAlgebraic() << std::endl;
}

void Search::sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score)
{
	std::cout << "info depth " << depth << " score cp " << score << " pv";
	for (Move move : pv)
	{
		std::cout << " " << move.toAlgebraic();
	}
	std::cout << std::endl;
}

void Search::sendCurrentMove(Move move, int pos)
{
	std::cout << "info currmove " << move.toAlgebraic() << " currmovenumber " << pos << std::endl;
}

void Search::sendStats()
{
	std::cout << std::endl
		<< "info string " << "\thash move cutoffs:\t" << Search::Stats.hash_move_cutoff << std::endl
		<< "info string " << "\thash score returned:\t" << Search::Stats.hash_score_returned << std::endl
		<< "info string " << "\tavg searched moves:\t" << Search::Stats.avg_searched_moves << std::endl
		<< "info string " << "\talpha-beta nodes:\t" << Search::Stats.alpha_beta_nodes << std::endl
		<< "info string " << "\tquiescence nodes:\t" << Search::Stats.quiescence_nodes << std::endl
		<< "info string " << "\talpha-beta cutoffs:\t" << Search::Stats.alpha_beta_cutoffs << std::endl
		<< "info string " << "\tquiescence cutoffs:\t" << Search::Stats.quiescence_cutoffs << std::endl;
}