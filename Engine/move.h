#pragma once

#include <iostream>

#include "types.h"

class Board;

struct Move
{
public:
	Move();
	Move(PieceType piece_type, Square from, Square to, unsigned flags = 0);
	Move(PieceType piece_type, Square from, Square to, PieceType promotion, unsigned flags = 0);

	static Move parse(const Board & board, std::string algebraic);
	std::string toAlgebraic() const;

	PieceType pieceType() const;
	Square from() const;
	Square to() const;
	PieceType promotion() const;

	bool isDoublePush() const;
	bool isCapture() const;
	bool isEnPassant() const;
	bool isPromotion() const;
	bool isCastle() const;
	bool isCastle(Side side) const;
	bool isValid() const;
	bool isNull() const;
	bool isQuiet() const;

	bool operator==(Move move) const;
	bool operator!=(Move move) const;

	static Move castle(Color color, Side side);
	static Move nullMove();

private:
	Move(unsigned move);
	unsigned _move;
};

enum MoveFlags : unsigned {
	FLAG_DOUBLE_PUSH = 1 << 18,
	FLAG_CAPTURE = 1 << 19,
	FLAG_EN_PASSANT = 1 << 20,
	FLAG_KINGSIDE_CASTLE = 1 << 21,
	FLAG_QUEENSIDE_CASTLE = 1 << 22,
	FLAG_CASTLE = FLAG_KINGSIDE_CASTLE | FLAG_QUEENSIDE_CASTLE,
	FLAG_PROMOTION = 1 << 23,
	FLAG_INVALID = 1 << 24,
	FLAG_NULL = 1 << 25,
};

class MoveParseError : std::exception
{
public:
	MoveParseError(const char *msg) : exception(msg) {}
};