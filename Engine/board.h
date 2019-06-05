#pragma once

#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "attack_tables.h"
#include "bitboard_iterator.h"
#include "config.h"
#include "move.h"
#include "types.h"

class FenParseError : std::exception
{
public:
	FenParseError(const char *msg) : exception(msg) {}
};

class Board
{
public:
	static Board fromFen(const std::string &fen);
	Board();

	std::string fen() const;

	void print(std::ostream &os) const;

	Color toMove() const;

	bool makeMove(Move move);

	Bitboard pieces(Color color, PieceType piece) const;
	Piece pieceAt(Square square) const;

	int material(Color color, PieceType piece_type) const;

	Bitboard occupied(Color color) const;
	Bitboard occupied() const;

	Bitboard attacked(Color color) const;
	Bitboard attacked(Square square) const;

	Bitboard pinnedPieces(Color color) const;

	int phase() const;

	bool canCastle(Color color, Side side) const;

	Square enPassantTarget() const;
	Square enPassantCaptureTarget() const;

	Square kingSquare(Color color) const;
	bool isInCheck(Color color) const;

	bool allowNullMove() const;

	int halfmoveClock() const;
	int fullmoveNum() const;

	u64 hash() const;

	Board flip() const;

	const static int AllCastlingRights;
	const static int CastleFlag[COLOR_NB][SIDE_NB];

	unsigned char _castling_rights;

private:
	void _castle(Side side);
	void _updateCastlingRights(Move move);
	void _makeNormalMove(Move move);

	template <Color color>
	Bitboard _pinnedPieces() const;

	Color _to_move;

	std::array<std::array<Bitboard, PIECE_TYPE_NB>, COLOR_NB> _pieces;
	std::array<Piece, SQUARE_NB> _piece_list;
	std::array<Bitboard, COLOR_NB> _occupied;
	std::array<Bitboard, SQUARE_NB> _attackedByPiece;
	std::array<Bitboard, COLOR_NB> _attackedByColor;
	std::array<Bitboard, COLOR_NB> _pinned_pieces;
	int _material[COLOR_NB][PIECE_TYPE_NB];
	
	Square _en_passant_target;
	Square _en_passant_capture_target;

	int _halfmove_clock;
	int _fullmove_num;

	u64 _hash;

	void _initOccupied();
	void _initPieceList();
	void _initMaterial();
	void _updateAttacked();
};

template <Color color>
Bitboard Board::_pinnedPieces() const
{
	assert(pieces(color, KING) != 0Ull);

	Square king_square = Util::bitScanForward(pieces(color, KING));
	Bitboard pinners = ((pieces(~color, ROOK) | pieces(~color, QUEEN)) & Attacks::pseudoRookAttacks(king_square))
		| ((pieces(~color, BISHOP) | pieces(~color, QUEEN)) & Attacks::pseudoBishopAttacks(king_square));
	Bitboard pinned = 0;

	for (Square pinner : BitboardIterator<Square>(pinners))
	{
		Bitboard b = ObstructedTable[pinner][king_square] & occupied();

		if ((b != 0)							// Van a pinner és a király között figura
			&& (b & (b - 1)) == 0				// Csak egy figura van a pinner és a király között
			&& ((b & occupied(color)) != 0))	// Ez a figura a sajátunk
			pinned |= b;
	}

	return pinned;
}