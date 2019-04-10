#pragma once

#include <regex>

#include "board.h"

class EpdParseError : std::exception
{
public:
	EpdParseError(const char *msg) : exception(msg) {}
};

struct EpdData
{
	EpdData(const std::string &epd);
	EpdData() {};

	Board board;
	Move bestMove = Move();
	Move avoidMove = Move();
	std::string id = "";
};

EpdData::EpdData(const std::string &epd)
{
	std::stringstream fen_ss;
	std::stringstream epd_ss(epd);

	for (int i = 0; i < 4; ++i)
	{
		std::string str;
		epd_ss >> str;
		fen_ss << str << " ";
	}

	fen_ss << "0 1";

	try 
	{
		board = Board::fromFen(fen_ss.str());
	}
	catch (FenParseError e)
	{
		throw EpdParseError(epd.c_str());
	}

	char buf[256];
	epd_ss.getline(buf, 256);

	std::string opcodes = buf;
	std::string opcode_regex = "(\\w+) ([^;]*);";

	std::smatch opcode_match_result;

	while (std::regex_search(opcodes, opcode_match_result, std::regex(opcode_regex)))
	{
		std::string opcode = opcode_match_result[1];
		std::string parameter = opcode_match_result[2];
		if (opcode == "bm")
		{
			bestMove = Move::fromSan(board, parameter);
			if (!bestMove.isValid())
			{
				std::stringstream msg;
				msg << "Invalid move: \"" << parameter << "\"";
				throw EpdParseError(msg.str().c_str());
			}
		}
		else if (opcode == "am")
			avoidMove = Move::fromSan(board, parameter);
		else if (opcode == "id")
			id = parameter;
		else
		{
			std::stringstream msg;
			msg << "Unknown opcode: \"" << opcode << "\"";
			throw EpdParseError(msg.str().c_str());
		}
		opcodes  = opcode_match_result.suffix().str();
	}
}