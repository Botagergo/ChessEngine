#pragma once

#include <array>
#include <sstream>
#include <string>
#include <vector>

#include "bitboard.h"
#include "bitboard_iterator.h"
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

	Bitboard occupied(Color color) const;
	Bitboard occupied() const;

	Bitboard attacked(Color color) const;
	Bitboard attacked(Square square) const;

	bool canCastle(Color color, Side side) const;

	Square enPassantTarget() const;
	Square enPassantCaptureTarget() const;

	bool isInCheck(Color color) const;

	int halfmoveClock() const;
	int fullmoveNum() const;

	unsigned long long hash() const;

	const static int AllCastlingRights;
	const static int CastleFlag[COLOR_NB][SIDE_NB];

	unsigned char _castling_rights;

private:
	void _castle(Side side);
	void _updateCastlingRights(Move move);
	void _makeNormalMove(Move move);

	Color _to_move;

	std::array<std::array<Bitboard, PIECE_TYPE_NB>, COLOR_NB> _pieces;
	std::array<Piece, SQUARE_NB> _piece_list;
	std::array<Bitboard, COLOR_NB> _occupied;
	std::array<Bitboard, SQUARE_NB> _attackedByPiece;
	std::array<Bitboard, COLOR_NB> _attackedByColor;
	std::array<Bitboard, COLOR_NB> _pinnedPieces;

	Square _en_passant_target;
	Square _en_passant_capture_target;

	int _halfmove_clock;
	int _fullmove_num;

	unsigned long long _hash;

	void _initOccupied();
	void _initPieceList();
	void _updateAttacked();
};

