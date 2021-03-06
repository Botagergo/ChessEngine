#pragma once

#include "attack_tables.h"
#include "board.h"
#include "piece_square_table.h"
#include "types.h"

namespace Evaluation
{
	extern const Score PieceValue[PIECE_TYPE_NB];
	extern const Score Mobility[PIECE_TYPE_NB][32];
	extern const Score TempoBonus;
	extern const Score BishopPair;
	extern const Score PawnIslands[5];
	extern const Score PassedPawn[8];
	extern const Score DoubledPawn[8];
	extern const Score IsolatedPawn[8];
	extern const Score HalfOpenFileBonus;
	extern const Score OpenFileBonus;
	extern const int KingAttacks[200];
	extern const int KingAttacksWeight[PIECE_TYPE_NB];


	template <Color color>
	bool isPassedPawn(Square pawn, Bitboard enemy_pawns)
	{
		Bitboard enemy_attacks = Attacks::pawnAttacks<~color>(enemy_pawns);

		if (Constants::SquareBB[pawn] & enemy_attacks)
			return false;

		File pawn_file = Util::getFile(pawn);
		Bitboard attacks_on_file = (enemy_attacks | enemy_pawns | Constants::SquareBB[pawn]) & Constants::FileBB[pawn_file];

		if (color == WHITE)
			return Util::bitScanReverse(attacks_on_file) == pawn;
		else
			return Util::bitScanForward(attacks_on_file) == pawn;
	}

	bool isIsolated(int file, Bitboard pieces);
	int islandCount(Bitboard pieces);

	template <Color color>
	Score evaluatePawnStructure(const Board & board)
	{
		Score score = PawnIslands[islandCount(board.pieces(color, PAWN))];

		for (File file = A_FILE; file < FILE_NB; ++file)
		{
			Bitboard pawns = board.pieces(color, PAWN) & Constants::FileBB[file];

			if (!pawns)
				continue;

			if (isIsolated(file, board.pieces(color, PAWN)))
				score += IsolatedPawn[file];

			if (Util::popCount(Constants::FileBB[file] & pawns) >= 2)
				score += DoubledPawn[file];

			for (Square pawn : BitboardIterator<Square>(pawns))
			{
				if (isPassedPawn<color>(pawn, board.pieces(~color, PAWN)))
					score += PassedPawn[Util::relativeRank<color>(Util::getRank(pawn))];
			}
		}

		return score;
	}

	template <Color color>
	Score evaluateRooks(const Board & board)
	{
		Score score;

		for (Square rook : BitboardIterator<Square>(board.pieces(color, ROOK)))
		{
			Bitboard front_squares = SlidingAttackTable[Util::forwardDirection<color>()][rook];

			if (!(board.pieces(color, PAWN) & front_squares))
			{
				if (!(board.pieces(~color, PAWN) & front_squares))
					score += OpenFileBonus;
				else
					score += HalfOpenFileBonus;
			}
		}

		return score;
	}
	template <Color color>
	Score evaluateKingSafety(const Board & board)
	{
		Score score;

		const static Bitboard KingShield[] = { 0xe000, 0xe0000000000000 };
		const static Bitboard QueenShield[] = { 0x700, 0x7000000000000 };

		int shelter1 = 0, shelter2 = 0;

		int king_file = Util::getFile(Util::bitScanForward(board.pieces(color, KING)));
		Bitboard pawns = board.pieces(color, PAWN);

		if (king_file > E_FILE)
		{
			shelter1 = Util::popCount(pawns & KingShield[color]);
			shelter2 = Util::popCount(pawns & Util::shift<Util::forwardDirection<color>()>(KingShield[color]));
		}
		else if (king_file < D_FILE)
		{
			shelter1 = Util::popCount(pawns & QueenShield[color]);
			shelter2 = Util::popCount(pawns & Util::shift<Util::forwardDirection<color>()>(QueenShield[color]));
		}

		score.mg += (int)(shelter1 * 5.5 + shelter2 * 2.0);
		score.eg += shelter1 + shelter2;

		return score;
	}


	template <Color color>
	int evaluate(const Board & board)
	{
		if (board.isDraw())
			return 0;

		Score score;

		Square king_square[COLOR_NB];
		king_square[color] = Util::bitScanForward(board.pieces(color, KING));
		king_square[~color] = Util::bitScanForward(board.pieces(~color, KING));

		Bitboard king_proximity[COLOR_NB];
		king_proximity[color] = Attacks::kingAttacks(king_square[color]);
		king_proximity[~color] = Attacks::kingAttacks(king_square[~color]);

		int king_attacks_count[COLOR_NB] = { 0 };

		for (PieceType piece_type = PAWN; piece_type <= QUEEN; ++piece_type)
		{
			score += PieceValue[piece_type] * Util::popCount(board.pieces(color, piece_type));
			score -= PieceValue[piece_type] * Util::popCount(board.pieces(~color, piece_type));
		}

		for (PieceType piece_type = PAWN; piece_type < PIECE_TYPE_NB; ++piece_type)
		{
			for (Square square : BitboardIterator<Square>(board.pieces(color, piece_type)))
			{
				int distance = DistanceTable[square][king_square[~color]];
				Bitboard attacked = board.attacked(square);
				king_attacks_count[~color] += KingAttacksWeight[piece_type] * Util::popCount(king_proximity[~color] & attacked) + (7 - distance);

				score += pieceSquareValue<color>(piece_type, square);
				score += Mobility[piece_type][Util::popCount(board.attacked(square) & ~board.occupied())];
			}

			for (Square square : BitboardIterator<Square>(board.pieces(~color, piece_type)))
			{
				int distance = DistanceTable[square][king_square[color]];
				Bitboard attacked = board.attacked(square);
				king_attacks_count[color] += KingAttacksWeight[piece_type] * Util::popCount(king_proximity[color] & attacked) + (7 - distance);

				score -= pieceSquareValue<~color>(piece_type, square);
				score -= Mobility[piece_type][Util::popCount(board.attacked(square) & ~board.occupied())];
			}
		}

		int our_pinned_score = 10 * Util::popCount(board.pinnedPieces(color));
		int their_pinned_score = 10 * Util::popCount(board.pinnedPieces(~color));

		score -= Score(our_pinned_score, our_pinned_score);
		score += Score(their_pinned_score, their_pinned_score);

		score += evaluateKingSafety<color>(board);
		score -= evaluateKingSafety<~color>(board);

		score += evaluatePawnStructure<color>(board);
		score -= evaluatePawnStructure<~color>(board);

		score += evaluateRooks<color>(board);
		score -= evaluateRooks<~color>(board);

		score -= Score(KingAttacks[king_attacks_count[color]], KingAttacks[king_attacks_count[color]] / 2);
		score += Score(KingAttacks[king_attacks_count[~color]], KingAttacks[king_attacks_count[~color]] / 2);

		if(board.toMove() == color)
			score += TempoBonus;
		else
			score -= TempoBonus;

		return (score.mg * (256 - board.phase()) + score.eg * board.phase()) / 256;
	}
}