#pragma once

#include "attacks.h"
#include "board.h"
#include "evaluation.h"
#include "see.h"

Bitboard _getAttackers(const Board & board, Color to_move, Bitboard occupied, Square square);

Square _getLeastValuableAttacker(const Board & board, Color to_move, Bitboard attackers);

template <Color toMove>
int see(const Board & board, Move move)
{
	if (board.pieceAt(move.to()) == NO_PIECE)
		return 0;

	Square attacker[2];
	Bitboard occupied[2];
	occupied[toMove] = board.occupied(toMove) ^ SquareBB[move.from()];
	occupied[~toMove] = board.occupied(~toMove);

	std::vector<int> scores;
	PieceType curr_piece = move.pieceType();

	scores.push_back(Evaluation::PieceValue[toPieceType(board.pieceAt(move.to()))].mg);

	Bitboard attackers_bb[2];
	Color color = toMove;

	for (int i = 1;; i += 2)
	{
		attackers_bb[~color] = _getAttackers(board, ~color, occupied[~color] | occupied[color], move.to()) & occupied[~color];
		if (!attackers_bb[~color])
			break;

		scores.push_back(scores[i - 1] - Evaluation::PieceValue[curr_piece].mg);
		attacker[~color] = _getLeastValuableAttacker(board, ~color, attackers_bb[~color]);
		occupied[~color] ^= SquareBB[attacker[~color]];

		attackers_bb[color] = _getAttackers(board, color, occupied[color] | occupied[~color], move.to()) & occupied[color];
		if (!attackers_bb[color])
			break;

		scores.push_back(scores[i] + Evaluation::PieceValue[toPieceType(board.pieceAt(attacker[~color]))].mg);
		attacker[color] = _getLeastValuableAttacker(board, color, attackers_bb[color]);
		occupied[color] ^= SquareBB[attacker[color]];

		curr_piece = toPieceType(board.pieceAt(attacker[color]));
	}

	int best = 0, curr = 0, i;
	for (i = 1; i < scores.size(); i += 2)
	{
		if (scores[i] > best)
			best = scores[i];
	}

	if (i == scores.size() && scores[i - 1] > best)
		best = scores[i - 1];

	return best;
}