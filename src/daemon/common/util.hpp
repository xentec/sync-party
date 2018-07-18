#pragma once

/**
  * @author Andrej Utz
  */

/** @brief Maps a value from a range to another range
 * @param x        Value to be mapped. Must be in input
 * @param in_min   Lower bound of input
 * @param in_max   Higher bound of input
 * @param out_min  Lower bound of output
 * @param out_max  Higher bound of output
 *
 * @tparam I       Type of input range and also give value
 * @tparam O       Type of output range and returned value
 * @tparam F       Intermediate float type for precise mapping between integer types
 *
 * @return Mapped value in between output range
 */
template<class O, class I, class F = float>
inline O map(I x, I in_min, I in_max, O out_min, O out_max)
{
	return O((x - in_min) * F(out_max - out_min) / F(in_max - in_min)) + out_min;
}

/** @brief Maps a value from two ranges to another two ranges depending on center value
 * @param x          Value to be mapped. Must be in input range
 * @param in_min     Lower bound of input
 * @param in_center  In-range center of input
 * @param in_max     Higher bound of input
 * @param out_min    Lower bound of output
 * @param out_center In-range center of output
 * @param out_max    Higher bound of output
 *
 * @tparam I       Type of input range and also give value
 * @tparam O       Type of output range and returned value
 * @tparam F       Intermediate float type for precise mapping between integer types
 *
 * @return Mapped value in between output range
 */
template<class O, class I, class F = float>
inline O map_dual(I x, I in_min, I in_center, I in_max, O out_min, O out_center, O out_max)
{
	if(x < in_center)
		return map<O, I, F>(x, in_min, in_center, out_min, out_center);
	else
		return map<O, I, F>(x, in_center, in_max, out_center, out_max);
}

/**
 * @overload O map_dual(I x, I in_min, I in_center, I in_max, O out_min, O out_center, O out_max)
 */
template<class O, class I, class F = float>
inline O map_dual(I x, I in_min, I in_max, O out_min, O out_max)
{
	return map_dual<O, I, F>(x, in_min, I(0), in_max, out_min, O(0), out_max);
}

/**
 * @brief Clamp an input between to values
 * @param x    Input value
 * @param min  Minimum bound
 * @param max  Maximum bound
 * @return The clamped value
 */
template<class T>
inline T clamp(T x, T min, T max)
{
	if(x < min)	x = min;
	else
	if(x > max) x = max;

	return x;
}

/**
 * @brief Calls fn only when value v has changes after previous call
 * @param v      Changing value
 * @param args   Arguments for the called function
 * @param fn     Called function with call signature (T prev, T v, ...args)
 */
template<class T, class ...Args, class Fn>
inline void on_change(const T& v, Args ...args, Fn fn)
{
	static T prev {};
	if(prev != v)
	{
		fn(prev, v, args...);
		prev = v;
	}
}
