#pragma once

#include <vector>

#include "attacks.h"
#include "board.h"
#include "evaluation.h"

Bitboard _getAttackers(const Board & board, Color to_move, Bitboard occupied, Square square);
Square _getLeastValuableAttacker(const Board & board, Color to_move, Bitboard attackers);

template <Color toMove>
int see(const Board & board, Move move)
{
	if (board.pieceAt(move.to()) == NO_PIECE)
		return 0;

	Square attacker[2];
	Bitboard occupied[2];
	occupied[toMove] = board.occupied(toMove) ^ Constants::SquareBB[move.from()];
	occupied[~toMove] = board.occupied(~toMove);

	std::array<int, 30> scores;
	int scores_size = 1;

	PieceType curr_piece = move.pieceType();

	scores[0] = Evaluation::PieceValue[toPieceType(board.pieceAt(move.to()))].mg;

	Bitboard attackers_bb[2];
	Color color = toMove;

	for (int i = 1;; i += 2)
	{
		attackers_bb[~color] = _getAttackers(board, ~color, occupied[~color] | occupied[color], move.to()) & occupied[~color];
		if (!attackers_bb[~color])
			break;

		scores[i] = scores[i - 1] - Evaluation::PieceValue[curr_piece].mg;
		scores_size++;

		attacker[~color] = _getLeastValuableAttacker(board, ~color, attackers_bb[~color]);
		occupied[~color] ^= Constants::SquareBB[attacker[~color]];

		attackers_bb[color] = _getAttackers(board, color, occupied[color] | occupied[~color], move.to()) & occupied[color];
		if (!attackers_bb[color])
			break;

		scores[i + 1] = scores[i] + Evaluation::PieceValue[toPieceType(board.pieceAt(attacker[~color]))].mg;
		scores_size++;

		attacker[color] = _getLeastValuableAttacker(board, color, attackers_bb[color]);
		occupied[color] ^= Constants::SquareBB[attacker[color]];

		curr_piece = toPieceType(board.pieceAt(attacker[color]));
	}

	int best = -SCORE_INFINITY, curr = 0, i;
	for (i = 1; i < scores_size; i += 2)
	{
		if (scores[i] > best)
			best = scores[i];
	}

	if (i == scores_size && scores[i - 1] > best)
		best = scores[i - 1];

	return best;
}