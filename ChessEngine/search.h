#pragma once

#include <chrono>
#include <thread>

#include "board.h"
#include "config.h"
#include "evaluation.h"
#include "evaluation_table.h"
#include "movegen.h"
#include "transposition_table.h"
#include "types.h"

using namespace std;

namespace Search
{
	extern std::thread search_thrd;

	extern bool debug;
	extern bool stop;
	extern bool ponder;
	extern bool passed_maxdepth;

	static TranspositionTable transposition_table(DEFAULT_HASH_TABLE_SIZE);
	static EvaluationTable evaluation_table(DEFAULT_HASH_TABLE_SIZE);

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

	void sendBestMove(Move move, Move ponder_move);
	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score, bool mate);
	void sendCurrentMove(Move move, int pos);

	void startSearch(const Board & board, int maxdepth);
	void search(const Board & board, int maxdepth);

	bool isRepetition(unsigned long long hash, int ply);

	template <Color toMove, bool pvNode, bool nullMoveAllowed>
	int alphaBeta(const Board & board, int alpha, int beta, int depthleft, int ply, std::vector<Move> * pv)
	{
		++Stats.alpha_beta_nodes;

		history[ply] = board.hash();

		if (Search::stop)
			return SCORE_INVALID;

		if (passed_maxdepth && !ponder)
			return SCORE_INVALID;

		Move hash_move = Move();

#ifdef TRANSPOSITION_TABLE
		auto hash = transposition_table.probe(board.hash(), depthleft, alpha, beta);
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
			Board board_copy = board;
			board_copy.makeMove(Move::nullMove());

			int score = -alphaBeta<~toMove, false, false>(board_copy, -beta, -beta + 1, depthleft - 2, ply + 1, nullptr);
			if (score >= beta)
				return beta;
		}
#endif

		++Stats._move_gen_count;
		int curr_pos = 0;
		int alpha_orig = alpha;
		int searched_moves = 0;
		int score;
		std::vector<Move> new_pv;
		std::vector<Move> * new_pv_ptr = nullptr;

		if (pvNode)
		{
			new_pv.resize(depthleft - 1);
			new_pv_ptr = &new_pv;
		}

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
			if (searched_moves < 1)
				score = -alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
			else
			{
				score = -alphaBeta<~toMove, false, true>(board_copy, -(alpha + 1), -alpha, depthleft - 1, ply + 1, nullptr);
				if (score > alpha)
				{
					++Stats.pv_search_research_count;
					score = -alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
				}
			}
#else
			Square from = mg.curr().from();
			Square to = mg.curr().to();
			score = -alphaBeta<~toMove, true, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
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
				transposition_table.insert(board.hash(), depthleft, beta, mg.curr(), LOWER_BOUND);
				return beta;
			}
			if (score > alpha)
			{
				if (SCORE_MIN_MATE <= score && score <= SCORE_MAX_MATE)
					score -= 1;
				else if (-SCORE_MAX_MATE <= score && score <= -SCORE_MIN_MATE)
					score += 1;

				alpha = score;

				if (pvNode)
				{
					assert(new_pv_ptr != nullptr && pv != nullptr);

					pv->resize(depthleft);

					(*pv)[0] = mg.curr();
					std::copy(new_pv.begin(), new_pv.end(), pv->begin() + 1);
				}

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

		if (alpha == alpha_orig)
		{
			transposition_table.insert(board.hash(), depthleft, alpha, Move(), UPPER_BOUND);
		}
		else if (pv)
		{
			assert((*pv)[0] != Move());
			transposition_table.insert(board.hash(), depthleft, alpha, (*pv)[0], EXACT);
		}

		return alpha;
	}

	template <Color toMove>
	int quiescence(const Board & board, int alpha, int beta)
	{
		++Stats.quiescence_nodes;

		int score = evaluation_table.probe(board.hash(), alpha, beta);
		if (score != SCORE_INVALID)
		{
			score = Evaluation::evaluate<toMove>(board);
			evaluation_table.insert(board.hash(), score, EXACT);
		}

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
				evaluation_table.insert(board.hash(), score, LOWER_BOUND);
				return beta;
			}
			if (score > alpha)
				alpha = score;
		}

		if (alpha > alpha_orig)
			evaluation_table.insert(board.hash(), alpha, EXACT);
		else
			evaluation_table.insert(board.hash(), alpha, UPPER_BOUND);

		return alpha;
	}
}