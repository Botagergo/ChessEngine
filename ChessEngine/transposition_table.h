#pragma once

#include "move.h"
#include "board.h"

class TranspositionTable
{
public:
	TranspositionTable(size_t mb) { resize(mb); }
	~TranspositionTable() { if (_entries) delete _entries; }

	struct Stat
	{
		unsigned long long pv_node_hits = 0;
		unsigned long long cut_node_hits = 0;
		unsigned long long all_node_hits = 0;
		unsigned long long unknown_hits = 0;
		unsigned long long failed_inserts = 0;
	};

	mutable Stat _stats;

	void resize(size_t mb)
	{
		if (_entries)
			delete _entries;

		_size = (mb * 1024 * 1024) / sizeof(Entry);
		_entries = new Entry[_size];

		clear();
	}

	void insert(unsigned long long hash, int depth, int score, Move move, ScoreType nodeType)
	{
		Entry *entry = _getEntry(hash);

		if (!entry->valid)
		{
			*entry = Entry(hash, depth, score, move, nodeType);
			++_entry_count;
		}
		else if (entry->depth < depth)
			*entry = Entry(hash, depth, score, move, nodeType);
		else if (entry->hash != hash)
			++_stats.failed_inserts;
	}

	std::pair<int, Move> probe(unsigned long long hash, int depth, int alpha, int beta)
	{
		assert(hash != 0);
		assert(_entries != nullptr);

		std::pair<int, Move> pair = std::make_pair(SCORE_INVALID, Move());
		Entry *entry = _getEntry(hash);

		if (entry->hash == hash)
		{
			if ((entry->nodeType == LOWER_BOUND || entry->nodeType == EXACT) && beta <= entry->score)
			{
				++_stats.cut_node_hits;
				pair = std::make_pair(beta, entry->move);
			}
			else if ((entry->nodeType == UPPER_BOUND || entry->nodeType == EXACT) && entry->score <= alpha)
			{
				++_stats.all_node_hits;
				pair = std::make_pair(alpha, Move());
			}
			else if (entry->nodeType == EXACT)
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
		std::memset(_entries, 0, _size * sizeof(Entry));
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
	struct Entry
	{
		Entry(unsigned long long hash, int depth, int score, Move move, ScoreType nodeType) :
			hash(hash), depth(depth), score(score), move(move), nodeType(nodeType), valid(true) {}

		Entry() : valid(false) {}

		unsigned long long hash;
		int depth;
		int score;
		Move move;
		ScoreType nodeType;
		bool valid;
	};

	Entry * _getEntry(unsigned long long hash)
	{
		return &_entries[hash % _size];
	}

	size_t _size;
	Entry *_entries;
	size_t _entry_count = 0;
};