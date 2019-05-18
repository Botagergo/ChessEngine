#include <regex>

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "constants.h"
#include "util.h"
#include "zobrist.h"


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

	_hash = 0;
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
			Piece piece = charToPiece(c);

			if (piece != NO_PIECE)
			{
				Color color = pieceColor(piece);
				Square square = (Square)((7 - i) * 8 + curr);
				Bitboard b = Constants::SquareBB[square];
				board._pieces[color][toPieceType(piece)] |= b;
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
	board._initMaterial();
	board._updateAttacked();

	board._hash = Zobrist::getBoardHash(board);

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
					fen << (char)(empty + '0');
					empty = 0;
				}

				fen << pieceToChar(_piece_list[square]);
			}
		}
		if (empty != 0)
			fen << (char)(empty + '0');

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
		fen << Constants::SquareStr[enPassantTarget()];
	else
		fen << '-';

	fen << " ";

	ASSERT(_halfmove_clock >= 0 && _fullmove_num > 0);
	fen << _halfmove_clock << " " << _fullmove_num;

	return fen.str();
}

void Board::print(std::ostream & os) const
{
	for (int i = 7; i >= 0; --i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Square square = (Square)(8 * i + j);
			char piece = _piece_list[square] != NO_PIECE ? pieceToChar(_piece_list[square]) : '0';

			os << piece << " ";
		}
		os << std::endl;
	}
}

Bitboard Board::pieces(Color color, PieceType piece_type) const
{
	return _pieces[color][piece_type];
}

Piece Board::pieceAt(Square square) const
{
	return _piece_list[square];
}

int Board::material(Color color, PieceType piece_type) const 
{
	return _material[color][piece_type];
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

bool Board::makeMove(Move move)
{
	if (!move.isNull())
	{
		if (move.isCastle())
			_castle(move.isCastle(KINGSIDE) ? KINGSIDE : QUEENSIDE);
		else
			_makeNormalMove(move);

		_updateCastlingRights(move);
		_updateAttacked();
	}
	else if (_en_passant_target != NO_SQUARE)
	{
		_hash ^= Zobrist::EnPassantFileHash[Util::getFile(enPassantTarget())];
		_en_passant_target = NO_SQUARE;
		_en_passant_capture_target = NO_SQUARE;
	}

	// Ha sötét lép (Black == 1), akkor növeljük a lépésszámot
	_fullmove_num += toMove();

	if (move.pieceType() == PAWN || move.isCapture())
		++_halfmove_clock;
	else
		_halfmove_clock = 0;

	_hash ^= Zobrist::BlackMovesHash;

	_to_move = ~toMove();

	if (isInCheck(~toMove()))
		return false;
	else
		return true;
}

int Board::phase() const
{
	const static int PHASE[] = { 0, 1, 1, 2, 4 };
	const static int total = PHASE[PAWN] * 16 + PHASE[KNIGHT] * 4 + PHASE[BISHOP] * 4
		+ PHASE[ROOK] * 4 + PHASE[QUEEN] * 2;

	int ret = total;

	for (PieceType piece_type = PAWN; piece_type < KING; ++piece_type)
	{
		ret -= _material[WHITE][piece_type] * PHASE[piece_type];
		ret -= _material[BLACK][piece_type] * PHASE[piece_type];
	}

	return (ret * 256 + (total / 2)) / total;
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

bool Board::allowNullMove() const
{
	// Ha kevés figura van a táblán, akkor zugzwang miatt kockácatos a null move engedélyezése
	return material(toMove(), KNIGHT) + material(toMove(), BISHOP)
		+ material(toMove(), ROOK) + material(toMove(), QUEEN) >= 3;
}

int Board::halfmoveClock() const
{
	return _halfmove_clock;
}

int Board::fullmoveNum() const
{
	return _fullmove_num;
}
u64 Board::hash() const
{
	return _hash;
}

Board Board::flip() const
{
	Board board(*this);

	for (PieceType piece_type = PAWN; piece_type < PIECE_TYPE_NB; ++piece_type)
	{
		std::swap(board._pieces[WHITE][piece_type], board._pieces[BLACK][piece_type]);
		std::swap(board._material[WHITE][piece_type], board._material[BLACK][piece_type]);
		board._pieces[WHITE][piece_type] = Util::verticalFlip(board._pieces[WHITE][piece_type]);
		board._pieces[BLACK][piece_type] = Util::verticalFlip(board._pieces[BLACK][piece_type]);

	}

	std::swap(board._occupied[WHITE], board._occupied[BLACK]);
	board._occupied[WHITE] = Util::verticalFlip(board._occupied[WHITE]);
	board._occupied[BLACK] = Util::verticalFlip(board._occupied[BLACK]);

	if (_en_passant_target != NO_SQUARE)
	{
		board._en_passant_target = Util::bitScanForward(Util::verticalFlip(Constants::SquareBB[_en_passant_target]));
		board._en_passant_capture_target = Util::bitScanForward(Util::verticalFlip(Constants::SquareBB[_en_passant_capture_target]));
	}

	unsigned char new_cr = 0;
	for (Color color : Colors)
		for (Side side : Sides)
			if (board.canCastle(color, side))
				new_cr |= CastleFlag[~color][side];
	board._castling_rights = new_cr;

	board._to_move = ~board._to_move;

	board._initPieceList();
	board._updateAttacked();

	board._hash = Zobrist::getBoardHash(board);

	return board;
}

void Board::_updateCastlingRights(const Move move)
{
	unsigned char new_castling_rights = _castling_rights;

	if (move.pieceType() == KING)
	{
		new_castling_rights &= ~CastleFlag[toMove()][KINGSIDE];
		new_castling_rights &= ~CastleFlag[toMove()][QUEENSIDE];
	}
	else if (move.from() == A1 || move.to() == A1)
		new_castling_rights &= ~CastleFlag[WHITE][QUEENSIDE];
	else if (move.from() == H1 || move.to() == H1)
		new_castling_rights &= ~CastleFlag[WHITE][KINGSIDE];
	else if (move.from() == A8 || move.to() == A8)
		new_castling_rights &= ~CastleFlag[BLACK][QUEENSIDE];
	else if (move.from() == H8 || move.to() == H8)
		new_castling_rights &= ~CastleFlag[BLACK][KINGSIDE];

	_hash ^= Zobrist::CastlingRightsHash[new_castling_rights ^ _castling_rights];
	_castling_rights = new_castling_rights;
}

void Board::_makeNormalMove(Move move)
{
	Piece piece = pieceAt(move.to());
	Color o = ~toMove();

	Bitboard b_from = Constants::SquareBB[move.from()];
	Bitboard b_to = Constants::SquareBB[move.to()];

	_pieces[toMove()][move.pieceType()] ^= b_from;
	_occupied[toMove()] ^= b_from;
	_occupied[toMove()] |= b_to;
	_hash ^= Zobrist::PiecePositionHash[toMove()][move.pieceType()][move.from()];

	if (move.isPromotion())
	{
		_pieces[toMove()][move.promotion()] |= b_to;
		_material[toMove()][move.promotion()] += 1;
		_material[toMove()][move.pieceType()] -= 1;
		_hash ^= Zobrist::PiecePositionHash[toMove()][move.promotion()][move.to()];
	}
	else
	{
		_pieces[toMove()][move.pieceType()] |= b_to;
		_hash ^= Zobrist::PiecePositionHash[toMove()][move.pieceType()][move.to()];
	}

	if (piece != NO_PIECE)
	{
		_pieces[o][toPieceType(piece)] ^= b_to;
		_material[o][toPieceType(piece)] -= 1;
		_occupied[o] ^= b_to;
		_hash ^= Zobrist::PiecePositionHash[o][toPieceType(piece)][move.to()];
	}

	Square en_passant_target = enPassantTarget();

	if (move.isEnPassant())
	{
		ASSERT(_en_passant_capture_target != NO_SQUARE);
		Bitboard ep_ct_bb = Constants::SquareBB[_en_passant_capture_target];

		_pieces[o][PAWN] ^= ep_ct_bb;
		_material[o][PAWN] -= 1;
		_piece_list[_en_passant_capture_target] = NO_PIECE;

		ASSERT(_occupied[o] & ep_ct_bb);
		_occupied[o] ^= ep_ct_bb;

		_hash ^= Zobrist::PiecePositionHash[o][PAWN][enPassantCaptureTarget()];
	}

	if (en_passant_target != NO_SQUARE)
	{
		_hash ^= Zobrist::EnPassantFileHash[Util::getFile(enPassantTarget())];
		_en_passant_target = NO_SQUARE;
		_en_passant_capture_target = NO_SQUARE;
	}

	if (move.isDoublePush())
	{
		Square en_passant_target_square = Square(toMove() == WHITE ? move.to() - 8 : move.to() + 8);
		_en_passant_target = en_passant_target_square;
		_en_passant_capture_target = move.to();

		_hash ^= Zobrist::EnPassantFileHash[Util::getFile(enPassantTarget())];
	}

	_piece_list[move.from()] = NO_PIECE;

	if (move.isPromotion())
		_piece_list[move.to()] = toPiece(move.promotion(), toMove());
	else
		_piece_list[move.to()] = toPiece(move.pieceType(), toMove());
}

void Board::_castle(Side side)
{
	ASSERT(canCastle(toMove(), side));

	Square k_from = toMove() == WHITE ? E1 : E8;
	Square k_to = toMove() == WHITE ? (side == KINGSIDE ? G1 : C1) : (side == KINGSIDE ? G8 : C8);

	Square r_from = toMove() == WHITE ? (side == KINGSIDE ? H1 : A1) : (side == KINGSIDE ? H8 : A8);
	Square r_to = toMove() == WHITE ? (side == KINGSIDE ? F1 : D1) : (side == KINGSIDE ? F8 : D8);

	_pieces[toMove()][KING] ^= Constants::SquareBB[k_from];
	_pieces[toMove()][KING] |= Constants::SquareBB[k_to];
	_pieces[toMove()][ROOK] ^= Constants::SquareBB[r_from];
	_pieces[toMove()][ROOK] |= Constants::SquareBB[r_to];

	_piece_list[k_from] = NO_PIECE;
	_piece_list[k_to] = toPiece(KING, toMove());
	_piece_list[r_from] = NO_PIECE;
	_piece_list[r_to] = toPiece(ROOK, toMove());

	_occupied[toMove()] ^= Constants::SquareBB[k_from];
	_occupied[toMove()] |= Constants::SquareBB[k_to];
	_occupied[toMove()] ^= Constants::SquareBB[r_from];
	_occupied[toMove()] |= Constants::SquareBB[r_to];

	_hash ^= Zobrist::PiecePositionHash[toMove()][KING][k_from];
	_hash ^= Zobrist::PiecePositionHash[toMove()][KING][k_to];
	_hash ^= Zobrist::PiecePositionHash[toMove()][ROOK][r_from];
	_hash ^= Zobrist::PiecePositionHash[toMove()][ROOK][r_to];

	if (_en_passant_target != NO_SQUARE)
	{
		_hash ^= Zobrist::EnPassantFileHash[Util::getFile(enPassantTarget())];
		_en_passant_target = NO_SQUARE;
		_en_passant_capture_target = NO_SQUARE;
	}
}

void Board::_initOccupied()
{
	_occupied[WHITE] = _occupied[BLACK] = 0;
	for (PieceType piece_type : PieceTypes)
	{
		_occupied[WHITE] |= _pieces[WHITE][piece_type];
		_occupied[BLACK] |= _pieces[BLACK][piece_type];
	}
}

void Board::_initPieceList()
{
	std::fill(_piece_list.begin(), _piece_list.end(), NO_PIECE);
	for(Color color : Colors)
	for (PieceType piece_type : PieceTypes)
		for (Square square : BitboardIterator<Square>(_pieces[color][piece_type]))
		{
			_piece_list[square] = toPiece(piece_type, color);
		}
}

void Board::_initMaterial()
{
	for (Color color : Colors)
		for (PieceType piece_type : PieceTypes)
		{
			_material[color][piece_type] = Util::popCount(pieces(color, piece_type));
		}
}

void Board::_updateAttacked()
{
	_attackedByColor[WHITE] = _attackedByColor[BLACK] = 0;
	_attackedByPiece = { 0 };

	for (Color color : Colors)
	{
		for (PieceType piece_type : PieceTypes)
		{
			for (Square square : BitboardIterator<Square>(pieces(color, piece_type)))
			{
				Piece piece = pieceAt(square);
				if (piece != NO_PIECE)
				{
					Bitboard attacks = 0;
					PieceType piece_type = toPieceType(piece);


					if (piece_type != PAWN)
						attacks = Attacks::pieceAttacks(square, piece_type, occupied());
					else
					{
						Bitboard pawn = Constants::SquareBB[square];
						if (occupied(WHITE) & pawn)
							attacks = Attacks::pawnAttacks<WHITE>(pawn);
						else
							attacks = Attacks::pawnAttacks<BLACK>(pawn);
					}

					_attackedByPiece[square] = attacks;

					if (Constants::SquareBB[square] & occupied(WHITE))
						_attackedByColor[WHITE] |= attacks;
					else
						_attackedByColor[BLACK] |= attacks;
				}
			}
		}
	}
}

const int Board::AllCastlingRights = 15;
const int Board::CastleFlag[COLOR_NB][SIDE_NB] = { { 1, 2 },{ 4, 8 } };
