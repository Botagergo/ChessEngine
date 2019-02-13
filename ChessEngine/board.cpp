#include <regex>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "constants.h"

Board::Board()
{
	std::for_each(_pieces.begin(), _pieces.end(), [](auto &a) {
		a.fill(0);
	});

	_piece_list.fill(NO_PIECE);
	_occupied.fill(0);
	_attackedByColor.fill(0);
	_attackedByPiece.fill(0);

	_to_move = WHITE;
	_en_passant_target = NO_SQUARE;
	_en_passant_capture_target = NO_SQUARE;
	_castling_rights = 0;

	_halfmove_clock = _fullmove_num = 1;
}

Board Board::fromFen(const std::string &fen)
{
	const static std::string pos_regex = "((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/"
		"((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/"
		"((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/"
		"((?:[1-8]|[PNBRQKpnbrqk]){1,8})\\/((?:[1-8]|[PNBRQKpnbrqk]){1,8})";

	const static std::string to_move_regex = "(w|b)";
	const static std::string castling_rights_regex = "(\\-|[KQkq]{1,4})";
	const static std::string en_passant_square_regex = "(\\-|[a-h][1-8])";
	const static std::string halfmove_clock_regex = "(0|[1-9][0-9]*)";
	const static std::string fullmove_number_regex = "([1-9][0-9]*)";
	const static std::string whitespace_regex = "\\s*";

	const static std::string fen_regex = std::string("^") +
		whitespace_regex + pos_regex +
		whitespace_regex + to_move_regex +
		whitespace_regex + castling_rights_regex +
		whitespace_regex + en_passant_square_regex +
		whitespace_regex + halfmove_clock_regex +
		whitespace_regex + fullmove_number_regex + 
		whitespace_regex + std::string("$");

	std::smatch match_result;
	if (!std::regex_search(fen, match_result, std::regex(fen_regex)))
		throw FenParseError(fen.c_str());

	Board board;

	for (int i = 0; i < 8; ++i)
	{
		std::string str = match_result[i + 1];
		int curr = 0;

		for (char c : str)
		{
			Piece piece = charToPiece(tolower(c));

			if (piece != NO_PIECE)
			{
				Color color = 'a' <= c && c <= 'z' ? BLACK : WHITE;
				Square square = (Square)((7 - i) * 8 + curr);
				Bitboard b = SquareBB[square];
				board._pieces[color][piece] |= b;
				++curr;
			}
			else
				curr += c - '0';
		}

		if (curr != 8)
			throw FenParseError(fen.c_str());
	}

	if (match_result[9] == 'w')
		board._to_move = WHITE;
	else
		board._to_move = BLACK;

	std::string castling = match_result[10];
	if (castling != "-")
	{
		for (char c : castling)
		{
			if (c == 'K')
				board._castling_rights |= CastleFlag[WHITE][KINGSIDE];
			else if (c == 'Q')
				board._castling_rights |= CastleFlag[WHITE][QUEENSIDE];
			else if (c == 'k')
				board._castling_rights |= CastleFlag[BLACK][KINGSIDE];
			else
				board._castling_rights |= CastleFlag[BLACK][QUEENSIDE];
		}
	}

	std::string en_passant = match_result[11];
	if (en_passant != "-")
	{
		board._en_passant_target = parseSquare(en_passant);
		board._en_passant_capture_target = board.toMove() == WHITE ? (Square)(board._en_passant_target - 8)
			: Square(board._en_passant_target + 8);
	}

	std::stringstream ss(match_result[12]);
	ss >> board._halfmove_clock;

	ss = std::stringstream(match_result[13]);
	ss >> board._fullmove_num;

	board._initOccupied();
	board._initPieceList();
	board._updateAttacked();

	return board;
}

std::string Board::fen() const
{
	std::stringstream fen;

	for (int r = 7; r >= 0; r--)
	{
		int empty = 0;
		for (int c = 0; c < 8; c++)
		{
			Square square = Square(r * 8 + c);
			if (_piece_list[square] == NO_PIECE)
				empty++;

			else
			{
				if (empty != 0)
				{
					fen << (char)empty + '0';
					empty = 0;
				}

				char p = pieceToChar(_piece_list[square]);
				if (occupied(WHITE) & SquareBB[square])
					p += ('A' - 'a');

				fen << p;
			}
		}
		if (empty != 0)
			fen << (char)empty + '0';

		if (r > 0)
			fen << '/';
	}

	fen << " ";

	if (toMove() == WHITE)
		fen << "w";
	else
		fen << "b";

	fen << " ";

	if (canCastle(WHITE, KINGSIDE) | canCastle(WHITE, QUEENSIDE) | canCastle(BLACK, KINGSIDE) | canCastle(BLACK, QUEENSIDE))
	{
		fen << (canCastle(WHITE, KINGSIDE) ? "K" : "");
		fen << (canCastle(WHITE, QUEENSIDE) ? "Q" : "");
		fen << (canCastle(BLACK, KINGSIDE) ? "k" : "");
		fen << (canCastle(BLACK, QUEENSIDE) ? "q" : "");
	}
	else
		fen << "-";

	fen << " ";

	if (enPassantTarget() != NO_SQUARE)
		fen << SquareStr[enPassantTarget()];
	else
		fen << '-';

	fen << " ";

	assert(_halfmove_clock >= 0 && _fullmove_num > 0);
	fen << _halfmove_clock << " " << _fullmove_num;

	return fen.str();
}

Move Board::parseMove(std::string str) const
{
	const static std::string square_regex = "([a-h][1-8])";
	const static std::string promotion_regex = "(nbrq)";
	const static std::string whitespace_regex = "\\s*";

	const static std::string move_regex = std::string("^") +
		whitespace_regex + square_regex + square_regex + promotion_regex +
		whitespace_regex + std::string("$");

	std::smatch match_result;
	if (!std::regex_search(str, match_result, std::regex(move_regex)))
		throw MoveParseError(str.c_str());

	Move move;

	move.from = parseSquare(match_result[1]);
	move.to = parseSquare(match_result[2]);
	move.promotion = match_result.size() == 4 ? charToPiece(*match_result[3].first) : NO_PIECE;
	move.piece_type = pieceAt(move.from);

	if (move.piece_type == KING)
	{
		if (move.to - move.from == 2)
			move.flags |= KINGSIDE_CASTLE;
		else if (move.from - move.to == 2)
			move.flags |= QUEENSIDE_CASTLE;
	}

	if (pieceAt(move.to) != NO_PIECE)
		move.flags |= CAPTURE;
	else if (move.piece_type == PAWN)
	{
		if (abs(move.from - move.to) == 16)
			move.flags |= DOUBLE_PUSH;
		else
		{
			int diff = abs(move.to - move.from);
			if (diff == 7 || diff == 9)
				move.flags |= EN_PASSANT;
		}
	}

	return move;
}

std::string Board::moveToString(const Move &move) const
{
	std::stringstream ss;
	ss << SquareStr[move.from] << SquareStr[move.to];
	if(move.promotion != NO_PIECE)
		ss << pieceToChar(move.promotion);
	return ss.str();
}

void Board::print(std::ostream & os) const
{
	for (int i = 7; i >= 0; --i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Square square = (Square)(8 * i + j);
			char piece = _piece_list[square] != NO_PIECE ? pieceToChar(_piece_list[square]) : '0';

			if (occupied(WHITE) & SquareBB[square])
				piece = toupper(piece);
			else
				piece = tolower(piece);

			os << piece << " ";
		}
		os << std::endl;
	}
}

Bitboard Board::pieces(Color color, Piece piece) const
{
	return _pieces[color][piece];
}

Piece Board::pieceAt(Square square) const
{
	return _piece_list[square];
}

Bitboard Board::occupied(Color color) const
{
	return _occupied[color];
}

Bitboard Board::occupied() const
{
	return occupied(WHITE) | occupied(BLACK);
}

Bitboard Board::attacked(Color color) const
{
	return _attackedByColor[color];
}

Bitboard Board::attacked(Square square) const
{
	return _attackedByPiece[square];
}

Color Board::toMove() const
{
	return _to_move;
}

bool Board::makeMove(const Move & move)
{
	if (!(move.flags & NULL_MOVE))
	{
		if (move.flags & CASTLE)
			_castle((move.flags & CASTLE) == KINGSIDE_CASTLE ? KINGSIDE : QUEENSIDE);
		else
			_makeNormalMove(move);

		_updateCastlingRights(move);
		_updateAttacked();
	}

	// Ha sötét lép (Black == 1), akkor növeljük a lépésszámot
	_fullmove_num += toMove();

	if (move.piece_type == PAWN || move.flags & CAPTURE)
		++_halfmove_clock;
	else
		_halfmove_clock = 0;

	_to_move = ~toMove();

	if (isInCheck(~toMove()))
		return false;
	else
		return true;
}

bool Board::canCastle(Color color, Side side) const
{
	return _castling_rights & CastleFlag[color][side];
}

Square Board::enPassantTarget() const
{
	return _en_passant_target;
}

Square Board::enPassantCaptureTarget() const
{
	return _en_passant_capture_target;
}

bool Board::isInCheck(Color color) const
{
	return pieces(color, KING) & attacked(~color);
}

int Board::halfmoveClock() const
{
	return _halfmove_clock;
}

int Board::fullmoveNum() const
{
	return _fullmove_num;
}

void Board::_updateCastlingRights(const Move &m)
{
	if (m.piece_type == KING)
	{
		_castling_rights &= ~CastleFlag[toMove()][KINGSIDE];
		_castling_rights &= ~CastleFlag[toMove()][QUEENSIDE];
	}
	else if (m.from == A1 || m.to == A1)
		_castling_rights &= ~CastleFlag[WHITE][QUEENSIDE];
	else if (m.from == H1 || m.to == H1)
		_castling_rights &= ~CastleFlag[WHITE][KINGSIDE];
	else if (m.from == A8 || m.to == A8)
		_castling_rights &= ~CastleFlag[BLACK][QUEENSIDE];
	else if (m.from == H8 || m.to == H8)
		_castling_rights &= ~CastleFlag[BLACK][KINGSIDE];
}

void Board::_makeNormalMove(const Move & move)
{
	Piece piece = pieceAt(move.to);
	Color o = ~toMove();

	Bitboard b_from = SquareBB[move.from];
	Bitboard b_to = SquareBB[move.to];

	_pieces[toMove()][move.piece_type] ^= b_from;
	_occupied[toMove()] ^= b_from;
	_occupied[toMove()] |= b_to;

	if (move.promotion != NO_PIECE)
		_pieces[toMove()][move.promotion] |= b_to;
	else
		_pieces[toMove()][move.piece_type] |= b_to;

	if (piece != NO_PIECE)
	{
		_pieces[o][piece] ^= b_to;
		_occupied[o] ^= b_to;
	}

	if (move.flags & EN_PASSANT)
	{
		assert(_en_passant_capture_target != NO_SQUARE);
		Bitboard ep_ct_bb = SquareBB[_en_passant_capture_target];
		_pieces[o][PAWN] ^= ep_ct_bb;
		_piece_list[_en_passant_capture_target] = NO_PIECE;

		assert(_occupied[o] & ep_ct_bb);
		_occupied[o] ^= ep_ct_bb;
	}

	if (move.flags & DOUBLE_PUSH)
	{
		Square en_passant_target_square = Square(toMove() == WHITE ? move.to - 8 : move.to + 8);
		_en_passant_target = en_passant_target_square;
		_en_passant_capture_target = move.to;
	}
	else
	{
		_en_passant_target = NO_SQUARE;
		_en_passant_capture_target = NO_SQUARE;
	}

	_piece_list[move.from] = NO_PIECE;

	if (move.promotion != NO_PIECE)
		_piece_list[move.to] = move.promotion;
	else
		_piece_list[move.to] = move.piece_type;
}

void Board::_castle(int side)
{
	assert(canCastle(toMove(), (Side)side));

	Square k_from = toMove() == WHITE ? E1 : E8;
	Square k_to = toMove() == WHITE ? (side == KINGSIDE ? G1 : C1) : (side == KINGSIDE ? G8 : C8);

	Square r_from = toMove() == WHITE ? (side == KINGSIDE ? H1 : A1) : (side == KINGSIDE ? H8 : A8);
	Square r_to = toMove() == WHITE ? (side == KINGSIDE ? F1 : D1) : (side == KINGSIDE ? F8 : D8);

	_pieces[toMove()][KING] ^= SquareBB[k_from];
	_pieces[toMove()][KING] |= SquareBB[k_to];
	_pieces[toMove()][ROOK] ^= SquareBB[r_from];
	_pieces[toMove()][ROOK] |= SquareBB[r_to];

	_piece_list[k_from] = NO_PIECE;
	_piece_list[k_to] = KING;
	_piece_list[r_from] = NO_PIECE;
	_piece_list[r_to] = ROOK;

	_occupied[toMove()] ^= SquareBB[k_from];
	_occupied[toMove()] |= SquareBB[k_to];
	_occupied[toMove()] ^= SquareBB[r_from];
	_occupied[toMove()] |= SquareBB[r_to];

	_en_passant_target = NO_SQUARE;
	_en_passant_capture_target = NO_SQUARE;
}

void Board::_initOccupied()
{
	_occupied[WHITE] = _occupied[BLACK] = 0;
	for (Piece piece_type : Pieces)
	{
		_occupied[WHITE] |= _pieces[WHITE][piece_type];
		_occupied[BLACK] |= _pieces[BLACK][piece_type];
	}
}

void Board::_initPieceList()
{
	std::fill(_piece_list.begin(), _piece_list.end(), NO_PIECE);
	for(Color color : Colors)
	for (Piece piece_type : Pieces)
	{
		for (Square square : BitboardIterator<Square>(_pieces[color][piece_type]))
		{
			_piece_list[square] = piece_type;
		}
	}
}

void Board::_updateAttacked()
{
	_attackedByColor[WHITE] = _attackedByColor[BLACK] = 0;

	for (Square square = A1; square < SQUARE_NB; ++square)
	{
		Piece piece = pieceAt(square);
		if(piece != NO_PIECE)
		{
			Bitboard attacks = 0;
			if(piece != PAWN)
				attacks = pieceAttacks(square, piece, occupied());
			else
			{
				Bitboard pawn = SquareBB[square];
				if (occupied(WHITE) & pawn)
					attacks = pawnAttacks<WHITE>(pawn);
				else
					attacks = pawnAttacks<BLACK>(pawn);
			}

			_attackedByPiece[square] = attacks;

			if (SquareBB[square] & occupied(WHITE))
				_attackedByColor[WHITE] |= attacks;
			else
				_attackedByColor[BLACK] |= attacks;
		}
	}
}



const int Board::AllCastlingRights = 15;
const int Board::CastleFlag[COLOR_NB][SIDE_NB] = { { 1, 2 },{ 4, 8 } };
