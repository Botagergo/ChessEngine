#include <cctype>
#include <sstream>

#include "constants.h"
#include "fen.h"

Board getBoardFromFen(const std::string &fen)
{
	Board board;

	std::istringstream iss(fen);

	for (int i = 0; i < 8; ++i)
	{
		int j = 0;
		char c = iss.get();

		while (j < 8 && c != '/')
		{
			Color color = islower(c) ? BLACK : WHITE;
			Square square = (Square)((7 - i) * 8 + j);
			Bitboard b = SquareBB[square];
			Piece piece = charToPiece(c);

			if (piece != NO_PIECE)
			{
				_pieces[piece][color] |= b;
				_piece_list[square] = piece;
				++j;
			}
			else
				j += c - '0';

			c = iss1.get();
		}
	}

	char next; iss >> next;

	if (next == 'w')
		_to_move = C_WHITE;
	else
		_to_move = C_BLACK;

	std::string castling; iss >> castling;
	for (char c : castling)
	{
		if (c == 'K')
			_castlingRights |= CastleFlag[C_WHITE][KINGSIDE];
		else if (c == 'Q')
			_castlingRights |= CastleFlag[C_WHITE][QUEENSIDE];
		else if (c == 'k')
			_castlingRights |= CastleFlag[C_BLACK][KINGSIDE];
		else if (c == 'q')
			_castlingRights |= CastleFlag[C_BLACK][QUEENSIDE];
	}

	std::string enPassant; iss >> enPassant;
	if (enPassant != "-")
	{
		_en_passant_target = SquareBB[parseSquare(enPassant.c_str())];
		_en_passant_capture_target = toMove() == C_WHITE ? _en_passant_target - Bitboard(8) : _en_passant_target + Bitboard(8);
	}
	else
	{
		_en_passant_target = 0;
		_en_passant_capture_target = 0;
	}

	_hash = Zobrist::getPositionHash(*this);

	_initMaterial();
	_initOccupied();
	_initPieceSquareScore();
}