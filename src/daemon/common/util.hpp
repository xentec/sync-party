#pragma once

template<class O, class I, class F = float>
O map(I x, I in_min, I in_max, O out_min, O out_max)
{
	return O((x - in_min) * F(out_max - out_min) / F(in_max - in_min)) + out_min;
}

template<class T, class Fn>
void on_change(const T& v, Fn fn)
{
	static T prev {};
	if(prev != v)
	{
		prev = v;
		fn(v);
	}
}
