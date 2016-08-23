#ifndef BARDATATUPLE_H
#define BARDATATUPLE_H

#include <map>
#include <string>

class BardataTuple {
	public:
		// map for {indicator -> value}
		mutable std::map<std::string, std::string> _data;

		// basic indicators
		std::string date = "";
		std::string time = "";
		double open = 0.0;
		double high = 0.0;
		double low = 0.0;
		double close = 0.0;
		long int volume = 0;
		double macd = 0.0;
		double macd_avg = 0.0;
		double macd_diff = 0.0;
		double rsi = 0.0;
		double slow_k = 0.0;
		double slow_d = 0.0;
		double fast_avg = 0.0;
		double slow_avg = 0.0;
		double dist_f = 0.0;
		double dist_s = 0.0;
		int fgs = 0;
		int fls = 0;
		int openbar = 0;
		int completed = 2; // default: complete pending
		long int rowid = 0;
		long int _parent = 0;
		int _parent_monthly = 0;
		long int _parent_prev = 0;
		int _parent_prev_monthly = 0;
		std::string prevdate = "";
		std::string prevtime = "";
		std::string prevbarcolor = "";
		double prev_close = 0.0;
		double bar_range = 0.0;
		double intraday_high = 0.0;
		double intraday_low = 0.0;
		double intraday_range = 0.0;

		double atr = 0.0;
		double prevdaily_atr = 0.0;
		double fastavg_slope = 0.0;
		double slowavg_slope = 0.0;
		int f_cross = 0;
		int s_cross = 0;
		int fs_cross = 0;

		// moving avg calculate
		double day_10 = 0.0;
		double day_50 = 0.0;
		double week_10 = 0.0;
		double week_50 = 0.0;
		double month_10 = 0.0;
		double month_50 = 0.0;

		// candle
		double candle_uptail = 0.0;
		double candle_uptail_normal = 0.0;
		double candle_downtail = 0.0;
		double candle_downtail_normal = 0.0;
		double candle_body = 0.0;
		double candle_body_normal = 0.0;
		double candle_totallength = 0.0;
		double candle_totallength_normal = 0.0;

		// OHLC zone
		int open_zone = 0;
		int open_zone_60min = 0;
		int open_zone_daily = 0;
		int open_zone_weekly = 0;
		int open_zone_monthly = 0;
		int high_zone = 0;
		int high_zone_60min = 0;
		int high_zone_daily = 0;
		int high_zone_weekly = 0;
		int high_zone_monthly = 0;
		int low_zone = 0;
		int low_zone_60min = 0;
		int low_zone_daily = 0;
		int low_zone_weekly = 0;
		int low_zone_monthly = 0;
		int close_zone = 0;
		int close_zone_60min = 0;
		int close_zone_daily = 0;
		int close_zone_weekly = 0;
		int close_zone_monthly = 0;

		double dist_cc_fscross = 0.0;
		double dist_cc_fscross_atr = 0.0;
		double dist_fs = 0.0;
		double n_dist_fs = 0.0;

		double dist_of = 0.0;
		double dist_os = 0.0;
		double n_dist_of = 0.0;
		double n_dist_os = 0.0;

		double dist_hf = 0.0;
		double dist_hs = 0.0;
		double n_dist_hf = 0.0;
		double n_dist_hs = 0.0;

		double dist_lf = 0.0;
		double dist_ls = 0.0;
		double n_dist_lf = 0.0;
		double n_dist_ls = 0.0;

		double dist_cf = 0.0;
		double dist_cs = 0.0;
		double n_dist_cf = 0.0;
		double n_dist_cs = 0.0;

		int hlf = 0; // BarsBelow(F)
		int hls = 0; // BarsBelow(S)
		int lgf = 0; // BarsAbove(F)
		int lgs = 0; // BarsAbove(S)

		BardataTuple();

		// getter
		double get_data_double(const std::string &column_name) const;
		int get_data_int(const std::string &column_name) const;
		std::string get_data_string(const std::string &column_name) const;
		std::string get_datetime() const;

		// setter
		void set_data(const std::string &column_name, const std::string &value);

	protected:
		bool is_exists(const std::string &column_name);
};

#endif // BARDATATUPLE_H
