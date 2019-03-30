#pragma once

#include <limits>

#include "bitboard.h"
#include "types.h"

namespace Constants
{
	const Bitboard FileBB[] =
	{
		C64(0x0101010101010101),
		C64(0x0202020202020202),
		C64(0x0404040404040404),
		C64(0x0808080808080808),
		C64(0x1010101010101010),
		C64(0x2020202020202020),
		C64(0x4040404040404040),
		C64(0x8080808080808080),
	};

	const Bitboard RankBB[] =
	{
		C64(0x00000000000000FF),
		C64(0x000000000000FF00),
		C64(0x0000000000FF0000),
		C64(0x00000000FF000000),
		C64(0x000000FF00000000),
		C64(0x0000FF0000000000),
		C64(0x00FF000000000000),
		C64(0xFF00000000000000),
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

	const Bitboard EmptyBB = C64(0);
	const Bitboard FullBB = std::numeric_limits<unsigned long long>::max();

	const Bitboard NotAFileBB = ~FileBB[A_FILE];
	const Bitboard NotHFileBB = ~FileBB[H_FILE];
	const Bitboard NotBFileBB = ~FileBB[B_FILE];
	const Bitboard NotGFileBB = ~FileBB[G_FILE];
	const Bitboard NotABFileBB = NotAFileBB & NotBFileBB;
	const Bitboard NotGHFileBB = NotGFileBB & NotHFileBB;

	void initSquareBB();
}