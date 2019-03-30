#include "movegen.h"

int MoveGen::mvvlva(const Board & board, Move move)
{
	PieceType victim = toPieceType(board.pieceAt(move.to()));
	PieceType attacker = toPieceType(board.pieceAt(move.from()));
	return Evaluation::PieceValue[victim].mg - Evaluation::PieceValue[attacker].mg;
}