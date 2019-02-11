#pragma once

#pragma warning( disable : 4715)

#include <cstdlib>

#include "bitboard.h"
#include "constants.h"
#include "types.h"

// Eltolja a táblát egyel a megadott irányba, forgatás nélkül
//
// dir		-	melyik irányba tolja el
// b		-	az eltolandó tábla
// return	-	az eltolt tábla
template <Direction dir>
Bitboard shift(Bitboard b)
{
	static_assert(dir != DIRECTION_NB);

	switch (dir)
	{
	case NORTH:
		return (b << 8);
	case NORTHEAST:
		return (b << 9) & NotAFileBB;
	case EAST:
		return (b << 1) & NotAFileBB;
	case SOUTHEAST:
		return (b >> 7) & NotAFileBB;
	case SOUTH:
		return (b >> 8);
	case SOUTHWEST:
		return (b >> 9) & NotHFileBB;
	case WEST:
		return (b >> 1) & NotHFileBB;
	case NORTHWEST:
		return (b << 7) & NotHFileBB;
	}
}

// Megfordítja a tábla sorainak sorrendjét
//
// bb		-	a megfordítandó tábla
// return	-	a megfordított tábla
Bitboard verticalFlip(Bitboard bb);

// Törli a legkisebb helyiértékű bitet
void clearLSB(Bitboard &bb);

// Visszaadja legkisebb helyiértékű bitet
Bitboard isolateLSB(Bitboard b);

// Visszaadja a legkisebb helyiértékű bitnek megfelelő mezőt
Square bitScanForward(Bitboard bb);

// Visszaadja a legkisebb helyiértékű bitnek megfelelő mezőt,
// ezzel együtt törli is a bitet
Square bitScanForwardPop(Bitboard &b);

// Visszaadja a legnagyobb helyiértékű bitnek megfelelő mezőt
Square bitScanReverse(Bitboard bb);