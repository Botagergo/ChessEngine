#pragma once

#include <string>

typedef unsigned long long Bitboard;
typedef unsigned long long u64;

enum Direction
{
	NORTH, NORTHWEST, WEST, SOUTHWEST, SOUTH, SOUTHEAST,EAST, NORTHEAST,
	DIRECTION_NB
};

enum File {
	NO_FILE = -1, A_FILE, B_FILE, C_FILE, D_FILE, E_FILE, F_FILE, G_FILE, H_FILE, FILE_NB
};

enum Rank {
	NO_RANK = -1, RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
};

enum Color {
	NO_COLOR = -1, WHITE, BLACK, COLOR_NB
};

const Color Colors[] = { WHITE, BLACK };

constexpr Color operator ~(Color other)
{
	return Color(other ^ BLACK);
}

enum Side
{
	KINGSIDE, QUEENSIDE, SIDE_NB
};

enum PieceType {
	NO_PIECE_TYPE = -1, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECE_TYPE_NB
};

enum Piece {
	NO_PIECE = -1,
	WHITE_PAWN, BLACK_PAWN,
	WHITE_KNIGHT, BLACK_KNIGHT,
	WHITE_BISHOP, BLACK_BISHOP,
	WHITE_ROOK, BLACK_ROOK,
	WHITE_QUEEN, BLACK_QUEEN,
	WHITE_KING, BLACK_KING,
	PIECE_NB
};

PieceType toPieceType(Piece piece);
Piece toPiece(PieceType piece_type, Color color);

PieceType charToPieceType(char c);
Piece charToPiece(char c);

char pieceTypeToChar(PieceType p);
char pieceToChar(Piece p);

File charToFile(char f);
Rank charToRank(char r);

Color pieceColor(Piece piece);

const PieceType PieceTypes[] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
const PieceType MinorMajorPieces[] = { KNIGHT, BISHOP, ROOK, QUEEN };

enum Square {
	NO_SQUARE = -1,
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	SQUARE_NB
};

const Square Squares [] = {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
};

const Side Sides[] = {
	KINGSIDE, QUEENSIDE
};

const File Files[] = {
	A_FILE, B_FILE, C_FILE, D_FILE, E_FILE, F_FILE, G_FILE, H_FILE
};

Square parseSquare(const std::string &s);

Piece operator++(Piece &piece);
PieceType operator++(PieceType &piece);

Square operator++(Square &square);

File operator++(File &file);
Rank operator++(Rank &rank);

struct Score
{
	explicit Score(int mg = 0, int eg = 0) : mg(mg), eg(eg) {}

	int mg;
	int eg;

	void operator+=(const Score &s) { mg += s.mg; eg += s.eg; }
	void operator-=(const Score &s) { mg -= s.mg; eg -= s.eg; }

	Score operator-() const { return Score(-mg, -eg); }
};

Score operator*(double d, const Score &score);
Score operator*(const Score &score, double d);

enum ScoreConstant
{
	SCORE_DRAW = 0,
	SCORE_KILLER = 150,
	SCORE_MIN_MATE = 32667,
	SCORE_MAX_MATE = 32767,
	SCORE_INFINITY = 32768,
	SCORE_INVALID = 32769
};

enum ScoreType
{
	EXACT,
	LOWER_BOUND,
	UPPER_BOUND
};