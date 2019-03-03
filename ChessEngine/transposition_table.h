#pragma once

#include "move.h"
#include "board.h"


enum NodeType
{
	PV_NODE,
	CUT_NODE,
	ALL_NODE
};

struct TTEntry
{
	TTEntry(unsigned long long hash, int depth, int score, Move move, NodeType nodeType) :
		hash(hash), depth(depth), score(score), move(move), nodeType(nodeType), valid(true) {}

	TTEntry() : valid(false) {}

	unsigned long long hash;
	int depth;
	int score;
	Move move;
	NodeType nodeType;
	bool valid;
};

class TranspositionTable
{
public:
	TranspositionTable(size_t mb) { resize(mb); }
	~TranspositionTable() { delete _entries; }

	struct Stat
	{
		unsigned long long pv_node_hits = 0;
		unsigned long long cut_node_hits = 0;
		unsigned long long all_node_hits = 0;
		unsigned long long unknown_hits = 0;
	};

	mutable Stat _stats;

	void resize(size_t mb)
	{
		delete _entries;

		_size = (mb * 1024 * 1024) / sizeof(TTEntry);
		_entries = new TTEntry[_size];

		clear();
	}

	void insertEntry(unsigned long long hash, int depth, int score, Move move, NodeType nodeType)
	{
		TTEntry *entry = _getEntry(hash);

		if (!entry->valid)
		{
			*entry = TTEntry(hash, depth, score, move, nodeType);
			++_entry_count;
		}
		else if(entry->depth < depth)
			*entry = TTEntry(hash, depth, score, move, nodeType);
	}

	std::pair<int, Move> probe(unsigned long long hash, int depth, int alpha, int beta)
	{
		assert(hash != 0);

		std::pair<int, Move> pair = std::make_pair(SCORE_INVALID, Move());
		TTEntry *entry = _getEntry(hash);

		if (entry->hash == hash)
		{
			if ((entry->nodeType == CUT_NODE || entry->nodeType == PV_NODE) && beta <= entry->score)
			{
				++_stats.cut_node_hits;
				pair = std::make_pair(beta, entry->move);
			}
			else if ((entry->nodeType == ALL_NODE || entry->nodeType == PV_NODE) && entry->score <= alpha)
			{
				++_stats.all_node_hits;
				pair = std::make_pair(alpha, Move());
			}
			else if (entry->nodeType == PV_NODE)
			{
				++_stats.pv_node_hits;
				pair = std::make_pair(entry->score, entry->move);
			}

			if (entry->depth < depth)
				pair.first = SCORE_INVALID;
		}

		return pair;
	}

	void clear()
	{
		std::memset(_entries, 0, _size * sizeof(TTEntry));
		_entry_count = 0;
	}

	const Stat *getStats() const
	{
		return &_stats;
	}

	double usage() const
	{
		return (double)_entry_count / (double)_size;
	}

private:
	TTEntry * _getEntry(unsigned long long hash)
	{
		return &_entries[hash % _size];
	}

	size_t _size;
	TTEntry *_entries;
	size_t _entry_count = 0;
};