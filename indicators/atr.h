#ifndef ATR_H
#define ATR_H

#include <list>

class ATR {
	public:
		ATR(int length = 14);

		void initialize(const std::list<double> &data, double prev_close);

		double update(double high, double low, double close);

	private:
		std::list<double> _tr_buffer;
		double _prev_close;
		double _prev_atr;
		int _length;
		bool _first_bar;
};

#endif // ATR_H
