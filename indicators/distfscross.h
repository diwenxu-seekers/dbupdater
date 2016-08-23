#ifndef DISTFSCROSS_H
#define DISTFSCROSS_H

class DistFSCross {
	public:
		DistFSCross();

		void initialize(double fs_close, double prev_close, int prev_fs_state);

		double update(double close, double fastavg, double slowavg);

		bool is_fscross() const;

	private:
		double _prev_close;
		double _fs_close;
		int _prev_fs_state;
		bool fscross_found;
};

#endif // DISTFSCROSS_H
