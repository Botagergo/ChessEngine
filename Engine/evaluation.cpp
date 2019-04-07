#include "evaluation.h"

int Evaluation::islandCount(Bitboard pieces)
{
	Bitboard islands = Util::northFill(pieces) & Constants::RankBB[RANK_1];
	return Util::popCount(islands & (islands ^ (Util::shift<WEST>(islands))));
}

bool Evaluation::isIsolated(int file, Bitboard pieces)
{
	if (file == 0)
		return !(Constants::FileBB[file + 1] & pieces);
	if (file != 7)
		return !((Constants::FileBB[file - 1] | Constants::FileBB[file + 1]) & pieces);
	return !(Constants::FileBB[file - 1] & pieces);
}

#define S Score

const Score Evaluation::PieceValue[PIECE_TYPE_NB] = { S(100, 100), S(300, 300), S(320, 320), S(500, 500), S(900, 900), S(0, 0) };

const Score Evaluation::Mobility[PIECE_TYPE_NB][32] = {
	{},
	{ S(-19,-15), S(-12,-11), S(-6,-6), S(0, -1), S(6,  3), S(12, 7), // Knights
		S(15, 1), S(19, 13), S(19, 13) },
	{ S(-12,-15), S(-5,-8), S(1, -1), S(8, 6), S(15, 13), S(22, 20), // Bishops
		S(26, 26), S(32, 30), S(35, 32), S(37, 34), S(38, 35), S(39, 36),
		S(39, 37), S(40, 37), S(40, 38), S(40, 38) },
	{ S(-10,-18), S(-7,-18), S(-4, -1), S(-1, 6), S(2, 14), S(5, 23), // Rooks
		S(7, 15), S(9, 39), S(11, 47), S(13,53), S(13, 55), S(14,57),
		S(14,58), S(15,58), S(15,59), S(16,59) },
	{ S(-5,-2), S(-4,-6), S(-3, -3), S(-1, -1), S(0,  1), S(0,  4), // Queens
		S(1, 6), S(2, 9), S(4, 11), S(5, 13), S(6, 16), S(7, 17),
		S(8, 17), S(8, 17), S(9, 17), S(10, 17), S(10, 17), S(10, 17),
		S(10, 17), S(10, 17), S(10, 17), S(10, 17), S(10, 17), S(10, 17),
		S(10, 17), S(10, 17), S(10, 17), S(10, 17), S(10, 17), S(10, 17),
		S(10, 17), S(10, 17) },
	{}
};

const Score Evaluation::TempoBonus = S(4, 4);

const Score Evaluation::BishopPair = S(10, 50);

const Score Evaluation::PawnIslands[5] = {
	S(0, 0), S(0, 0), S(-5, -8), S(-7, -10), S(-9, -12)
};

const Score Evaluation::PassedPawn[8] = {
	S(0, 0), S(4, 17), S(7, 20), S(14, 36), S(42, 62), S(72, 120), S(110, 190)
};

const Score Evaluation::DoubledPawn[8] = {
	S(-13, -43), S(-20, -48), S(-23, -48), S(-23, -48),
	S(-23, -48), S(-23, -48), S(-20, -48), S(-13, -43)
};

const Score Evaluation::IsolatedPawn[8] = {
	S(-5, -5),		S(-7, -7),		S(-10, -10),	S(-15, -15),
	S(-15, -15),	S(-10, -10),	S(-7, -7),		S(-5, -5)
};

const Score Evaluation::HalfOpenFileBonus = S(8, 0);

const Score Evaluation::OpenFileBonus = S(16, 0);

const int Evaluation::KingAttacks[200] = {
	0, 1, 2, 3, 4, 6, 7, 8, 10, 11,
	13, 14, 16, 18, 20, 23, 26, 29, 33, 37,
	42, 46, 51, 58, 65, 74, 84, 96, 110, 125,
	143, 162, 183, 205, 228, 252, 277, 301, 325, 348,
	369, 389, 407, 423, 437, 450, 461, 470, 478, 485,
	491, 496, 500, 503, 506, 508, 510, 512, 513, 514,
	515, 516, 517, 517, 518, 518, 518, 518, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
	519, 519, 519, 519, 519, 519, 519, 519, 519, 519,
};

#undef S

const int Evaluation::KingAttacksWeight[PIECE_TYPE_NB] = { 0, 2, 2, 4, 6, 0 };