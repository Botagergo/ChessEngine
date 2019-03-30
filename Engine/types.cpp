#include <cctype>
#include <string>
#include <cassert>

#include "types.h"

PieceType toPieceType(Piece piece)
{
	return (PieceType)(piece >> 1);
}

Piece toPiece(PieceType piece_type, Color color)
{
	return (Piece)((piece_type << 1) + color);
}

PieceType charToPieceType(char c)
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
		return NO_PIECE_TYPE;
	}
}

Piece charToPiece(char c)
{
	Piece piece;
	switch (tolower(c))
	{
	case 'p':
		piece = WHITE_PAWN;
		break;
	case 'n':
		piece = WHITE_KNIGHT;
		break;
	case 'b':
		piece = WHITE_BISHOP;
		break;
	case 'r':
		piece = WHITE_ROOK;
		break;
	case 'q':
		piece = WHITE_QUEEN;
		break;
	case 'k':
		piece = WHITE_KING;
		break;
	default:
		return NO_PIECE;
	}

	if (islower(c))
		++piece;

	return piece;
}

char pieceTypeToChar(PieceType p)
{
	if (p <= NO_PIECE_TYPE || PIECE_TYPE_NB <= p)
		return '0';

	return pieceToChar(toPiece(p, BLACK));
}

char pieceToChar(Piece p)
{
	if (p <= NO_PIECE || PIECE_NB <= p)
		return '0';

	const static char map[PIECE_NB] = { 'P', 'p', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};
	return map[p];
}

Color pieceColor(Piece piece)
{
	if (piece <= NO_PIECE || PIECE_NB <= piece)
		return NO_COLOR;

	return (Color)(piece % 2);
}


Square parseSquare(const std::string &s)
{
	if (s.size() != 2 || s[0] < 'a' || 'h' < s[0] || s[1] < '1' || '8' < s[1])
		return NO_SQUARE;

	int col = s[0] - 'a';
	int row = s[1] - '1';
	return Square(8 * row + col);
}

PieceType operator++(PieceType &piece)
{
	piece = static_cast<PieceType>(static_cast<int>(piece) + 1);
	return piece;
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

File operator++(File &file)
{
	file = static_cast<File>(static_cast<int>(file) + 1);
	return file;
}

Rank operator++(Rank &rank)
{
	rank = static_cast<Rank>(static_cast<int>(rank) + 1);
	return rank;
}

Score operator*(double d, const Score &score)
{
	return Score((int)(score.mg * d), (int)(score.eg * d));
}

Score operator*(const Score &score, double d)
{
	return Score((int)(score.mg * d), (int)(score.eg * d));
}

