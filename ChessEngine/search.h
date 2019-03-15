#pragma once

#include <chrono>
#include <thread>

#include "board.h"
#include "config.h"
#include "evaluation.h"
#include "movegen.h"
#include "transposition_table.h"
#include "types.h"

using namespace std;

namespace Search
{
	extern std::thread search_thrd;

	extern bool debug;
	extern bool stop;

	static TranspositionTable ab_tr_table(128);
	static TranspositionTable qs_tr_table(128);

	static std::pair<Move, Move> killer_moves[MAX_DEPTH];

	static std::array<unsigned long long, MAX_DEPTH> history;

	static struct {
		unsigned long long alpha_beta_nodes;
		unsigned long long quiescence_nodes;
		unsigned long long alpha_beta_cutoffs;
		unsigned long long quiescence_cutoffs;
		unsigned long long _move_gen_count;
		unsigned long long hash_move_cutoffs;
		unsigned long long killer_move_cutoffs;
		unsigned long long hash_score_returned;
		unsigned long long pv_search_research_count;
		float avg_searched_moves;
	} Stats;

	static struct
	{
		unsigned long long last_node_count;
		std::chrono::steady_clock::time_point last_time;
	} SearchInfo;

	void sendStats();
	void updateNodesPerSec();
	void infoThread();

	void sendBestMove(Move move);
	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score, bool mate);
	void sendCurrentMove(Move move, int pos);

	void startSearch(const Board & board, int maxdepth);
	void search(const Board & board, int maxdepth);

	bool isRepetition(unsigned long long hash, int ply);

	template <Color toMove, bool pvNode, bool nullMoveAllowed>
	int alphaBeta(const Board & board, int alpha, int beta, int depthleft, int maxdepth, std::vector<Move> & pv)
	{
		++Stats.alpha_beta_nodes;
		int ply = maxdepth - depthleft;

		history[ply] = board.hash();

		if (Search::stop && maxdepth >= 5)
			return SCORE_INVALID;

		Move hash_move = Move();

#ifdef TRANSPOSITION_TABLE
		auto hash = ab_tr_table.probe(board.hash(), depthleft, alpha, beta);
		if (hash.first == alpha || hash.first == beta)
		{
			assert(ply);
			++Stats.hash_score_returned;
			return hash.first;
		}
		else if (hash.second.isValid())
		{
			assert(board.pieceAt(hash.second.from()) != NO_PIECE);
			hash_move = hash.second;
		}
#endif

		if (depthleft <= 0)
			return quiescence<toMove>(board, alpha, beta);

#ifdef NULL_MOVE_PRUNING
		if (nullMoveAllowed && depthleft >= 4 && !board.isInCheck(toMove))
		{
			std::vector<Move> new_pv(depthleft - 1);
			Board board_copy = board;
			board_copy.makeMove(Move::nullMove());

			int score = -alphaBeta<~toMove, false, false>(board_copy, -beta, -beta + 1, depthleft - 2, maxdepth, new_pv);
			if (score >= beta)
				return beta;
		}
#endif

		++Stats._move_gen_count;

		int curr_pos = 0;
		int alpha_orig = alpha;
		int searched_moves = 0;
		int score;
		std::vector<Move> new_pv(depthleft - 1);

		MoveGen::MoveGenerator<toMove, false> mg(board, hash_move, killer_moves[ply]);
		for(int i = 1; !mg.end(); ++i, mg.next())
		{
			Board board_copy = board;

			if (!board_copy.makeMove(mg.curr()))
				continue;

			if (isRepetition(board_copy.hash(), ply))
			{
				score = 0;
				goto SearchEnd;
			}

			if (ply == 0)
				sendCurrentMove(mg.curr(), i);



#ifdef PV_SEARCH
			if (searched_moves < 7)
				score = -alphaBeta<~toMove, true, false>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
			else
			{
				score = -alphaBeta<~toMove, false, true>(board_copy, -(alpha + 1), -alpha, depthleft - 1, maxdepth, new_pv);
				if (score > alpha)
				{
					++Stats.pv_search_research_count;
					score = -alphaBeta<~toMove, true, false>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
				}
			}
#else
			Square from = mg.curr().from();
			Square to = mg.curr().to();

			score = -alphaBeta<~toMove, true, false>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
#endif

			if (score == -SCORE_INVALID)
				return SCORE_INVALID;

SearchEnd:

			++searched_moves;

			if (score >= beta)
			{
				++Stats.alpha_beta_cutoffs;

				if (mg.curr() == hash_move)
					++Stats.hash_move_cutoffs;

				if (!mg.curr().isCapture())
				{
					if (mg.curr() == killer_moves[ply].first || mg.curr() == killer_moves[ply].second)
						++Stats.killer_move_cutoffs;

					killer_moves[ply].second = killer_moves[ply].first;
					killer_moves[ply].first = mg.curr();
				}


				assert(mg.curr() != Move());
				ab_tr_table.insertEntry(board.hash(), depthleft, beta, mg.curr(), CUT_NODE);
				return beta;
			}
			if (score > alpha)
			{
				if (SCORE_MIN_MATE <= score && score <= SCORE_MAX_MATE)
					score -= 1;
				else if (-SCORE_MAX_MATE <= score && score <= -SCORE_MIN_MATE)
					score += 1;

				alpha = score;

				pv.resize(depthleft);
				pv[0] = mg.curr();
				assert(pv[0].from() != 0 || pv[0].to() != 0);
				std::copy(new_pv.begin(), new_pv.end(), pv.begin() + 1);

				if (alpha == SCORE_MAX_MATE - 1)
					break;
			}
		}

		if (searched_moves == 0)
		{
			if (board.isInCheck(toMove))
				return -SCORE_MAX_MATE;
			else
				return SCORE_DRAW;
		}

		if (alpha != alpha_orig)
		{
			assert(pv[0] != Move());
			ab_tr_table.insertEntry(board.hash(), depthleft, alpha, pv[0], PV_NODE);
		}
		else
			ab_tr_table.insertEntry(board.hash(), depthleft, alpha, Move(), ALL_NODE);

		return alpha;
	}

	template <Color toMove>
	int quiescence(const Board & board, int alpha, int beta)
	{
		++Stats.quiescence_nodes;

#ifdef TRANSPOSITION_TABLE
		auto hash = qs_tr_table.probe(board.hash(), 0, alpha, beta);
		if (hash.first != SCORE_INVALID)
			return hash.first;
#endif
		int score = Evaluation::evaluate<toMove>(board);

		if (score >= beta)
			return beta;
		if (alpha < score)
			alpha = score;

		int alpha_orig = alpha;

		MoveGen::MoveGenerator<toMove, true> mg(board, Move());
		for (int i = 1; !mg.end(); ++i, mg.next())
		{
			assert(mg.curr().isCapture());

			Board board_copy = board;
			if (!board_copy.makeMove(mg.curr()))
				continue;

			score = -quiescence<~toMove>(board_copy, -beta, -alpha);
			if (score >= beta)
			{
				++Stats.quiescence_cutoffs;
				qs_tr_table.insertEntry(board.hash(), 0, score, Move(), CUT_NODE);
				return beta;
			}
			if (score > alpha)
				alpha = score;
		}

		if (alpha > alpha_orig)
			qs_tr_table.insertEntry(board.hash(), 0, alpha, Move(), PV_NODE);
		else
			qs_tr_table.insertEntry(board.hash(), 0, alpha, Move(), ALL_NODE);

		return alpha;
	}
}