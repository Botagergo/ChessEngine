#pragma once

#include <vector>

namespace Perft
{
	struct PerftResult
	{
		long long nodes;
		long long captures;
		long long king_castles;
		long long queen_castles;
		long long en_passants;
		long long promotions;
	};

	PerftResult perft(Board board, int depth);
	std::vector<std::pair<Move, PerftResult> > perftDivided(Board board, int depth);
	void perft(const std::string &file);

	long long _perft(Board &board, long long depth, long long &captures, long long &en_passants, long long &king_castles, long long &queen_castles, long long &promotions);
}