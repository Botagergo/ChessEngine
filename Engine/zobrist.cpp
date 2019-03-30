#include <random>
#include <ctime>

#include "zobrist.h"

namespace Zobrist
{
	u64 PiecePositionHash[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB] = { 0 };
	u64 BlackMovesHash = { 0 };
	u64 CastlingRightsHash[16] = { 0 };
	u64 EnPassantFileHash[FILE_NB] = { 0 };

	void initZobristHashing()
	{
		std::random_device rd;
		std::mt19937_64 e2(std::mt19937_64::default_seed);

		std::uniform_int_distribution<u64> dist;

		for (PieceType piece_type : PieceTypes)
		{
			for (Square square : Squares)
			{
				PiecePositionHash[WHITE][piece_type][square] = dist(e2);
				PiecePositionHash[BLACK][piece_type][square] = dist(e2);
			}
		}

		BlackMovesHash = dist(e2);

		u64 castling_rights_hash[COLOR_NB][SIDE_NB];
		for (Color color : Colors)
		{
			for (Side side : Sides)
			{
				castling_rights_hash[color][side] = dist(e2);
			}
		}

		for (int i = 0; i < 16; ++i)
		{
			for (Color color : Colors)
			{
				for (Side side : Sides)
				{
					if (i & Board::CastleFlag[color][side])
					{
						CastlingRightsHash[i] ^= castling_rights_hash[color][side];
					}
				}
			}
		}

		for (File file : Files)
		{
			EnPassantFileHash[file] = dist(e2);
		}
	}

	u64 getBoardHash(const Board & board)
	{
		u64 hash = 0;

		if (board.toMove() == BLACK)
			hash ^= BlackMovesHash;

		for (Color color : Colors)
		{
			for (PieceType piece_type : PieceTypes)
			{
				for (Square square : BitboardIterator<Square>(board.pieces(color, piece_type)))
				{
					hash ^= PiecePositionHash[color][piece_type][square];
				}
			}
		}

		hash ^= CastlingRightsHash[board._castling_rights];

		if (board.enPassantTarget() != NO_SQUARE)
			hash ^= EnPassantFileHash[Util::getFile(board.enPassantTarget())];

		return hash;
	}
}