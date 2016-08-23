#include "atr.h"

#include <numeric>

ATR::ATR(int length):_length(length), _first_bar(true)
{
}

void ATR::initialize(const std::list<double> &data, double prev_close)
{
	_tr_buffer = data;
	_prev_close = prev_close;
	_first_bar = false;
}

double ATR::update(double high, double low, double close)
{
	double tr_1 = high - low;
	double tr_2 = 0.0;
	double tr_3 = 0.0;
	double value = 0.0;

	if (_prev_close > 0.0)
	{
		tr_2 = std::fabs(high - _prev_close);
		tr_3 = std::fabs(low - _prev_close);
	}

	_tr_buffer.emplace_back(std::fmax(std::fmax( tr_1 , tr_2 ), tr_3));

	if (_tr_buffer.size() > _length)
	{
		_tr_buffer.pop_front();
	}

	if (_tr_buffer.size() == _length)
	{
		value = std::accumulate(_tr_buffer.begin(), _tr_buffer.end(), 0.0) / _length;
	}
	else
	{
		if (_first_bar)
		{
			value = tr_1;
			_first_bar = false;
		}
		else
		{
			value = (_prev_atr * (_length - 1) + _tr_buffer.back()) / _length;
		}
	}

	_prev_close = close;
	_prev_atr = value;

	return value;
}
