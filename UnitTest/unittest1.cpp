#include "stdafx.h"
#include "CppUnitTest.h"

#include "attacks.h"
#include "bitboard.h"
#include "bitboard_iterator.h"
#include "board.h"
#include "util.h"
#include "zobrist.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Color>(const Color& c) { RETURN_WIDE_STRING((int)c); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Square>(const Square& s) { RETURN_WIDE_STRING((int)s); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<Piece>(const Piece& p) { RETURN_WIDE_STRING((int)p); }
template<> inline std::wstring  Microsoft::VisualStudio::CppUnitTestFramework::ToString<PieceType>(const PieceType& p) { RETURN_WIDE_STRING((int)p); }

namespace UnitTests
{
	TEST_CLASS(UnitTest1)
	{
	public:
		TEST_METHOD(shiftTest)
		{
			Bitboard bb = C64(0x920000091002002);

			Assert::AreEqual(Util::shift<NORTH>(0), C64(0));

			Assert::AreEqual(Util::shift<NORTH>(bb), C64(0x2000009100200200));
			Assert::AreEqual(Util::shift<NORTHEAST>(bb), C64(0x4000002200400400));
			Assert::AreEqual(Util::shift<EAST>(bb), C64(0x1240000022004004));
			Assert::AreEqual(Util::shift<SOUTHEAST>(bb), C64(0x12400000220040));
			Assert::AreEqual(Util::shift<SOUTH>(bb), C64(0x9200000910020));
			Assert::AreEqual(Util::shift<SOUTHWEST>(bb), C64(0x4100000480010));
			Assert::AreEqual(Util::shift<WEST>(bb), C64(0x410000048001001));
			Assert::AreEqual(Util::shift<NORTHWEST>(bb), C64(0x1000004800100100));
		}

		TEST_METHOD(flipTest)
		{
			Assert::AreEqual(Util::verticalFlip(C64(0)), C64(0));
			Assert::AreEqual(Util::verticalFlip(C64(0x1000200020020004)), C64(0x400022000200010));
		}

		TEST_METHOD(pieceTest)
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

		TEST_METHOD(pawnPushTargetsTest)
		{
			Assert::AreEqual(pawnPushTargets<WHITE>(C64(0x1000000000082100), FullBB), C64(0x29210000));
			Assert::AreEqual(pawnPushTargets<WHITE>(C64(0x2800002100), FullBB ^ C64(0x480001200000)), C64(0x200000010000));
		}

		TEST_METHOD(pawnAttacksTest)
		{
			Bitboard pawns = C64(0x881000000040000);
			Bitboard attacks = C64(0x420000000a000000);

			Assert::AreEqual(pawnAttacks<WHITE>(pawns), attacks);
			Assert::AreEqual(pawnAttacks<BLACK>(Util::verticalFlip(pawns)), Util::verticalFlip(attacks));
		}

		TEST_METHOD(knightAttacksTest)
		{
			initSquareBB();
			initAttackTables();
			
			Assert::AreEqual(knightAttacks(E4), C64(0x284400442800));
			Assert::AreEqual(knightAttacks(B4), C64(0x50800080500));
			Assert::AreEqual(knightAttacks(D7), C64(0x2200221400000000));
			Assert::AreEqual(knightAttacks(G5), C64(0xa0100010a00000));
			Assert::AreEqual(knightAttacks(E2), C64(0x28440044));
			Assert::AreEqual(knightAttacks(A5), C64(0x2040004020000));
			Assert::AreEqual(knightAttacks(D8), C64(0x22140000000000));
			Assert::AreEqual(knightAttacks(H5), C64(0x40200020400000));
			Assert::AreEqual(knightAttacks(E1), C64(0x284400));
			Assert::AreEqual(knightAttacks(B2), C64(0x5080008));
			Assert::AreEqual(knightAttacks(A2), C64(0x2040004));
			Assert::AreEqual(knightAttacks(A1), C64(0x20400));
			Assert::AreEqual(knightAttacks(A8), C64(0x4020000000000));
			Assert::AreEqual(knightAttacks(H8), C64(0x20400000000000));
			Assert::AreEqual(knightAttacks(H1), C64(0x402000));
		}

		TEST_METHOD(bishopAttacksTest)
		{
			initSquareBB();
			initAttackTables();

			Assert::AreEqual(bishopAttacks(A8, EmptyBB), C64(0x2040810204080));
			Assert::AreEqual(bishopAttacks(E6, EmptyBB), C64(0x4428002844820100));
			Assert::AreEqual(bishopAttacks(D1, EmptyBB), C64(0x8041221400));
			Assert::AreEqual(bishopAttacks(E4, C64(0x80000800280000)), C64(0x80402800280000));
			Assert::AreEqual(bishopAttacks(A1, C64(0x40200)), C64(0x200));
			Assert::AreEqual(bishopAttacks(D4, C64(0x21000140000)), C64(0x21400140000));
		}

		TEST_METHOD(rookAttacksTest)
		{
			initSquareBB();
			initAttackTables();

			Assert::AreEqual(rookAttacks(D3, EmptyBB), C64(0x808080808f70808));
			Assert::AreEqual(rookAttacks(A1, EmptyBB), C64(0x1010101010101fe));
			Assert::AreEqual(rookAttacks(A1, C64(0x104)), C64(0x106));
			Assert::AreEqual(rookAttacks(D3, C64(0x808000000120000)), C64(0x8080808160808));
			Assert::AreEqual(rookAttacks(H5, C64(0x802080000000)), C64(0x806080000000));
		}

		TEST_METHOD(queenAttacks)
		{
			initSquareBB();
			initAttackTables();
		}

		TEST_METHOD(bitboardIteratorTest)
		{
			BitboardIterator<Square> squares(C64(0));
			auto b1 = squares.begin();
			Assert::IsTrue(squares.end() == b1);

			squares = BitboardIterator<Square>(C64(0x21000008000004c0));

			auto expected1 = { G1, H1, C2, D5, A8, F8 };
			Assert::IsTrue(std::equal(squares.begin(), squares.end(), expected1.begin()));

			BitboardIterator<Bitboard> bitboards(C64(0));
			auto b2 = bitboards.begin();
			Assert::IsTrue(bitboards.end() == b2);

			bitboards = BitboardIterator<Bitboard>(C64(0x21000008000004c0));

			auto expected2 = { C64(0x40), C64(0x80), C64(0x400), C64(0x800000000), C64(0x100000000000000), C64(0x2000000000000000) };
			Assert::IsTrue(std::equal(bitboards.begin(), bitboards.end(), expected2.begin()));
		}

		TEST_METHOD(Move_test)
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
			move = Move::parse(board, "b4c3");

			Assert::IsTrue(move.from() == B4);
			Assert::IsTrue(move.to() == C3);
			Assert::IsTrue(move.pieceType() == BISHOP);
			Assert::IsTrue(move.isCapture());
			Assert::IsFalse(move.isDoublePush());
			Assert::IsFalse(move.isEnPassant());
			Assert::IsFalse(move.isPromotion());
		}

		TEST_METHOD(Board_fromFenTest) //TODO: add more tests
		{
			initSquareBB();
			initAttackTables();
 
			Board b = Board::fromFen("5k2/2p5/1n2p3/8/4pP2/4P3/6PP/R2QK1NR b Q f3 0 1 ");

			Assert::IsFalse(b.canCastle(BLACK, KINGSIDE));
			Assert::IsFalse(b.canCastle(BLACK, QUEENSIDE));
			Assert::IsTrue(b.canCastle(WHITE, QUEENSIDE));
			Assert::IsFalse(b.canCastle(WHITE, KINGSIDE));

			Assert::AreEqual(b.toMove(), BLACK);

			Assert::AreEqual(b.pieces(WHITE, PAWN), C64(0x2010c000));
			Assert::AreEqual(b.pieces(WHITE, KNIGHT), C64(0x40));
			Assert::AreEqual(b.pieces(WHITE, BISHOP), C64(0x0));
			Assert::AreEqual(b.pieces(WHITE, ROOK), C64(0x81));
			Assert::AreEqual(b.pieces(WHITE, QUEEN), C64(0x8));
			Assert::AreEqual(b.pieces(WHITE, KING), C64(0x10));

			Assert::AreEqual(b.pieces(BLACK, PAWN), C64(0x4100010000000));
			Assert::AreEqual(b.pieces(BLACK, KNIGHT), C64(0x20000000000));
			Assert::AreEqual(b.pieces(BLACK, BISHOP), C64(0x0));
			Assert::AreEqual(b.pieces(BLACK, ROOK), C64(0x0));
			Assert::AreEqual(b.pieces(BLACK, QUEEN), C64(0x0));
			Assert::AreEqual(b.pieces(BLACK, KING), C64(0x2000000000000000));

			Assert::AreEqual(b.enPassantTarget(), F3);
			Assert::AreEqual(b.enPassantCaptureTarget(), F4);

			Assert::AreEqual(b.occupied(WHITE), C64(0x2010c0d9));
			Assert::AreEqual(b.occupied(BLACK), C64(0x2004120010000000));

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

		TEST_METHOD(zobristTest)
		{
			initSquareBB();
			initAttackTables();
			Zobrist::initZobristHashing();

			vector<string> fen1 = { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
				"r1bqkbnr/ppppp1pp/2n5/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"};
			vector<vector<string>> moves = {
				{"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6", "e1g1", "d7d6", "b1c3", "f8e7", "d2d3", "e8g8"},
				{"g1f3", "g8f6", "f3g1", "f6g8"}
			};
			vector<string> fen2 = { "r1bq1rk1/ppp1bppp/2np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 1 7 ",
				"r1bqkbnr/ppppp1pp/2n5/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"};

			vector<bool> equals = { true, false };

			for (int i = 0; i < fen1.size(); ++i)
			{
				Board board = Board::fromFen(fen1[i]);

				for (std::string move : moves[i])
				{
					board.makeMove(Move::parse(board, move));
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
	};
}

