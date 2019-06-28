#include "stdafx.h"
#include "CppUnitTest.h"

#include "attacks.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "evaluation.h"
#include "search.h"
#include "see.h"
#include "util.h"
#include "zobrist.h"

#include <locale>
#include <codecvt>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace Evaluation;
using namespace Constants;

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Color>(const Color& c) { RETURN_WIDE_STRING((int)c); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Square>(const Square& s) { RETURN_WIDE_STRING((int)s); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Piece>(const Piece& p) { RETURN_WIDE_STRING((int)p); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<PieceType>(const PieceType& p) { RETURN_WIDE_STRING((int)p); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Move>(const Move& m) { return converter.from_bytes(m.toAlgebraic()); }

void init()
{
	initSquareBB();
	initAttackTables();
	initObstructedTable();
	Zobrist::initZobristHashing();
}

namespace UnitTests
{
	TEST_CLASS(UnitTest1)
	{
	public:
		TEST_METHOD(shift_Test)
		{
			Bitboard bb = 0x920000091002002Ull;

			Assert::AreEqual(Util::shift<NORTH>(0), 0Ull);

			Assert::AreEqual(Util::shift<NORTH>(bb), 0x2000009100200200Ull);
			Assert::AreEqual(Util::shift<NORTHEAST>(bb), 0x4000002200400400Ull);
			Assert::AreEqual(Util::shift<EAST>(bb), 0x1240000022004004Ull);
			Assert::AreEqual(Util::shift<SOUTHEAST>(bb), 0x12400000220040Ull);
			Assert::AreEqual(Util::shift<SOUTH>(bb), 0x9200000910020Ull);
			Assert::AreEqual(Util::shift<SOUTHWEST>(bb), 0x4100000480010Ull);
			Assert::AreEqual(Util::shift<WEST>(bb), 0x410000048001001Ull);
			Assert::AreEqual(Util::shift<NORTHWEST>(bb), 0x1000004800100100Ull);
		}

		TEST_METHOD(flip_Test)
		{
			Assert::AreEqual(Util::verticalFlip(0Ull), 0Ull);
			Assert::AreEqual(Util::verticalFlip(0x1000200020020004Ull), 0x400022000200010Ull);
		}

		TEST_METHOD(piece_Test)
		{
			Assert::AreEqual(PAWN, toPieceType(WHITE_PAWN));
			Assert::AreEqual(PAWN, toPieceType(BLACK_PAWN));
			Assert::AreEqual(KNIGHT, toPieceType(WHITE_KNIGHT));
			Assert::AreEqual(KNIGHT, toPieceType(BLACK_KNIGHT));
			Assert::AreEqual(BISHOP, toPieceType(WHITE_BISHOP));
			Assert::AreEqual(BISHOP, toPieceType(BLACK_BISHOP));
			Assert::AreEqual(ROOK, toPieceType(WHITE_ROOK));
			Assert::AreEqual(ROOK, toPieceType(BLACK_ROOK));
			Assert::AreEqual(QUEEN, toPieceType(WHITE_QUEEN));
			Assert::AreEqual(QUEEN, toPieceType(BLACK_QUEEN));
			Assert::AreEqual(KING, toPieceType(WHITE_KING));
			Assert::AreEqual(KING, toPieceType(BLACK_KING));

			Assert::AreEqual(BLACK_PAWN, toPiece(PAWN, BLACK));
			Assert::AreEqual(BLACK_KNIGHT, toPiece(KNIGHT, BLACK));
			Assert::AreEqual(BLACK_BISHOP, toPiece(BISHOP, BLACK));
			Assert::AreEqual(WHITE_ROOK, toPiece(ROOK, WHITE));
			Assert::AreEqual(WHITE_QUEEN, toPiece(QUEEN, WHITE));
			Assert::AreEqual(BLACK_KING, toPiece(KING, BLACK));

			Assert::AreEqual('p', pieceTypeToChar(PAWN));
			Assert::AreEqual('q', pieceTypeToChar(QUEEN));
			Assert::AreEqual('n', pieceTypeToChar(KNIGHT));

			Assert::AreEqual('Q', pieceToChar(WHITE_QUEEN));
			Assert::AreEqual('R', pieceToChar(WHITE_ROOK));
			Assert::AreEqual('p', pieceToChar(BLACK_PAWN));
			Assert::AreEqual('n', pieceToChar(BLACK_KNIGHT));

			Assert::AreEqual(BLACK_KNIGHT, charToPiece('n'));
			Assert::AreEqual(BLACK_BISHOP, charToPiece('b'));
			Assert::AreEqual(WHITE_QUEEN, charToPiece('Q'));
			Assert::AreEqual(WHITE_KING, charToPiece('K'));

			Assert::AreEqual(KNIGHT, charToPieceType('n'));
			Assert::AreEqual(BISHOP, charToPieceType('B'));
			Assert::AreEqual(ROOK, charToPieceType('R'));
			Assert::AreEqual(QUEEN, charToPieceType('q'));
		}

		TEST_METHOD(pawnPushTargets_Test)
		{
			Assert::AreEqual(Attacks::pawnPushTargets<WHITE>(0x1000000000082100Ull, FullBB), 0x29210000Ull);
			Assert::AreEqual(Attacks::pawnPushTargets<WHITE>(0x2800002100Ull, FullBB ^ 0x480001200000Ull), 0x200000010000Ull);
		}

		TEST_METHOD(pawnAttacks_Test)
		{
			Bitboard pawns = 0x881000000040000Ull;
			Bitboard attacks = 0x420000000a000000Ull;

			Assert::AreEqual(Attacks::pawnAttacks<WHITE>(pawns), attacks);
			Assert::AreEqual(Attacks::pawnAttacks<BLACK>(Util::verticalFlip(pawns)), Util::verticalFlip(attacks));
		}

		TEST_METHOD(knightAttacks_Test)
		{
			initSquareBB();
			initAttackTables();

			Assert::AreEqual(Attacks::knightAttacks(E4), 0x284400442800Ull);
			Assert::AreEqual(Attacks::knightAttacks(B4), 0x50800080500Ull);
			Assert::AreEqual(Attacks::knightAttacks(D7), 0x2200221400000000Ull);
			Assert::AreEqual(Attacks::knightAttacks(G5), 0xa0100010a00000Ull);
			Assert::AreEqual(Attacks::knightAttacks(E2), 0x28440044Ull);
			Assert::AreEqual(Attacks::knightAttacks(A5), 0x2040004020000Ull);
			Assert::AreEqual(Attacks::knightAttacks(D8), 0x22140000000000Ull);
			Assert::AreEqual(Attacks::knightAttacks(H5), 0x40200020400000Ull);
			Assert::AreEqual(Attacks::knightAttacks(E1), 0x284400Ull);
			Assert::AreEqual(Attacks::knightAttacks(B2), 0x5080008Ull);
			Assert::AreEqual(Attacks::knightAttacks(A2), 0x2040004Ull);
			Assert::AreEqual(Attacks::knightAttacks(A1), 0x20400Ull);
			Assert::AreEqual(Attacks::knightAttacks(A8), 0x4020000000000Ull);
			Assert::AreEqual(Attacks::knightAttacks(H8), 0x20400000000000Ull);
			Assert::AreEqual(Attacks::knightAttacks(H1), 0x402000Ull);
		}

		TEST_METHOD(bishopAttacks_Test)
		{
			initSquareBB();
			initAttackTables();

			Assert::AreEqual(Attacks::bishopAttacks(A8, EmptyBB), 0x2040810204080Ull);
			Assert::AreEqual(Attacks::bishopAttacks(E6, EmptyBB), 0x4428002844820100Ull);
			Assert::AreEqual(Attacks::bishopAttacks(D1, EmptyBB), 0x8041221400Ull);
			Assert::AreEqual(Attacks::bishopAttacks(E4, 0x80000800280000Ull), 0x80402800280000Ull);
			Assert::AreEqual(Attacks::bishopAttacks(A1, 0x40200Ull), 0x200Ull);
			Assert::AreEqual(Attacks::bishopAttacks(D4, 0x21000140000Ull), 0x21400140000Ull);
		}

		TEST_METHOD(rookAttacks_Test)
		{
			initSquareBB();
			initAttackTables();

			Assert::AreEqual(Attacks::rookAttacks(D3, EmptyBB), 0x808080808f70808Ull);
			Assert::AreEqual(Attacks::rookAttacks(A1, EmptyBB), 0x1010101010101feUll);
			Assert::AreEqual(Attacks::rookAttacks(A1, 0x104Ull), 0x106Ull);
			Assert::AreEqual(Attacks::rookAttacks(D3, 0x808000000120000Ull), 0x8080808160808Ull);
			Assert::AreEqual(Attacks::rookAttacks(H5, 0x802080000000Ull), 0x806080000000Ull);
		}

		TEST_METHOD(queenAttacks_Test)
		{
			initSquareBB();
			initAttackTables();
		}

		TEST_METHOD(bitboardIterator_Test)
		{
			BitboardIterator<Square> squares(0Ull);
			auto b1 = squares.begin();
			Assert::IsTrue(squares.end() == b1);

			squares = BitboardIterator<Square>(0x21000008000004c0Ull);

			auto expected1 = { G1, H1, C2, D5, A8, F8 };
			Assert::IsTrue(std::equal(squares.begin(), squares.end(), expected1.begin()));

			BitboardIterator<Bitboard> bitboards(0Ull);
			auto b2 = bitboards.begin();
			Assert::IsTrue(bitboards.end() == b2);

			bitboards = BitboardIterator<Bitboard>(0x21000008000004c0Ull);

			auto expected2 = { 0x40Ull, 0x80Ull, 0x400Ull, 0x800000000Ull, 0x100000000000000Ull, 0x2000000000000000Ull };
			Assert::IsTrue(std::equal(bitboards.begin(), bitboards.end(), expected2.begin()));
		}

		TEST_METHOD(Move_Test)
		{
			initSquareBB();
			initAttackTables();

			Move move(PAWN, A2, A4, FLAG_DOUBLE_PUSH);

			Assert::IsTrue(move.from() == A2);
			Assert::IsTrue(move.to() == A4);
			Assert::IsTrue(move.pieceType() == PAWN);
			Assert::IsTrue(move.isDoublePush());
			Assert::IsFalse(move.isEnPassant());
			Assert::IsFalse(move.isPromotion());

			Board board = Board::fromFen("r1bqk2r/ppp2ppp/2n1pn2/8/1bBP4/2N1PN2/PP3PPP/R1BQ1RK1 b kq - 3 7 ");
			move = Move::fromAlgebraic(board, "b4c3");

			Assert::IsTrue(move.from() == B4);
			Assert::IsTrue(move.to() == C3);
			Assert::IsTrue(move.pieceType() == BISHOP);
			Assert::IsTrue(move.isCapture());
			Assert::IsFalse(move.isDoublePush());
			Assert::IsFalse(move.isEnPassant());
			Assert::IsFalse(move.isPromotion());
		}

		TEST_METHOD(Board_fromFen_Test) //TODO: add more tests
		{
			initSquareBB();
			initAttackTables();

			Board b = Board::fromFen("5k2/2p5/1n2p3/8/4pP2/4P3/6PP/R2QK1NR b Q f3 0 1 ");

			Assert::IsFalse(b.canCastle(BLACK, KINGSIDE));
			Assert::IsFalse(b.canCastle(BLACK, QUEENSIDE));
			Assert::IsTrue(b.canCastle(WHITE, QUEENSIDE));
			Assert::IsFalse(b.canCastle(WHITE, KINGSIDE));

			Assert::AreEqual(b.toMove(), BLACK);

			Assert::AreEqual(b.pieces(WHITE, PAWN), 0x2010c000Ull);
			Assert::AreEqual(b.pieces(WHITE, KNIGHT), 0x40Ull);
			Assert::AreEqual(b.pieces(WHITE, BISHOP), 0x0Ull);
			Assert::AreEqual(b.pieces(WHITE, ROOK), 0x81Ull);
			Assert::AreEqual(b.pieces(WHITE, QUEEN), 0x8Ull);
			Assert::AreEqual(b.pieces(WHITE, KING), 0x10Ull);

			Assert::AreEqual(b.pieces(BLACK, PAWN), 0x4100010000000Ull);
			Assert::AreEqual(b.pieces(BLACK, KNIGHT), 0x20000000000Ull);
			Assert::AreEqual(b.pieces(BLACK, BISHOP), 0x0Ull);
			Assert::AreEqual(b.pieces(BLACK, ROOK), 0x0Ull);
			Assert::AreEqual(b.pieces(BLACK, QUEEN), 0x0Ull);
			Assert::AreEqual(b.pieces(BLACK, KING), 0x2000000000000000Ull);

			Assert::AreEqual(b.enPassantTarget(), F3);
			Assert::AreEqual(b.enPassantCaptureTarget(), F4);

			Assert::AreEqual(b.occupied(WHITE), 0x2010c0d9Ull);
			Assert::AreEqual(b.occupied(BLACK), 0x2004120010000000Ull);

			Board::fromFen("5k2/2p5/1n2p3/8/4pP2/4P3/6PP/R2QK1NR b - - 0 111 ");
			Board::fromFen("5k2/2p5/1n2p3/8/4pP2/4P3/6PP/R2QK1NR b - - 4444 111 ");

			std::vector<std::string> bad_fens = {
				"",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP w KQ - 1 8",
				"rnbq1kr/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP w KQ - 1 8"
				"rnbq1krqq/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP w KQ - 1 8"
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8/8 w KQ - 1 8",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8 r KQ - 1 8",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8 r KQw - 1 8",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8 r KQ * 1 8",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8 r KQ * 1 -8",
				"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/8 r KQ * 0 8",
			};

			for (auto fen : bad_fens)
			{
				Assert::ExpectException<FenParseError>([&]()
				{
					Board::fromFen(fen);
				});
			}
		}

		TEST_METHOD(zobrist_Test)
		{
			initSquareBB();
			initAttackTables();
			Zobrist::initZobristHashing();

			vector<string> fen1 = { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
				"r1bqkbnr/ppppp1pp/2n5/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3" };
			vector<vector<string>> moves = {
				{ "e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "e1g1", "d7d6", "b1c3", "f8e7", "d2d3", "e8g8" },
			{ "g1f3", "g8f6", "f3g1", "f6g8" }
			};
			vector<string> fen2 = { "r1bq1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 1 7 ",
				"r1bqkbnr/ppppp1pp/2n5/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3" };

			vector<bool> equals = { true, false };

			for (int i = 0; i < fen1.size(); ++i)
			{
				Board board = Board::fromFen(fen1[i]);

				for (std::string move : moves[i])
				{
					board.makeMove(Move::fromAlgebraic(board, move));
				}

				unsigned long long hash1 = board.hash();
				board = Board::fromFen(fen2[i]);
				unsigned long long hash2 = board.hash();

				if (equals[i])
					Assert::AreEqual(hash1, hash2);
				else
					Assert::AreNotEqual(hash1, hash2);
			}
		}

		TEST_METHOD(see_Test)
		{
			initSquareBB();
			initAttackTables();
			Zobrist::initZobristHashing();

			Board board = Board::fromFen("rnb1kbnr/pppp1ppp/8/4p1q1/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3 ");

			Move move = Move::fromAlgebraic(board, "f3g5");
			Assert::AreEqual(PieceValue[QUEEN].mg, see<WHITE>(board, Move::fromAlgebraic(board, "f3g5")));

			board = Board::fromFen("rnb1k1nr/ppppbppp/8/4p1q1/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 4 4 ");
			Assert::AreEqual(PieceValue[QUEEN].mg - PieceValue[KNIGHT].mg, see<WHITE>(board, Move::fromAlgebraic(board, "f3g5")));

			board = Board::fromFen("rnb1k1nr/1pppbppp/p7/4p1q1/4P3/2NP1N2/PPP2PPP/R1BQKB1R w KQkq - 0 5 ");
			Assert::AreEqual(PieceValue[QUEEN].mg - PieceValue[KNIGHT].mg + PieceValue[BISHOP].mg, see<WHITE>(board, Move::fromAlgebraic(board, "f3g5")));

			board = Board::fromFen("rnb1k1nr/1pppb1pp/p4p2/4p1q1/4P3/2NP1N2/PPPQ1PPP/R1B1KB1R w KQkq - 0 6 ");
			Assert::AreEqual(PieceValue[QUEEN].mg - PieceValue[KNIGHT].mg, see<WHITE>(board, Move::fromAlgebraic(board, "f3g5")));

			board = Board::fromFen("r2qkb1r/p1pbpppp/1pnp1n2/1B6/Q2P4/2P1P3/PP3PPP/RNB1K1NR w KQkq - 0 6 ");
			Assert::AreEqual(PieceValue[KNIGHT].mg, see<WHITE>(board, Move::fromAlgebraic(board, "b5c6")));

			board = Board::fromFen("r2qkb1r/p1pbpppp/1pnp1n2/8/Q2P4/2P1P3/PP3PPP/RNB1K1NR w KQkq - 0 6 ");
			Assert::AreEqual(PieceValue[KNIGHT].mg - PieceValue[QUEEN].mg, see<WHITE>(board, Move::fromAlgebraic(board, "a4c6")));
		}

		TEST_METHOD(isPassedPawn_Test)
		{
			initSquareBB();

			Assert::IsFalse(isPassedPawn<WHITE>(D5, 0x8000000000000));
			Assert::IsFalse(isPassedPawn<WHITE>(D5, 0x4000000000000));
			Assert::IsFalse(isPassedPawn<WHITE>(D5, 0x40000000000));
			Assert::IsTrue(isPassedPawn<WHITE>(D5, 0x400000000));


			Assert::IsFalse(isPassedPawn<BLACK>(D4, 0x800));
			Assert::IsFalse(isPassedPawn<BLACK>(D4, 0x400));
			Assert::IsFalse(isPassedPawn<BLACK>(D4, 0x40000));
			Assert::IsTrue(isPassedPawn<BLACK>(D4, 0x4000000));
		}

		TEST_METHOD(pinnedPieces_Test)
		{
			initSquareBB();
			initAttackTables();
			initObstructedTable();
			Zobrist::initZobristHashing();

			Board board = Board::fromFen("8/4K3/6B1/8/8/3n4/2k5/8 b - - 0 1 ");
			Assert::IsTrue(board.pinnedPieces(BLACK) & board.pieces(BLACK, KNIGHT));

			board = Board::fromFen("4k3/8/2r3q1/8/8/2PN4/2K5/8 b - - 0 1 ");
			Assert::IsTrue(board.pinnedPieces(WHITE) & (board.pieces(WHITE, KNIGHT) | board.pieces(WHITE, PAWN)));

			board = Board::fromFen("8/3k4/3q4/3p2b1/5N2/3PP3/3K4/8 b - - 0 1 ");
			Assert::IsFalse(board.pinnedPieces(WHITE));

			board = Board::fromFen("8/3k4/3q4/8/8/3Q4/3R4/4K3 b - - 0 1 ");
			Assert::IsTrue(board.pinnedPieces(BLACK) & board.pieces(BLACK, QUEEN));
		}

		TEST_METHOD(flipBoard_Test)
		{
			initSquareBB();
			initAttackTables();
			initObstructedTable();
			Zobrist::initZobristHashing();

			Board board = Board::fromFen("r2qk1nr/1pp1bp2/p1n5/3pPbp1/7p/1NP1PN1P/PP3PPB/R2QKB1R w Kkq - 1 1");
			Board flipped_board = board.flip();
			Assert::AreEqual(std::string("r2qkb1r/pp3ppb/1np1pn1p/7P/3PpBP1/P1N5/1PP1BP2/R2QK1NR b KQk - 1 1"), flipped_board.fen());

			board = Board::fromFen("r2qkb1r/pp3ppb/1n2pn1p/7P/1PpPpBP1/P1N5/2P1BP2/R2QK1NR b - b3 1 1");
			flipped_board = board.flip();
			Assert::AreEqual(std::string("r2qk1nr/2p1bp2/p1n5/1pPpPbp1/7p/1N2PN1P/PP3PPB/R2QKB1R w - b6 1 1"), flipped_board.fen());
		}

		TEST_METHOD(evaluationSymmetry_Test)
		{
			initSquareBB();
			initAttackTables();
			initObstructedTable();
			Zobrist::initZobristHashing();

			std::vector<std::string> fens = {
				"r2qk1nr/1pp1bp2/p1n5/3pPbp1/7p/1NP1PN1P/PP3PPB/R2QKB1R w Kkq - 1 1",
				"r2k1b1r/pp2p1pp/4Qn2/1B1p4/4q3/4B3/PP3PPP/R4RK1 w - - 0 1",
			};

			for (const std::string &fen : fens)
			{
				Board board = Board::fromFen(fen);
				int eval1 = Evaluation::evaluate<WHITE>(board);
				int eval2 = Evaluation::evaluate<BLACK>(board.flip());
				Assert::AreEqual(eval1, eval2);

				eval1 = Evaluation::evaluate<BLACK>(board);
				eval2 = Evaluation::evaluate<WHITE>(board.flip());
				Assert::AreEqual(eval1, eval2);
			}
		}

		//TEST_METHOD(searchSymmetry_Test)
		//{
		//	initSquareBB();
		//	initAttackTables();
		//	initObstructedTable();
		//	Zobrist::initZobristHashing();

		//	const static int MAXDEPTH = 4;
		//	std::vector<std::string> fens = {
		//		"r2qk1nr/1pp1bp2/p1n5/3pPbp1/7p/1NP1PN1P/PP3PPB/R2QKB1R w Kkq - 1 1",
		//		"r2k1b1r/pp2p1pp/4Qn2/1B1p4/4q3/4B3/PP3PPP/R4RK1 w - - 0 1",
		//	};

		//	for (const std::string &fen : fens)
		//	{
		//		Board board = Board::fromFen(fen);
		//		int score1, score2;

		//		if (board.toMove() == WHITE)
		//		{
		//			score1 = Search::_alphaBeta<WHITE, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, MAXDEPTH, 0, nullptr);
		//			score2 = Search::_alphaBeta<BLACK, true, false>(board.flip(), -SCORE_INFINITY, SCORE_INFINITY, MAXDEPTH, 0, nullptr);
		//		}
		//		else
		//		{
		//			score1 = Search::_alphaBeta<BLACK, true, false>(board, -SCORE_INFINITY, SCORE_INFINITY, MAXDEPTH, 0, nullptr);
		//			score2 = Search::_alphaBeta<WHITE, true, false>(board.flip(), -SCORE_INFINITY, SCORE_INFINITY, MAXDEPTH, 0, nullptr);
		//		}

		//		Assert::AreEqual(score1, score2);
		//	}
		//}

		TEST_METHOD(Move_fromSan_Test)
		{
			init();

			Board board = Board::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");

			Assert::AreEqual(Move(PAWN, E2, E4, FLAG_DOUBLE_PUSH), Move::fromSan(board, "e4"));
			Assert::AreEqual(Move(PAWN, E2, E3, 0), Move::fromSan(board, "e3"));
			Assert::AreEqual(Move(KNIGHT, B1, C3, 0), Move::fromSan(board, "Nc3"));

			board = Board::fromFen("rn1qkb1r/ppp1pp2/7p/3p2p1/3Pn1b1/2N1PNBP/PPP2PP1/R2QKB1R b KQkq - 0 7 ");
			Assert::AreEqual(Move(BISHOP, G4, F3, FLAG_CAPTURE), Move::fromSan(board, "Bxf3"));

			board = Board::fromFen("r2qkb1r/ppp1pp2/7p/3p2p1/n2Pn3/P1N1PQBP/1PP2PP1/R3KB1R b Kkq - 8 13 ");
			Assert::AreEqual(Move(KNIGHT, E4, C3, FLAG_CAPTURE), Move::fromSan(board, "Nexc3"));
			Assert::AreEqual(Move(KNIGHT, E4, C3, FLAG_CAPTURE), Move::fromSan(board, "Ne4xc3"));

			board = Board::fromFen("r2qk2r/2p1ppb1/p6p/1pPp2p1/3Pn3/P3PQBP/2P2PP1/R3KB1R w Kkq b6 0 17  ");
			Assert::AreEqual(Move(PAWN, C5, B6, FLAG_CAPTURE|FLAG_EN_PASSANT), Move::fromSan(board, "cxb6e.p"));

			board = Board::fromFen("1k3q2/4P3/8/8/8/8/3K4/8 w - b6 0 17 ");
			Assert::AreEqual(Move(PAWN, E7, F8, ROOK, FLAG_CAPTURE), Move::fromSan(board, "exf8R"));
			Assert::AreEqual(Move(PAWN, E7, E8, KNIGHT), Move::fromSan(board, "e8N"));
			Assert::AreEqual(Move(PAWN, E7, E8, ROOK), Move::fromSan(board, "e8R+"));

			board = Board::fromFen("r2q1rk1/pppbbppp/2np1n2/1B2p1B1/4P3/2NP1N2/PPPQ1PPP/R3K2R w KQ - 4 8 ");
			Assert::AreEqual(Move::castle(WHITE, KINGSIDE), Move::fromSan(board, "0-0"));
			Assert::AreEqual(Move::castle(WHITE, QUEENSIDE), Move::fromSan(board, "0-0-0"));
		}
	};
}

