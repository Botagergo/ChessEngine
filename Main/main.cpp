#include "attacks.h"
#include "board.h"
#include "bitboard_iterator.h"
#include "epd.h"
#include "perft.h"
#include "zobrist.h"
#include "config.h"
#include "search.h"
#include "search_event_handler.h"

#include <iostream>
#include <fstream>
#include <map>
#include <stdio.h>

using namespace std;

const std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void setoptionReceived(Search::Search& search, std::string name, std::string value);
void perftReceived(Board board, int depth, std::vector<Move> moves, bool per_move, bool full);
void printPerftRes(const Perft::PerftResult &res);

void runTest(const std::string &in_file, int depth, const std::string &out_file = "");

int main(int argc, char *argv[])
{
	Search::Search search;

	initSquareBB();
	initAttackTables();
	initObstructedTable();
	Zobrist::initZobristHashing();
	initDistanceTable();

	Board board;
	bool debug = false;

	std::ofstream log("log.txt");

	std::string line;
	auto   running = true;
	while (running && getline(std::cin, line))
	{
		log << line << std::endl;

		std::istringstream iss(line);
		std::string        token;
		iss >> std::skipws >> token;
		if (token == "uci")
		{
			std::cout << "option name Hash type spin min 2 max 4096 default 32" << std::endl;
			std::cout << "option name Ponder" << std::endl;
			std::cout << "uciok" << std::endl;
		}
		else if (token == "debug")
		{
			iss >> token;
			debug = token == "on";
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

			setoptionReceived(search, name, value);
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
			long long clock_left[COLOR_NB] = { -1, -1 };
			search.setInfinite(false);
			search.unsetMaxDepth();
			search.unsetClock(WHITE);
			search.unsetClock(BLACK);
			search.unsetMoveTime();
			search.setPonder(false);

			while (iss >> token)
				if (token == "ponder")
					search.setPonder(true);
				else if (token == "depth")
				{
					int depth;
					iss >> depth;
					search.setMaxDepth(depth);
				}
				else if (token == "movetime")
				{
					long long movetime;
					iss >> movetime;
					search.setMoveTime(std::chrono::milliseconds(movetime));
				}
				else if (token == "wtime")
				{
					long long clock;
					iss >> clock;
					search.setClock(WHITE, std::chrono::milliseconds(clock));
				}
				else if (token == "btime")
				{
					long long clock;
					iss >> clock;
					search.setClock(BLACK, std::chrono::milliseconds(clock));
				}

			if (search.getPonder())
			{
				search.unsetMaxDepth();
				search.unsetClock(WHITE);
				search.unsetClock(BLACK);
				search.unsetMoveTime();
			}

			SearchEventHandler handler(search, debug);
			search.startSearch(board);
		}
		else if (token == "stop")
		{
			search.stopSearch();
		}
		else if (token == "ponderhit")
		{
			search.setPonder(false);
		}
		else if (token == "quit")
		{
			search.stopSearch();
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
		else if (token == "run_perft")
		{
			std::string filename;
			iss >> filename;
			Perft::perft(filename);
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

void setoptionReceived(Search::Search &search, std::string name, std::string value)
{
	if (name == "Hash")
	{
		std::stringstream ss(value);
		int size;
		ss >> size;
		search.setHashSize(size);
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
	Search::Search search;
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

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	for (int i = 0; i < epdData.size(); ++i)
	{
		std::stringstream status_message;
		status_message << "Running tests... (" << i + 1 << "/" << epdData.size() << ")";
		std::cout << status_message.str();

		std::stringstream message;
		Move bestMove;

		search.setMaxDepth(depth);
		search.search(epdData[i].board, &bestMove);

		message << epdData[i].id << "\t:\t";
		if (std::find(epdData[i].bad_moves.begin(), epdData[i].bad_moves.end(), bestMove) != epdData[i].bad_moves.end() |
			std::find(epdData[i].good_moves.begin(), epdData[i].good_moves.end(), bestMove) != epdData[i].good_moves.end())
			message << "incorrect move: " << bestMove.toAlgebraic();
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