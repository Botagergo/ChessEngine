#include "search.h"

class SearchEventHandler
{
public:
	SearchEventHandler(Search::Search& search, bool debug)
	{
		search.onStats = nullptr;
		search.onBestMove = &onBestMove;
		search.onPrincipalVariation = onPrincipalVariation;
		search.onCurrentMove = onCurrentMove;
		search.onNodeInfo = onNodeInfo;
		search.onHashfull = onHashfull;

		if (debug)
			search.onStats = onStats;
	}

private:
	static void onBestMove(Move move, Move ponder_move)
	{
		std::cout << "bestmove " << move.toAlgebraic();
		if (ponder_move != Move())
			std::cout << " ponder " << ponder_move.toAlgebraic();
		std::cout << std::endl;
	}

	static void onPrincipalVariation(const std::vector<Move>& pv, int depth, int score, bool mate)
	{
		int moves_nb = depth;

		if (SCORE_MIN_MATE <= abs(score) && abs(score) <= SCORE_MAX_MATE)
		{
			int moves_nb = SCORE_MAX_MATE - abs(score);
			int mate_in = (int)round(moves_nb / 2.0);
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

	static void onCurrentMove(Move move, int pos)
	{
		std::cout << "info currmove " << move.toAlgebraic() << " currmovenumber " << pos << std::endl;
	}

	static void onNodeInfo(u64 node_count, u64 nodes_per_sec)
	{
		std::cout << "info nodes " << node_count
			<< " nps " << nodes_per_sec << std::endl;
	}

	static void onHashfull(int permill)
	{
		std::cout << "info hashfull " << permill << std::endl;
	}

	static void onStats(const Search::Search::Stats &stats)
	{
		std::cout << std::endl
			<< "info string " << "\tpv_search_researches:\t" << stats.pv_search_research_count << std::endl
			<< "info string " << "\tkiller move cutoffs:\t" << stats.killer_move_cutoffs << std::endl
			<< "info string " << "\thash move cutoffs:\t" << stats.hash_move_cutoffs << std::endl
			<< "info string " << "\thash score returned:\t" << stats.hash_score_returned << std::endl
			<< "info string " << "\tavg searched moves:\t" << stats.avg_searched_moves << std::endl
			<< "info string " << "\talpha-beta nodes:\t" << stats.alpha_beta_nodes << std::endl
			<< "info string " << "\tquiescence nodes:\t" << stats.quiescence_nodes << std::endl
			<< "info string " << "\talpha-beta cutoffs:\t" << stats.alpha_beta_cutoffs << std::endl
			<< "info string " << "\tquiescence cutoffs:\t" << stats.quiescence_cutoffs << std::endl;
	}
};