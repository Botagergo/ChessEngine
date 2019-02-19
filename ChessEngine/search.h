#pragma once

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

	static TranspositionTable tr_table(256);
	static TranspositionTable qs_tr_table(256);

	static struct {
		unsigned long long alpha_beta_nodes;
		unsigned long long quiescence_nodes;
		unsigned long long alpha_beta_cutoffs;
		unsigned long long quiescence_cutoffs;
		float avg_searched_moves;
		unsigned long long _move_gen_count;
		unsigned long long hash_move_cutoff;
		unsigned long long hash_score_returned;
	} Stats;

	void sendStats();
	void infoThread();

	void sendBestMove(Move move);
	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score);
	void sendCurrentMove(Move move, int pos);

	void startSearch(const Board & board, int maxdepth);
	void search(const Board & board, int maxdepth);

	template <Color toMove, bool root, bool pvNode>
	int alphaBeta(const Board & board, int alpha, int beta, int depthleft, int maxdepth, std::vector<Move> & pv)
	{
		if (Search::stop && maxdepth >= 5)
			return INVALID_SCORE;

		Move *hash_move = nullptr;

#ifdef TRANSPOSITION_TABLE
		auto hash = tr_table.probe(board.hash(), depthleft, alpha, beta);
		if (hash.first == alpha || hash.first == beta)
		{
			assert(!root);
			++Stats.hash_score_returned;
			return hash.first;
		}
		else if (hash.second != nullptr)
		{
			assert(!(hash.second->from() == A1 && hash.second->to() == A1));
			hash_move = hash.second;
		}
#endif

		if (depthleft <= 0)
			return quiescence<toMove>(board, alpha, beta);

		++Stats._move_gen_count;

		int curr_pos = 0;
		int alpha_orig = alpha;

		Move_Gen::MoveGenerator<toMove, false> mg(board, hash_move);
		for(int i = 1; !mg.end(); ++i, mg.next())
		{
			Board board_copy = board;

			if (!board_copy.makeMove(mg.curr()))
				continue;

			if (root)
				sendCurrentMove(mg.curr(), i);

			++Stats.alpha_beta_nodes;
			std::vector<Move> new_pv(depthleft - 1);

			int score;

#ifdef PV_SEARCH
			if (&mg.curr() == hash_move)
				score = -alphaBeta<~toMove, false, true>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
			else
			{
				score = -alphaBeta<~toMove, false, false>(board_copy, -(alpha + 1), -alpha, depthleft - 1, maxdepth, new_pv);
				if (score > alpha)
					score = -alphaBeta<~toMove, false, true>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
			}
#else
			score = -alphaBeta<~toMove, false, true>(board_copy, -beta, -alpha, depthleft - 1, maxdepth, new_pv);
#endif

			if (score == -INVALID_SCORE)
				return INVALID_SCORE;

			if (score >= beta)
			{
				++Stats.alpha_beta_cutoffs;
				if (&mg.curr() == hash_move)
					++Stats.hash_move_cutoff;

				tr_table.insertEntry(board.hash(), depthleft, beta, mg.curr(), CUT_NODE);
				return beta;
			}
			if (score > alpha)
			{
				alpha = score;

				pv.resize(depthleft);
				pv[0] = mg.curr();
				std::copy(new_pv.begin(), new_pv.end(), pv.begin() + 1);
			}
		}

		if (alpha != alpha_orig)
			tr_table.insertEntry(board.hash(), alpha, depthleft, pv[0], PV_NODE);
		else
			tr_table.insertEntry(board.hash(), alpha, depthleft, Move(), ALL_NODE);
		return alpha;
	}

	template <Color toMove>
	int quiescence(const Board & board, int alpha, int beta)
	{
//#ifdef TRANSPOSITION_TABLE
//		auto hash = qs_tr_table.probe(board.hash(), 0, alpha, beta);
//		if (hash.first != INVALID_SCORE)
//			return hash.first;
//#endif
		int score = Evaluation::evaluate<toMove>(board).mg;

		if (score >= beta)
			return beta;
		if (alpha < score)
			alpha = score;

		int alpha_orig = alpha;

		Move_Gen::MoveGenerator<toMove, true> mg(board, nullptr);
		for (int i = 1; !mg.end(); ++i, mg.next())
		{
			assert(mg.curr().isCapture());

			Board board_copy = board;
			if (!board_copy.makeMove(mg.curr()))
				continue;

			++Stats.quiescence_nodes;

			score = -quiescence<~toMove>(board_copy, -beta, -alpha);
			if (score >= beta)
			{
				++Stats.quiescence_cutoffs;
				qs_tr_table.insertEntry(board_copy.hash(), 0, score, Move(), CUT_NODE);
				return beta;
			}
			if (score > alpha)
				alpha = score;
		}

		if (alpha > alpha_orig)
			qs_tr_table.insertEntry(board.hash(), alpha, 0, Move(), PV_NODE);
		else
			qs_tr_table.insertEntry(board.hash(), alpha, 0, Move(), ALL_NODE);

		return alpha;
	}
}