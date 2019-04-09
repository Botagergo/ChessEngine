#include <regex>
#include <sstream>

#include "attacks.h"
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

Move Move::fromAlgebraic(const Board & board, std::string algebraic)
{
	const static std::string square_regex = "([a-h][1-8])";
	const static std::string promotion_regex = "(n|b|r|q)?";
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

bool Move::isQuiet() const
{
	return !isCapture() && !isPromotion();
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
	ss << Constants::SquareStr[from()] << Constants::SquareStr[to()];
	if (isPromotion())
		ss << pieceTypeToChar(promotion());
	return ss.str();
}

bool isRank(char c)
{
	return '1' <= c && c <= '8';
}

bool isFile(char c)
{
	return 'a' <= c && c <= 'h';
}

Move Move::fromSan(const Board &board, const std::string &san)
{
	Square from = NO_SQUARE, to = NO_SQUARE;
	int flags = 0;

	std::string pawn_regex = "^(?:([a-h])x)?([a-h][1-8])(e\\.p)?(N|B|R|Q)?(\\+|#)?$";
	std::string piece_move_regex = "^(N|B|R|Q|K)([a-h])?([1-8])?(x)?([a-h][1-8])(\\+|#)?$";
	std::string king_castle_regex = "^0-0(\\+|#)?$";
	std::string queen_castle_regex = "^0-0-0(\\+|#)?$";

	std::smatch match;
	if (std::regex_search(san, match, std::regex(pawn_regex)))
	{
		to = parseSquare(match[2]);
		if (match[1].matched)
		{
			Bitboard pawns = board.pieces(board.toMove(), PAWN) & Constants::FileBB[charToFile(*match[1].first)];
			for (Square from : BitboardIterator<Square>(pawns))
			{
				if (board.attacked(from) & Constants::SquareBB[to])
				{
					flags = FLAG_CAPTURE;
					if (match[3].matched)
						flags |= FLAG_EN_PASSANT;
					if (match[4].matched)
						return Move(PAWN, from, to, charToPieceType(*match[4].first), flags);
					else
						return Move(PAWN, from, to, flags);
				}
			}
			return Move();
		}
		else
		{
			Bitboard pawns = board.pieces(board.toMove(), PAWN) & Constants::FileBB[charToFile(*match[2].first)];
			for (Bitboard from : BitboardIterator<Bitboard>(pawns))
			{
				if (board.toMove() == WHITE && (pawnSinglePushTargets<WHITE>(from, ~board.occupied()) & Constants::SquareBB[to])
					|| board.toMove() == BLACK && (pawnSinglePushTargets<BLACK>(from, ~board.occupied()) & Constants::SquareBB[to]))
				{
					if (match[4].matched)
						return Move(PAWN, Util::bitScanForward(from), to, charToPieceType(*match[4].first), 0);
					else
						return Move(PAWN, Util::bitScanForward(from), to, 0);
				}
				else if (board.toMove() == WHITE && (pawnDoublePushTargets<WHITE>(from, ~board.occupied()) & Constants::SquareBB[to])
					|| board.toMove() == BLACK && (pawnDoublePushTargets<BLACK>(from, ~board.occupied()) & Constants::SquareBB[to]))
				{
					return Move(PAWN, Util::bitScanForward(from), to, FLAG_DOUBLE_PUSH);
				}
			}
			return Move();
		}
	}
	else if (std::regex_search(san, match, std::regex(piece_move_regex)))
	{
		Bitboard pieces = board.pieces(board.toMove(), charToPieceType(*match[1].first));
		if (match[2].matched)
			pieces &= Constants::FileBB[charToFile(*match[2].first)];
		if (match[3].matched)
			pieces &= Constants::RankBB[charToRank(*match[3].first)];
		for (Square from : BitboardIterator<Square>(pieces))
		{
			if (board.attacked(from) & Constants::SquareBB[parseSquare(match[5])])
			{
				flags = 0;
				if (match[4].matched)
					flags = FLAG_CAPTURE;
				return Move(charToPieceType(*match[1].first), from, parseSquare(match[5]), flags);
			}
		}
		return Move();
	}
	else if (std::regex_search(san, match, std::regex(king_castle_regex)))
		return Move::castle(board.toMove(), KINGSIDE);
	else if (std::regex_search(san, match, std::regex(queen_castle_regex)))
		return Move::castle(board.toMove(), QUEENSIDE);

	return Move();
}

