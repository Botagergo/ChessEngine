#include "board.h"
#include "movegen.h"
#include "perft.h"
#include <stack>
#include <vector>

Perft::PerftResult Perft::perft(Board board, int depth)
{
	Perft::PerftResult res = { 0 };
	res.nodes = Perft::_perft(board, depth, res.captures, res.en_passants, res.king_castles, res.queen_castles, res.promotions);

	return res;
}

std::vector<std::pair<Move, Perft::PerftResult> > Perft::perftPerMove(Board board, int depth)
{
	auto res = std::vector<std::pair<Move, Perft::PerftResult> >();
	std::vector<Move> moves;

	if (board.toMove() == WHITE)
		Move_Gen::genMoves<WHITE, false>(board, moves);
	else
		Move_Gen::genMoves<BLACK, false>(board, moves);

	for (Move &move : moves)
	{
		Board board_copy = board;
		if (board_copy.makeMove(move))
			res.push_back(std::make_pair(move, Perft::perft(board_copy, depth)));
	}

	return res;
}

int Perft::_perft(Board &board, int depth, int &captures, int &en_passants, int &king_castles, int &queen_castles, int &promotions)
{
	if (depth <= 0)
	{
		return 1;
	}

	std::vector<Move> moves;
	int score = 0;

	if (board.toMove() == WHITE)
		Move_Gen::genMoves<WHITE, false>(board, moves);
	else
		Move_Gen::genMoves<BLACK, false>(board, moves);

	for (Move &move : moves)
	{
		Board board_copy = board;

		if (board_copy.makeMove(move))
		{
			captures += move.flags & CAPTURE ? 1 : 0;
			en_passants += move.flags & EN_PASSANT ? 1 : 0;
			king_castles += move.flags & KINGSIDE_CASTLE ? 1 : 0;
			queen_castles += move.flags & QUEENSIDE_CASTLE ? 1 : 0;
			promotions += move.promotion != NO_PIECE ? 1 : 0;

			score += _perft(board_copy, depth - 1, captures, en_passants, king_castles, queen_castles, promotions);
		}
	}

	return score;
}