#include "board.h"
#include "movegen.h"
#include "perft.h"
#include <stack>
#include <vector>

#include "config.h"

Perft::PerftResult Perft::perft(Board board, int depth)
{
	Perft::PerftResult res = { 0 };
	res.nodes = Perft::_perft(board, depth, res.captures, res.en_passants, res.king_castles, res.queen_castles, res.promotions);

	return res;
}

std::vector<std::pair<Move, Perft::PerftResult> > Perft::perftDivided(Board board, int depth)
{
	auto res = std::vector<std::pair<Move, Perft::PerftResult> >();

	Move moves[MAX_MOVES];
	int move_count;

	if (board.toMove() == WHITE)
		MoveGen::genMoves<WHITE, false>(board, moves, move_count);
	else
		MoveGen::genMoves<BLACK, false>(board, moves, move_count);

	for (int i = 0; i < move_count; ++i)
	{
		Board board_copy = board;
		if (board_copy.makeMove(moves[i]))
			res.push_back(std::make_pair(moves[i], Perft::perft(board_copy, depth)));
	}

	return res;
}

int Perft::_perft(Board &board, int depth, int &captures, int &en_passants, int &king_castles, int &queen_castles, int &promotions)
{
	if (depth <= 0)
	{
		return 1;
	}

	Move moves[MAX_MOVES];
	int move_count;

	int score = 0;

	if (board.toMove() == WHITE)
		MoveGen::genMoves<WHITE, false>(board, moves, move_count);
	else
		MoveGen::genMoves<BLACK, false>(board, moves, move_count);

	for (int i = 0; i < move_count; ++i)
	{
		Board board_copy = board;

		if (board_copy.makeMove(moves[i]))
		{
			captures += moves[i].isCapture() ? 1 : 0;
			en_passants += moves[i].isEnPassant() ? 1 : 0;
			king_castles += moves[i].isCastle(KINGSIDE) ? 1 : 0;
			queen_castles += moves[i].isCastle(QUEENSIDE)? 1 : 0;
			promotions += moves[i].isPromotion() ? 1 : 0;

			score += _perft(board_copy, depth - 1, captures, en_passants, king_castles, queen_castles, promotions);
		}
	}

	return score;
}