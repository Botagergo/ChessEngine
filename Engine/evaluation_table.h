#pragma once

#include "move.h"
#include "board.h"

class EvaluationTable
{
public:
	EvaluationTable(size_t mb) { resize(mb); }
	~EvaluationTable() { if (_entries) delete _entries; }

	void resize(size_t mb)
	{
		if (_entries)
			delete _entries;

		_size = (mb * 1024 * 1024) / sizeof(Entry);
		_entries = new Entry[_size];

		clear();
	}

	void insert(unsigned long long hash, int score, ScoreType nodeType)
	{
		Entry *entry = _getEntry(hash);

		if (!entry->valid)
		{
			*entry = Entry(hash, score, nodeType);
			++_entry_count;
		}
	}

	int probe(unsigned long long hash, int alpha, int beta)
	{
		assert(hash != 0);

		std::pair<int, Move> pair = std::make_pair(SCORE_INVALID, Move());
		Entry *entry = _getEntry(hash);

		if (entry->hash == hash)
		{
			if ((entry->nodeType == LOWER_BOUND || entry->nodeType == EXACT) && beta <= entry->score)
			{
				return beta;
			}
			else if ((entry->nodeType == UPPER_BOUND || entry->nodeType == EXACT) && entry->score <= alpha)
			{
				return alpha;
			}
			else if (entry->nodeType == EXACT)
			{
				return entry->score;
			}
		}
	}

	void clear()
	{
		if (_entries != nullptr)
		{
			std::memset(_entries, 0, _size * sizeof(Entry));
			_entry_count = 0;
		}
	}

private:
	struct Entry
	{
		Entry(unsigned long long hash, int score, ScoreType nodeType) :
			hash(hash), score(score), nodeType(nodeType), valid(true) {}

		Entry() : valid(false) {}

		unsigned long long hash;
		int score;
		ScoreType nodeType;
		bool valid;
	};

	Entry * _getEntry(unsigned long long hash)
	{
		return &_entries[hash % _size];
	}

	size_t _size;
	Entry *_entries = nullptr;
	size_t _entry_count = 0;
};