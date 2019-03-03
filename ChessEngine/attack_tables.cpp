#include "attack_tables.h"
#include "constants.h"
#include "util.h"

#include <iostream>

using namespace Constants;

Bitboard KnightAttackTable[SQUARE_NB] = {};
Bitboard KingAttackTable[SQUARE_NB] = {};
Bitboard SlidingAttackTable[DIRECTION_NB][SQUARE_NB] = {};

void initAttackTables() {
	for (int i = 0; i < SQUARE_NB; ++i)
	{
		Bitboard knight = SquareBB[i];
		Bitboard attacks = C64(0);

		attacks |= knight << 15 & NotHFileBB;
		attacks |= knight << 6 & NotGHFileBB;
		attacks |= knight >> 10 & NotGHFileBB;
		attacks |= knight >> 17 & NotHFileBB;
		attacks |= knight >> 15 & NotAFileBB;
		attacks |= knight >> 6 & NotABFileBB;
		attacks |= knight << 10 & NotABFileBB;
		attacks |= knight << 17 & NotAFileBB;

		KnightAttackTable[i] = attacks;
	}

	for (int i = 0; i < SQUARE_NB; ++i)
	{
		Bitboard king = SquareBB[i];

		Bitboard attacks = Util::shift<NORTH>(king) | Util::shift<NORTHEAST>(king) |
			Util::shift<EAST>(king)  | Util::shift<SOUTHEAST>(king) |
			Util::shift<SOUTH>(king) | Util::shift<SOUTHWEST>(king) |
			Util::shift<WEST>(king)  | Util::shift<NORTHWEST>(king);

		KingAttackTable[i] = attacks;
	}

	//------------------------
	//north ray
	//------------------------
	Bitboard ray = FileBB[A_FILE] ^ SquareBB[A1];

	for (int i = 0; i < SQUARE_NB; ++i, ray <<= 1)
	{
		SlidingAttackTable[NORTH][i] = ray;
	}

	//------------------------
	//northwest ray
	//------------------------
	ray = C64(0x0102040810204000);

	for (int file = 7; file >= 0; --file, ray = Util::shift<WEST>(ray))
	{
		Bitboard n = ray;
		for (int rank = 0; rank < SQUARE_NB; rank += 8, n <<= 8)
		{
			SlidingAttackTable[NORTHWEST][rank + file] = n;
		}
	}

	//------------------------
	//west ray
	//------------------------
	ray = RankBB[RANK_1] ^ SquareBB[H1];

	for (int rank = 0; rank < SQUARE_NB; rank += 8, ray <<= 8)
	{
		Bitboard n = ray;
		for (int file = 7; file >= 0; --file, n = Util::shift<WEST>(n))
		{
			SlidingAttackTable[WEST][rank + file] = n;
		}
	}

	//------------------------
	//southwest ray
	//------------------------
	ray = C64(0x40201008040201);

	for (int file = 7; file >= 0; --file, ray = Util::shift<WEST>(ray))
	{
		Bitboard n = ray;
		for (int rank = 56; rank >= 0; rank -= 8, n >>= 8)
		{
			SlidingAttackTable[SOUTHWEST][rank + file] = n;
		}
	}

	//------------------------
	//south ray
	//------------------------
	ray = FileBB[H_FILE] ^ SquareBB[H8];

	for (int i = 63; i >= 0; --i, ray >>= 1)
		SlidingAttackTable[SOUTH][i] = ray;

	//------------------------
	//southeast ray
	//------------------------
	ray = C64(0x2040810204080);

	for (int file = 0; file < 8; ++file, ray = Util::shift<EAST>(ray))
	{
		Bitboard n = ray;
		for (int rank = 56; rank >= 0; rank -= 8, n >>= 8)
		{
			SlidingAttackTable[SOUTHEAST][rank + file] = n;
		}
	}

	//------------------------
	//east ray
	//------------------------
	ray = RankBB[RANK_1] ^ SquareBB[A1];

	for (int rank = 0; rank < SQUARE_NB; rank += 8, ray <<= 8)
	{
		Bitboard n = ray;
		for (int file = 0; file < 8; ++file, n = Util::shift<EAST>(n))
		{
			SlidingAttackTable[EAST][rank + file] = n;
		}
	}

	//------------------------
	//northeast ray
	//------------------------
	ray = C64(0x8040201008040200);

	for (int file = 0; file < 8; ++file, ray = Util::shift<EAST>(ray))
	{
		Bitboard n = ray;
		for (int rank = 0; rank < 8 * 8; rank += 8, n <<= 8)
		{
			SlidingAttackTable[NORTHEAST][rank + file] = n;
		}
	}

	std::cout << SlidingAttackTable[NORTH][A1] << std::endl;
}