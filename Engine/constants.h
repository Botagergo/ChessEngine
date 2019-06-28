#pragma once

#include <string>

#include "types.h"

namespace Constants
{
	const Bitboard FileBB[] =
	{
		0x0101010101010101ULL,
		0x0202020202020202ULL,
		0x0404040404040404ULL,
		0x0808080808080808ULL,
		0x1010101010101010ULL,
		0x2020202020202020ULL,
		0x4040404040404040ULL,
		0x8080808080808080ULL,
	};

	const Bitboard RankBB[] =
	{
		0x00000000000000FFUll,
		0x000000000000FF00Ull,
		0x0000000000FF0000Ull,
		0x00000000FF000000Ull,
		0x000000FF00000000Ull,
		0x0000FF0000000000Ull,
		0x00FF000000000000Ull,
		0xFF00000000000000Ull,
	};

	extern Bitboard SquareBB[SQUARE_NB];

	const std::string SquareStr[64] = {
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
		"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
		"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
		"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
		"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
		"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
		"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
		"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
	};

	const Bitboard EmptyBB = 0ULL;
	const Bitboard FullBB = std::numeric_limits<u64>::max();

	const Bitboard NotAFileBB = ~FileBB[A_FILE];
	const Bitboard NotHFileBB = ~FileBB[H_FILE];
	const Bitboard NotBFileBB = ~FileBB[B_FILE];
	const Bitboard NotGFileBB = ~FileBB[G_FILE];
	const Bitboard NotABFileBB = NotAFileBB & NotBFileBB;
	const Bitboard NotGHFileBB = NotGFileBB & NotHFileBB;

	void initSquareBB();
}