#include "distfscross.h"

inline int get_fs_state(double fastavg, double slowavg)
{
	if (fastavg == 0.0 || slowavg == 0.0) return 0; // initial state or invalid
	if (fastavg > slowavg) return 1; // fast above slow
	if (fastavg < slowavg) return 2; // fast below slow
	return 3; // fast equal slow
}

DistFSCross::DistFSCross(): _prev_close(0.0), _fs_close(0.0), _prev_fs_state(0), fscross_found(false)
{
}

void DistFSCross::initialize(double fs_close, double prev_close, int prev_fs_state)
{
	_fs_close = fs_close;
	_prev_close = prev_close;
	_prev_fs_state = prev_fs_state;
}

bool DistFSCross::is_fscross() const
{
	return fscross_found;
}

double DistFSCross::update(double close, double fastavg, double slowavg)
{
	int curr_state = get_fs_state(fastavg, slowavg);
	double value = 0.0;
	fscross_found = false;

	if (_prev_fs_state != 0)
	{
		if (curr_state != _prev_fs_state)
		{
			_fs_close = _prev_close;
			fscross_found = true;
		}

		// to calculate value, first fs-cross must encountered before
		// value = (current close) - (close before fs-cross)
		if (_fs_close != 0.0)
		{
			value = close - _fs_close;
		}
	}

	_prev_fs_state = curr_state;
	_prev_close = close;

	return value;
}
