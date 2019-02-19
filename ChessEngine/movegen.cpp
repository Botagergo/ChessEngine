#include "movegen.h"

//int Move_Gen::getScore(const Board & board, const Move & move)
//{
//	if (move.promotion != NO_PIECE)
//		return Evaluation::PieceValue[move.promotion];
//	else
//	{
//		PieceType victim = toPieceType(board.pieceAt(move.to));
//		PieceType attacker = toPieceType(board.pieceAt(move.from));
//		return Evaluation::PieceValue[victim] - Evaluation::PieceValue[attacker];
//	}
//}