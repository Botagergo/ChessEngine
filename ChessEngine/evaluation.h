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

	template <Color color>
	bool isPassedPawn(Square pawn, Bitboard enemy_pawns)
	{
		Bitboard enemy_attacks = pawnAttacks<~color>(enemy_pawns);

		if (SquareBB[pawn] & enemy_attacks)
			return false;

		File pawn_file = Util::getFile(pawn);
		Bitboard attacks_on_file = (enemy_attacks | enemy_pawns | SquareBB[pawn]) & FileBB[pawn_file];

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

		for (int file = 0; file < FILE_NB; ++file)
		{
			Bitboard pawns = board.pieces(color, PAWN) & FileBB[file];

			if (!pawns)
				continue;

			if (isIsolated(file, board.pieces(color, PAWN)))
				score += IsolatedPawn[file];

			if (Util::popCount(FileBB[file] & pawns) >= 2)
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
			Bitboard front_squares = color == WHITE ? SlidingAttackTable[SOUTH][rook] : SlidingAttackTable[NORTH][rook];

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

		constexpr Direction shift_dir = color == WHITE ? NORTH : SOUTH;

		int king_file = Util::getFile(Util::bitScanForward(board.pieces(color, KING)));
		Bitboard pawns = board.pieces(color, PAWN);

		if (king_file > E_FILE)
		{
			shelter1 = Util::popCount(pawns & KingShield[color]);
			shelter2 = Util::popCount(pawns & Util::shift<shift_dir>(KingShield[color]));
		}
		else if (king_file < D_FILE)
		{
			shelter1 = Util::popCount(pawns & QueenShield[color]);
			shelter2 = Util::popCount(pawns & Util::shift<shift_dir>(QueenShield[color]));
		}

		score.mg += (int)(shelter1 * 5.5 + shelter2 * 2);
		score.eg += shelter1 + shelter2;

		return score;
	}


	template <Color color>
	int evaluate(const Board & board)
	{
		Score score;

		for (PieceType piece_type : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN})
		{
			score += PieceValue[piece_type] * Util::popCount(board.pieces(color, piece_type));
			score -= PieceValue[piece_type] * Util::popCount(board.pieces(~color, piece_type));
		}

		for (PieceType piece_type : PieceTypes)
		{
			for (Square square : BitboardIterator<Square>(board.pieces(color, piece_type)))
			{
				score += pieceSquareValue<color>(piece_type, square);
				score += Mobility[piece_type][Util::popCount(board.attacked(square))];
			}

			for (Square square : BitboardIterator<Square>(board.pieces(~color, piece_type)))
			{
				score -= pieceSquareValue<~color>(piece_type, square);
				score -= Mobility[piece_type][Util::popCount(board.attacked(square))];
			}
		}

		int our_pinned_score = 10 * Util::popCount(board.pinnedPieces<color>());
		int their_pinned_score = 10 * Util::popCount(board.pinnedPieces<~color>());

		score += evaluateKingSafety<color>(board);
		score -= evaluateKingSafety<~color>(board);

		score -= Score(our_pinned_score, our_pinned_score);
		score += Score(their_pinned_score, their_pinned_score);

		score += evaluatePawnStructure<color>(board);
		score -= evaluatePawnStructure<~color>(board);

		score += evaluateRooks<color>(board);
		score -= evaluateRooks<~color>(board);

		if(board.toMove() == color)
			score += TempoBonus;
		else
			score -= TempoBonus;

		return (score.mg * (256 - board.phase()) + score.eg * board.phase()) / 256;
	}
}