#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "bitboard_iterator.h"
#include "perft.h"
#include "zobrist.h"

#include <iostream>
#include <map>
#include <search.h>

using namespace std;

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void setoptionReceived(std::string name, std::string value);
void perft_received(Board board, int depth, std::vector<Move> moves, bool per_move, bool full);
void printPerftRes(Perft::PerftResult res);

Board board;
int maxdepth = 7;

enum class command
{
	search_moves,
	ponder,
	white_time,
	black_time,
	white_increment,
	black_increment,
	moves_to_go,
	depth,
	nodes,
	mate,
	move_time,
	infinite
};

struct CommandParam
{
	std::string str = "";
	int number = 0;

};

int main(int argc, char *argv[])
{
	initSquareBB();
	initAttackTables();
	initObstructedTable();
	Zobrist::initZobristHashing();

	std::string line;
	auto   running = true;
	while (running && getline(std::cin, line))
	{
		std::istringstream iss(line);
		std::string        token;
		iss >> std::skipws >> token;
		if (token == "uci")
		{
			std::cout << "option name maxdepth type string" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if (token == "debug")
		{
			iss >> token;
			if (token == "on")
				Search::debug = true;
			else if (token == "off")
				Search::debug = false;
		}
		else if (token == "isready")
		{
			std::cout << "readyok" << std::endl;
		}
		else if (token == "setoption")
		{
			std::string name, value;
			iss >> token;
			while (iss >> token && token != "value")
				name += std::string(" ", name.empty() ? 0 : 1) + token;
			while (iss >> token)
				value += std::string(" ", value.empty() ? 0 : 1) + token;

			setoptionReceived(name, value);
		}
		else if (token == "register")
		{
			auto        later = false;
			std::string name;
			std::size_t code = 0;
			iss >> token;
			if (token == "later")
				later = true;
			if (token == "name")
				while (iss >> token && token != "code")
					name += std::string(" ", name.empty() ? 0 : 1) + token;
			if (token == "code")
				iss >> code;
		}
		else if (token == "ucinewgame")
		{
		}
		else if (token == "position")
		{
			std::string fen;
			std::vector<std::string> moves;
			iss >> token;
			if (token == "startpos")
			{
				fen = start_fen;
				iss >> token;
			}
			else if (token == "fen")
				while (iss >> token && token != "moves")
					fen += token + " ";
			else
				continue;
			while (iss >> token)
				moves.push_back(token);

			board = Board::fromFen(fen);
			for (std::string move : moves)
			{
				board.makeMove(Move::parse(board, move));
			}
		}
		else if (token == "go")
		{
			std::map<command, CommandParam> commands;
			commands[command::depth].number = maxdepth;

			while (iss >> token)
				if (token == "searchmoves")
					while (iss >> token)
						commands[command::search_moves].str += std::string(" ", commands[command::search_moves].str.empty() ? 0 : 1) + token;
				else if (token == "ponder")
					commands[command::ponder];
				else if (token == "wtime")
					iss >> commands[command::white_time].number;
				else if (token == "btime")
					iss >> commands[command::black_time].number;
				else if (token == "winc")
					iss >> commands[command::white_increment].number;
				else if (token == "binc")
					iss >> commands[command::black_increment].number;
				else if (token == "movestogo")
					iss >> commands[command::moves_to_go].number;
				else if (token == "depth")
					iss >> commands[command::depth].number;
				else if (token == "nodes")
					iss >> commands[command::nodes].number;
				else if (token == "mate")
					iss >> commands[command::mate].number;
				else if (token == "move_time")
					iss >> commands[command::move_time].number;
				else if (token == "infinite")
					commands[command::infinite];

			Search::startSearch(board, commands[command::depth].number);
		}
		else if (token == "stop")
		{
			Search::stop = true;
		}
		else if (token == "ponderhit")
		{
		}
		else if (token == "quit")
		{
			running = false;
		}
		else if (token == "eval")
		{
			std::cout << Evaluation::evaluate<WHITE>(board) << std::endl;
		}
		else if (token == "perft")
		{
			int depth = 6;
			std::string fen = start_fen;
			std::vector<std::string> str_moves;
			bool per_move = false, full = false;

			while (iss >> token)
			{
				if (token == "depth")
					iss >> depth;
				else if (token == "fen")
				{
					std::stringstream ss;
					for (int i = 0; i < 6; ++i)
					{
						std::string part;
						iss >> part;
						ss << part << " ";
					}

					fen = ss.str();
				}
				else if (token == "moves")
				{
					int n;
					iss >> n;
					for (int i = 0; i < n; ++i)
					{
						std::string move_str;
						iss >> move_str;
						str_moves.push_back(move_str);
					}
				}
				else if (token == "per_move")
				{
					per_move = true;
				}
				else if (token == "full")
				{
					full = true;
				}
				else
					continue;
			}

			Board board = Board::fromFen(fen);
			std::vector<Move> moves;

			for (auto move : str_moves)
			{
				moves.push_back(Move::parse(board, move));
			}

			perft_received(board, depth, moves, per_move, full);
		}
		else
		{
			std::cout << "Unrecognized command: " << line << std::endl;
		}
	}
}

void setoptionReceived(std::string name, std::string value)
{
	if (name == "maxdepth")
	{
		std::stringstream ss(value);
		ss >> maxdepth;
	}
}

void perft_received(Board board, int depth, std::vector<Move> moves, bool per_move, bool full)
{
	for (auto move : moves)
	{
		board.makeMove(move);
	}

	std::cout << std::endl << "perft results" << std::endl;
	std::cout << "fen\t\t\t" << board.fen() << std::endl;
	std::cout << "depth\t\t\t" << depth << std::endl;
	std::cout << std::endl;

	if (per_move)
	{
		auto res = Perft::perftPerMove(board, depth);
		for (auto p : res)
		{
			std::cout << p.first.toAlgebraic() << ":\t";

			if (full)
			{
				std::cout << std::endl;
				printPerftRes(p.second);
				std::cout << std::endl;
			}
			else
				std::cout << p.second.nodes << std::endl;
		}

		if (full)
			std::cout << std::endl;
	}
	else
	{
		auto res = Perft::perft(board, depth);
		printPerftRes(res);
	}
}

void printPerftRes(Perft::PerftResult res)
{
	std::cout << "nodes\t\t\t" << res.nodes << std::endl;
	std::cout << "captures\t\t" << res.captures << std::endl;
	std::cout << "en passants\t\t" << res.en_passants << std::endl;
	std::cout << "kingside castles\t" << res.king_castles << std::endl;
	std::cout << "queenside castles\t" << res.queen_castles << std::endl;
	std::cout << "promotions\t\t" << res.promotions << std::endl;
}