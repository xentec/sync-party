#pragma once

template<class O, class I, class F = float>
O map(I x, I in_min, I in_max, O out_min, O out_max)
{
	return O((x - in_min) * F(out_max - out_min) / F(in_max - in_min)) + out_min;
}

template<class O, class I, class F = float>
O map_dual(I x, I in_min, I in_center, I in_max, O out_min, O out_center, O out_max)
{
	if(x < in_center)
		return map(x, in_min, in_center, out_min, out_center);
	else
		return map(x, in_center, in_max, out_center, out_max);
}

template<class O, class I, class F = float>
O map_dual(I x, I in_min, I in_max, O out_min, O out_max)
{
	return map_dual(x, in_min, I(0), in_max, out_min, O(0), out_max);
}

template<class T, class ...Args, class Fn>
void on_change(const T& v, Args ...args, Fn fn)
{
	static T prev {};
	if(prev != v)
	{
		prev = v;
		fn(v, args...);
	}
}
