#pragma once

#include "types.h"

enum MoveFlags { CAPTURE = 1, EN_PASSANT = 2, KINGSIDE_CASTLE = 4, QUEENSIDE_CASTLE = 8, CASTLE = 12, NULL_MOVE = 16, DOUBLE_PUSH = 32};

struct Move
{
	Move(Piece piece_type, Square from, Square to, int flags=0, Piece promotion=NO_PIECE) : 
		piece_type(piece_type), from(from), to(to), flags(flags), promotion(promotion) {}
	Move() : Move(NO_PIECE, NO_SQUARE, NO_SQUARE) {}

	Piece piece_type;
	Square from;
	Square to;
	int flags;
	Piece promotion;
};

