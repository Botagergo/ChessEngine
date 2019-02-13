#include <cctype>
#include <string>
#include <cassert>

#include "types.h"

Piece charToPiece(char c)
{
	switch (tolower(c))
	{
	case 'p':
		return PAWN;
	case 'n':
		return KNIGHT;
	case 'b':
		return BISHOP;
	case 'r':
		return ROOK;
	case 'q':
		return QUEEN;
	case 'k':
		return KING;
	default:
		return NO_PIECE;
	}
}

char pieceToChar(Piece p)
{
	if ((int)p < 0 || 5 < (int)p)
		return '0';

	char map[PIECE_NB] = { 'p', 'n', 'b', 'r', 'q', 'k'};
	return map[p];
}

Square parseSquare(const std::string &s)
{
	if (s.size() != 2 || s[0] < 'a' || 'h' < s[0] || s[1] < '1' || '8' < s[1])
		return NO_SQUARE;

	int col = s[0] - 'a';
	int row = s[1] - '1';
	return Square(8 * row + col);
}

Piece operator++(Piece &piece)
{
	piece = static_cast<Piece>(static_cast<int>(piece) + 1);
	return piece;
}

Square operator++(Square &square)
{
	square = static_cast<Square>(static_cast<int>(square) + 1);
	return square;
}
