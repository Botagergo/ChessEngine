#include "board.h"
#include "movegen.h"
#include "perft.h"

#include <fstream>
#include <stack>
#include <vector>

#include "config.h"

Perft::PerftResult Perft::perft(Board board, int depth)
{
	Perft::PerftResult res = { 0 };
	res.nodes = Perft::_perft(board, depth, res.captures, res.en_passants, res.king_castles, res.queen_castles, res.promotions);

	return res;
}

void Perft::perft(const std::string &file)
{
	std::ifstream in(file);
	if (!in)
	{
		std::cerr << "Error opening file \"" << file << "\"" << std::endl;
		return;
	}

	char buf[256];
	bool success = true;

	while (in.getline(buf, 256))
	{
		std::stringstream line(buf), fen;
		int depth;
		long long expected;

		for (int i = 0; i < 6; ++i)
		{
			std::string part;
			line >> part;
			fen << part << " ";
		}

		line >> depth >> expected;

		PerftResult res = perft(Board::fromFen(fen.str()), depth);
		if (res.nodes != expected)
		{
			std::cerr << "Error: " << fen.str() << std::endl;
			success = false;
		}
	}

	if (success)
		std::cerr << "Perft ok" << std::endl;
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

long long Perft::_perft(Board &board, long long depth, long long &captures, long long &en_passants, long long &king_castles, long long &queen_castles, long long &promotions)
{
	if (depth <= 0)
	{
		return 1;
	}

	Move moves[MAX_MOVES];
	int move_count;

	long long score = 0;

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