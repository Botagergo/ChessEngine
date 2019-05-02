#include "attacks.h"
#include "board.h"
#include "bitboard_iterator.h"
#include "epd.h"
#include "perft.h"
#include "zobrist.h"

#include <iostream>
#include <fstream>
#include <map>
#include <search.h>

using namespace std;

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void setoptionReceived(std::string name, std::string value);
void perftReceived(Board board, int depth, std::vector<Move> moves, bool per_move, bool full);
void printPerftRes(const Perft::PerftResult &res);

void runTest(const std::string &in_file, int depth, const std::string &out_file = "");

Board board;
int maxdepth = 9;

enum Command
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
	initDistanceTable();

	std::string line;
	auto   running = true;
	while (running && getline(std::cin, line))
	{
		std::istringstream iss(line);
		std::string        token;
		iss >> std::skipws >> token;
		if (token == "uci")
		{
			std::cout << "option name Hash type spin min 2 max 4096 default 32" << std::endl;
			std::cout << "option name Ponder" << std::endl;
			std::cout << "option name maxdepth type string default 6" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if (token == "debug")
		{
			iss >> token;
			if (token == "on")
				Search::searchInfo.debug = true;
			else if (token == "off")
				Search::searchInfo.debug = false;
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
				board.makeMove(Move::fromAlgebraic(board, move));
			}
		}
		else if (token == "go")
		{
			std::map<Command, CommandParam> commands;
			commands[Command::depth].number = maxdepth;

			Search::searchInfo.ponder = false;
			Search::searchInfo.sendOutput = true;

			while (iss >> token)
				if (token == "searchmoves")
					while (iss >> token)
						commands[Command::search_moves].str += std::string(" ", commands[Command::search_moves].str.empty() ? 0 : 1) + token;
				else if (token == "ponder")
					Search::searchInfo.ponder = true;
				else if (token == "wtime")
					iss >> commands[Command::white_time].number;
				else if (token == "btime")
					iss >> commands[Command::black_time].number;
				else if (token == "winc")
					iss >> commands[Command::white_increment].number;
				else if (token == "binc")
					iss >> commands[Command::black_increment].number;
				else if (token == "movestogo")
					iss >> commands[Command::moves_to_go].number;
				else if (token == "depth")
					iss >> commands[Command::depth].number;
				else if (token == "nodes")
					iss >> commands[Command::nodes].number;
				else if (token == "mate")
					iss >> commands[Command::mate].number;
				else if (token == "move_time")
					iss >> commands[Command::move_time].number;
				else if (token == "infinite")
					commands[Command::infinite];

			Search::startSearch(board, commands[Command::depth].number);
		}
		else if (token == "stop")
		{
			Search::searchInfo.stop = true;
		}
		else if (token == "ponderhit")
		{
			Search::searchInfo.ponder = false;
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
			std::vector<std::string> str_moves;
			bool divided = false, full = false;

			while (iss >> token)
			{
				if (token == "depth")
					iss >> depth;
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
				else if (token == "divided")
				{
					divided = true;
				}
				else if (token == "full")
				{
					full = true;
				}
				else
					continue;
			}

			std::vector<Move> moves;

			for (auto move : str_moves)
			{
				moves.push_back(Move::fromAlgebraic(board, move));
			}

			perftReceived(board, depth, moves, divided, full);
		}
		else if (token == "run_test")
		{
			std::string filename;
			int depth;
			iss >> filename;
			iss >> depth;
			runTest(filename, depth);
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
	else if (name == "Hash")
	{
		std::stringstream ss(value);
		int size_mb;
		ss >> size_mb;
		Search::searchInfo.transposition_table.resize(size_mb);
		Search::searchInfo.evaluation_table.resize(size_mb);
	}
}

void perftReceived(Board board, int depth, std::vector<Move> moves, bool per_move, bool full)
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
		auto res = Perft::perftDivided(board, depth);
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

void printPerftRes(const Perft::PerftResult& res)
{
	std::cout << "nodes\t\t\t" << res.nodes << std::endl;
	std::cout << "captures\t\t" << res.captures << std::endl;
	std::cout << "en passants\t\t" << res.en_passants << std::endl;
	std::cout << "kingside castles\t" << res.king_castles << std::endl;
	std::cout << "queenside castles\t" << res.queen_castles << std::endl;
	std::cout << "promotions\t\t" << res.promotions << std::endl;
}

void runTest(const std::string &in_file, int depth, const std::string &out_file)
{
	std::ifstream in(in_file);

	if (!in)
	{
		std::cerr << "Error opening test file: " << in_file << std::endl;
		return;
	}

	std::vector<EpdData> epdData;

	char buf[256];
	while (in.getline(buf, 256))
	{
		try
		{
			epdData.push_back(EpdData(buf));
		}
		catch (EpdParseError e)
		{
			std::cerr << "Invalid epd string: " << buf << std::endl;
			return;
		}
	}

	std::vector<std::string> messages;
	messages.reserve(epdData.size());

	int passed = 0;
	Search::searchInfo.sendOutput = false;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	for (int i = 0; i < epdData.size(); ++i)
	{
		std::stringstream status_message;
		status_message << "Running tests... (" << i + 1 << "/" << epdData.size() << ")";
		std::cout << status_message.str();

		std::stringstream message;
		Move bestMove;
		Search::search(epdData[i].board, depth, &bestMove);

		message << epdData[i].id << "\t:\t";
		if (epdData[i].avoidMove.isValid() && bestMove == epdData[i].avoidMove)
			message << "incorrect move: " << bestMove.toAlgebraic();
		else if (epdData[i].bestMove.isValid() && bestMove != epdData[i].bestMove)
			message << "expected: " << epdData[i].bestMove.toAlgebraic() << ", actual:" << bestMove.toAlgebraic();
		else
		{
			message << "OK";
			passed++;
		}

		messages.push_back(message.str());
		std::cout << std::string(status_message.str().size(), '\b');
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	u64 duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	chrono::system_clock::time_point timestamp = chrono::system_clock::now();
	time_t t = chrono::system_clock::to_time_t(timestamp);

	std::string filename = out_file;

	if (filename.empty())
	{
		std::stringstream ss;
		ss << in_file << "_" << t << ".txt";
		filename = std::string(ss.str());
	}

	std::ofstream out(filename);
	if (!out)
	{
		std::cerr << "Error creating test result file" << std::endl;
		return;
	}

	out << "test duration:\t" << duration / 1000.0 << " seconds " << std::endl;
	out << "passed:\t\t\t" << passed << std::endl;
	out << "failed:\t\t\t" << epdData.size() - passed << std::endl;
	out << std::endl;

	for (const std::string &message : messages)
	{
		out << message << std::endl;
	}

	out.close();
	std::cout << std::endl << "Finished" << std::endl;
}