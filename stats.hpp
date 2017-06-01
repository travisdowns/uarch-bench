/*
 * Really simple descriptive stats.
 *
 * stats.hpp
 */

#ifndef STATS_HPP_
#define STATS_HPP_

#include <algorithm>
#include <limits>
#include <iterator>

namespace Stats {

class DescriptiveStats {
	double min_, max_, avg_, median_;
	size_t count_;
public:
	DescriptiveStats(double min, double max, double avg, double median, size_t count) :
		min_(min), max_(max), avg_(avg), median_(median), count_(count) {}

	double getAvg() const {
		return avg_;
	}

	size_t getCount() const {
		return count_;
	}

	double getMax() const {
		return max_;
	}

	double getMin() const {
		return min_;
	}

	double getMedian() const {
		return median_;
	}
};

template <typename iter_type>
typename std::iterator_traits<iter_type>::value_type median(iter_type first, iter_type last) {
	if (first == last) {
		throw std::logic_error("can't get median of empty range");
	}
	using T = typename std::iterator_traits<iter_type>::value_type;
	std::vector<T> copy(first, last);
	std::sort(copy.begin(), copy.end());
	size_t sz = copy.size(), half_sz = sz / 2;
	return sz % 2 ? copy[half_sz] : (copy[half_sz - 1] + copy[half_sz]) / 2;
}


template <typename iter_type>
DescriptiveStats get_stats(iter_type first, iter_type last) {
	using dlimits = std::numeric_limits<double>;
	double min = dlimits::max(), max = dlimits::min(), total = 0;
	size_t count = 0;
	for (iter_type itr = first; itr != last; itr++) {
		auto val = *itr;
		double vald = val;
		if (vald < min) min = vald;
		if (vald > max) max = vald;
		total += vald;
		count++;
	}

	return DescriptiveStats(min, max, total / count, median(first, last), count);
}



std::ostream& operator<<(std::ostream &os, const DescriptiveStats &stats) {
	os << "min=" << stats.getMin() << ", median=" << stats.getMedian() << ", avg=" << stats.getAvg()
			<< ", max=" << stats.getMax() << ", n=" << stats.getCount();
	return os;
}

} // namepsace Stats

#endif /* STATS_HPP_ */
