#pragma once

#include "types.h"
#include "util.h"

template <typename T>
class BitboardIterator
{
public:
	BitboardIterator(Bitboard bb) : _bb(bb) {}

	class iterator
	{
	public:
		friend class BitboardIterator;
		iterator(const iterator &sq_i) : _bb(sq_i._bb) {}

		typedef iterator self_type;
		typedef std::input_iterator_tag iterator_category;
		typedef T value_type;
		typedef T& reference;
		typedef T* pointer;
		typedef int difference_type;

		value_type operator*() const
		{
			assert(false);
		}

		self_type& operator++()
		{
			Util::clearLSB(_bb);
			return *this;
		}

		self_type operator++(int)
		{
			self_type ret = *this;
			Util::clearLSB(_bb);
			return ret;
		}

		self_type& operator=(const iterator &other)
		{
			_bb = other._bb;
			return *this;
		}

		bool operator==(const iterator &other) const
		{
			return _bb == other._bb;
		}

		bool operator!=(const iterator &other) const
		{
			return !operator==(other);
		}

	private:
		iterator(Bitboard bb) : _bb(bb) {}

		Bitboard _bb;
	};

	iterator begin()
	{
		return iterator(_bb);
	}

	iterator end()
	{
		return iterator(0);
	}

private:
	Bitboard _bb;
};

template<>
typename BitboardIterator<Square>::iterator::value_type BitboardIterator<Square>::iterator::operator*() const
{
	return Util::bitScanForward(_bb);
}

template<>
typename BitboardIterator<Bitboard>::iterator::value_type BitboardIterator<Bitboard>::iterator::operator*() const
{
	return Util::getLSB(_bb);
}