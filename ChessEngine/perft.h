#pragma once

struct PerftResult
{
	int nodes;
	int captures;
	int king_castles;
	int queen_castles;
	int en_passants;
	int promotions;
};

PerftResult perft(Board board, int depth);
std::vector<std::pair<Move, PerftResult> > perftPerMove(Board board, int depth);
int _perft(Board &board, int depth, int &captures, int &en_passants, int &king_castles, int &queen_castles, int &promotions);