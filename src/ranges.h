#ifndef RANGES_H
#define RANGES_H

#include <algorithm>

// This is a (partial) reimplementation of c++20 'std::ranges' stuff.
// In the future we can:
// * Replace '#include "ranges.h"' with '#include <ranges>'.
// * Replace 'ranges::xxx()' with 'std::ranges::xxx()'.

// Reimplementation of c++20 std::identity.
struct identity {
	template<typename T>
	[[nodiscard]] constexpr T&& operator()(T&& t) const noexcept {
		return std::forward<T>(t);
	}
};

namespace ranges {

template<typename OutputRange, typename T>
void fill(OutputRange&& range, const T& value)
{
	std::fill(std::begin(range), std::end(range), value);
}

template<typename InputRange, typename T>
[[nodiscard]] auto find(InputRange&& range, const T& value)
{
	return std::find(std::begin(range), std::end(range), value);
}

template<typename InputRange, typename T, typename Proj>
[[nodiscard]] auto find(InputRange&& range, const T& value, Proj proj)
{
	return find_if(std::forward<InputRange>(range),
	               [&](const auto& e) { return std::invoke(proj, e) == value; });
}

template<typename InputRange, typename UnaryPredicate>
[[nodiscard]] auto find_if(InputRange&& range, UnaryPredicate pred)
{
	return std::find_if(std::begin(range), std::end(range), pred);
}

template<typename ForwardRange, typename T, typename Compare = std::less<>, typename Proj = identity>
[[nodiscard]] auto lower_bound(ForwardRange&& range, const T& value, Compare comp = {}, Proj proj = {})
{
	auto comp2 = [&](const auto& x, const auto& y) {
		return comp(std::invoke(proj, x), y);
	};
	return std::lower_bound(std::begin(range), std::end(range), value, comp2);
}

template<typename ForwardRange, typename T, typename Compare = std::less<>, typename Proj = identity>
[[nodiscard]] auto upper_bound(ForwardRange&& range, const T& value, Compare comp = {}, Proj proj = {})
{
	auto comp2 = [&](const auto& x, const auto& y) {
		return comp(x, std::invoke(proj, y));
	};
	return std::upper_bound(std::begin(range), std::end(range), value, comp2);
}

} // namespace ranges

#endif
