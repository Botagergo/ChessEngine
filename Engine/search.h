#pragma once

#include <chrono>
#include <thread>

#include "board.h"
#include "config.h"
#include "evaluation.h"
#include "evaluation_table.h"
#include "movegen.h"
#include "moveselect.h"
#include "transposition_table.h"
#include "types.h"

using namespace std;

namespace Search
{

	extern std::thread search_thrd;

	struct SearchInfo
	{
		SearchInfo() : transposition_table(DEFAULT_HASH_TABLE_SIZE), evaluation_table(DEFAULT_HASH_TABLE_SIZE) {};

		TranspositionTable transposition_table;
		EvaluationTable evaluation_table;
		std::pair<Move, Move> killer_moves[MAX_DEPTH];
		std::array<u64, MAX_DEPTH> history;

		bool debug = false;
		bool stop = false;
		bool ponder = false;
		bool passed_maxdepth = false;
		bool sendOutput = true;

		u64 move_gen_count;
		u64 searched_moves_sum;

		struct
		{
			u64 alpha_beta_nodes;
			u64 quiescence_nodes;
			u64 alpha_beta_cutoffs;
			u64 quiescence_cutoffs;
			u64 hash_move_cutoffs;
			u64 killer_move_cutoffs;
			u64 hash_score_returned;
			u64 pv_search_research_count;
			float avg_searched_moves;
		} Stats;

		struct
		{
			u64 last_node_count;
			u64 node_count;
			std::chrono::steady_clock::time_point last_time;
			int nodes_per_sec;
		} NodeCountInfo;
	};

	extern SearchInfo searchInfo;

	void sendStats();
	void updateNodesPerSec();
	void infoThread();

	void sendBestMove(Move move, Move ponder_move);
	void sendPrincipalVariation(const std::vector<Move> & pv, int depth, int score, bool mate);
	void sendCurrentMove(Move move, int pos);
	void sendNodeInfo(u64 node_count, u64 nodes_per_sec);
	void sendHashfull(int permill);

	void startSearch(const Board & board, int maxdepth);
	void search(const Board & board, int maxdepth, Move *bestMove = nullptr);

	bool isRepetition(u64 hash, int ply);

	template <Color toMove, bool pvNode, bool nullMoveAllowed>
	int alphaBeta(const Board & board, int alpha, int beta, int depthleft, int ply, std::vector<Move> * pv)
	{
		ASSERT(depthleft >= 0);

		++searchInfo.Stats.alpha_beta_nodes;

		if (searchInfo.Stats.alpha_beta_nodes % 10000 == 0)
		{
			sendNodeInfo(searchInfo.NodeCountInfo.node_count, searchInfo.NodeCountInfo.nodes_per_sec);
			sendHashfull((int)(searchInfo.transposition_table.usage() * 1000));
		}

		searchInfo.history[ply] = board.hash();

		if (searchInfo.stop || searchInfo.passed_maxdepth && !searchInfo.ponder)
			return SCORE_INVALID;

		Move hash_move = Move();

		// Transposition table lookup
		auto hash = searchInfo.transposition_table.probe(board.hash(), depthleft, alpha, beta);
		if (hash.first == alpha || hash.first == beta)
		{
			ASSERT(ply);
			++searchInfo.Stats.hash_score_returned;
			return hash.first;
		}
		else if (hash.second.isValid())
		{
			ASSERT(board.pieceAt(hash.second.from()) != NO_PIECE);
			hash_move = hash.second;
		}

		if (depthleft == 0)
			return quiescence<toMove>(board, alpha, beta);

		if (nullMoveAllowed && depthleft >= 4 && !board.isInCheck(toMove))
		{
			Board board_copy = board;
			board_copy.makeMove(Move::nullMove());

			int score = -alphaBeta<~toMove, false, false>(board_copy, -beta, -beta + 1, depthleft - 2, ply + 1, nullptr);
			if (score >= beta)
				return beta;
		}

		int curr_pos = 0;
		int searched_moves = 0;
		int alpha_orig = alpha;
		int score;
		std::vector<Move> new_pv;
		std::vector<Move> * new_pv_ptr = nullptr;

		if (pvNode)
		{
			new_pv.resize(MAX_DEPTH);
			new_pv_ptr = &new_pv;
		}

		MoveSelect::MoveSelector<toMove, false> mg(board, hash_move, searchInfo.killer_moves[ply]);
		searchInfo.move_gen_count++;

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

			if (ply == 0 && searchInfo.sendOutput)
				sendCurrentMove(mg.curr(), searched_moves + 1);

			if (searched_moves < 1)
				score = -alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
			else
			{
				score = -alphaBeta<~toMove, false, true>(board_copy, -(alpha + 1), -alpha, depthleft - 1, ply + 1, nullptr);
				if (score > alpha)
				{
					++searchInfo.Stats.pv_search_research_count;
					score = -alphaBeta<~toMove, pvNode, false>(board_copy, -beta, -alpha, depthleft - 1, ply + 1, new_pv_ptr);
				}
			}

			if (score == -SCORE_INVALID)
				return SCORE_INVALID;

SearchEnd:

			++searched_moves;
			searchInfo.searched_moves_sum++;

			if (score >= beta)
			{
				++searchInfo.Stats.alpha_beta_cutoffs;

				if (mg.curr() == hash_move)
					++searchInfo.Stats.hash_move_cutoffs;

				if (mg.curr().isQuiet())
				{
					if (mg.curr() == searchInfo.killer_moves[ply].first || mg.curr() == searchInfo.killer_moves[ply].second)
						++searchInfo.Stats.killer_move_cutoffs;

					searchInfo.killer_moves[ply].second = searchInfo.killer_moves[ply].first;
					searchInfo.killer_moves[ply].first = mg.curr();
				}

				assert(mg.curr() != Move());
				searchInfo.transposition_table.insert(board.hash(), depthleft, beta, mg.curr(), LOWER_BOUND);
				return beta;
			}

			if (score > alpha)
			{
				alpha = score;

				if (pv)
				{
					assert(new_pv_ptr != nullptr && pv != nullptr);

					pv->resize(MAX_DEPTH);

					(*pv)[0] = mg.curr();
					std::copy(new_pv.begin(), new_pv.begin() + depthleft - 1, pv->begin() + 1);
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
			searchInfo.transposition_table.insert(board.hash(), depthleft, alpha, Move(), UPPER_BOUND);
		}
		else if (pv)
		{
			assert((*pv)[0] != Move());
			searchInfo.transposition_table.insert(board.hash(), depthleft, alpha, (*pv)[0], EXACT);
		}

		return alpha;
	}

	template <Color toMove>
	int quiescence(const Board & board, int alpha, int beta)
	{
		++searchInfo.Stats.quiescence_nodes;

		int stand_pat = searchInfo.evaluation_table.probe(board.hash(), alpha, beta);
		if (stand_pat != SCORE_INVALID)
		{
			stand_pat = Evaluation::evaluate<toMove>(board);
			searchInfo.evaluation_table.insert(board.hash(), stand_pat, EXACT);
		}

		if (stand_pat >= beta)
			return beta;

		if (alpha < stand_pat)
			alpha = stand_pat;

		int alpha_orig = alpha;

		MoveSelect::MoveSelector<toMove, true> mg(board, Move());
		for (int i = 1; !mg.end(); ++i, mg.next())
		{
			assert(!mg.curr().isQuiet());

			Board board_copy = board;
			if (!board_copy.makeMove(mg.curr()))
				continue;

			int score = -quiescence<~toMove>(board_copy, -beta, -alpha);
			if (score >= beta)
			{
				++searchInfo.Stats.quiescence_cutoffs;
				searchInfo.evaluation_table.insert(board.hash(), score, LOWER_BOUND);
				return beta;
			}
			if (score > alpha)
				alpha = score;
		}

		if (alpha > alpha_orig)
			searchInfo.evaluation_table.insert(board.hash(), alpha, EXACT);
		else
			searchInfo.evaluation_table.insert(board.hash(), alpha, UPPER_BOUND);

		return alpha;
	}
}