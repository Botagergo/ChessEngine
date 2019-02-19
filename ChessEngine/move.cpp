#include <regex>
#include <sstream>

#include "board.h"
#include "constants.h"
#include "move.h"

Move::Move(unsigned move) : _move(move)
{}

Move::Move() : Move(FLAG_INVALID)
{}

Move::Move(PieceType piece_type, Square from, Square to, unsigned flags)
	: Move(piece_type | (from << 3) | (to << 9) | flags)
{}

Move::Move(PieceType piece_type, Square from, Square to, PieceType promotion, unsigned flags)
	: Move(piece_type | (from << 3) | (to << 9) | (promotion << 15) | FLAG_PROMOTION | flags)
{}

Move Move::parse(const Board & board, std::string algebraic)
{
	const static std::string square_regex = "([a-h][1-8])";
	const static std::string promotion_regex = "(nbrq)?";
	const static std::string whitespace_regex = "\\s*";

	const static std::string move_regex = std::string("^") +
		whitespace_regex + square_regex + square_regex + promotion_regex +
		whitespace_regex + std::string("$");

	std::smatch match_result;
	if (!std::regex_search(algebraic, match_result, std::regex(move_regex)))
		throw MoveParseError(algebraic.c_str());

	Square from = parseSquare(match_result[1]);
	Square to = parseSquare(match_result[2]);
	PieceType promotion = match_result[3].matched ? charToPieceType(*match_result[3].first) : NO_PIECE_TYPE;
	PieceType piece_type = toPieceType(board.pieceAt(from));

	unsigned flags = 0;

	if (piece_type == KING)
	{
		if (to - from == 2)
			flags |= FLAG_KINGSIDE_CASTLE;
		else if (from - to == 2)
			flags |= FLAG_QUEENSIDE_CASTLE;
	}

	if (board.pieceAt(to) != NO_PIECE)
		flags |= FLAG_CAPTURE;
	else if (piece_type == PAWN)
	{
		if (abs(from - to) == 16)
			flags |= FLAG_DOUBLE_PUSH;
		else
		{
			int diff = abs(to - from);
			if (diff == 7 || diff == 9)
				flags |= FLAG_EN_PASSANT;
		}
	}

	if (promotion != NO_PIECE_TYPE)
		return Move(piece_type, from, to, promotion, flags);
	else
		return Move(piece_type, from, to, flags);
}

PieceType Move::pieceType() const
{
	return (PieceType)(_move & 7);
}

Square Move::from() const
{
	return (Square)((_move >> 3) & 63);
}

Square Move::to() const
{
	return (Square)((_move >> 9) & 63);
}

PieceType Move::promotion() const
{
	return (PieceType)((_move >> 15) & 7);
}

bool Move::isDoublePush() const
{
	return _move & FLAG_DOUBLE_PUSH;
}


bool Move::isCapture() const
{
	return _move & FLAG_CAPTURE;
}

bool Move::isEnPassant() const
{
	return _move & FLAG_EN_PASSANT;
}

bool Move::isPromotion() const
{
	return _move & FLAG_PROMOTION;
}

bool Move::isCastle() const
{
	return _move & FLAG_CASTLE;
}

bool Move::isCastle(Side side) const
{
	return _move & ((side + 1) << 21);
}

bool Move::isValid() const
{
	return !(_move & FLAG_INVALID);
}

bool Move::isNull() const
{
	return _move & FLAG_NULL;
}

bool Move::operator==(Move move) const
{
	return move._move == _move;
}

bool Move::operator!=(Move move) const
{
	return move._move != _move;
}

Move Move::castle(Color color, Side side)
{
	Square from = color == WHITE ? E1 : E8;
	Square to = side == KINGSIDE
		? (color == WHITE ? G1 : G8)
		: (color == WHITE ? C1 : C8);

	return Move(KING, from, to, side == KINGSIDE ? FLAG_KINGSIDE_CASTLE : FLAG_QUEENSIDE_CASTLE);
}


Move Move::nullMove()
{
	return Move(FLAG_NULL);
}

std::string Move::toAlgebraic() const
{
	std::stringstream ss;
	ss << SquareStr[from()] << SquareStr[to()];
	if (isPromotion())
		ss << pieceTypeToChar(promotion());
	return ss.str();
}