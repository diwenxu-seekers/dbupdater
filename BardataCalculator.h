/** BardataCalculator.h
 * --------------------
 * contain all indicators calculation function here
 */
#ifndef BARDATACALCULATOR_H
#define BARDATACALCULATOR_H

#include "searchbar/globals.h"
#include "bardatatuple.h"
#include "logdefs.h"

#include <QSqlDatabase>
#include <QSqlQuery>

#include <string>
#include <vector>

class BardataCalculator {
	public:
		// update _parent_monthly and _parent_monthly_prev
		static int update_parent_monthly_v2 (const QSqlDatabase &base_database, const QString &database_path, const QString &symbol);

		// indexing parent prev
		static int update_index_parent_prev (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																				 const IntervalWeight parent, int start_rowid);

		// candle normalization and ranking
		static int update_candle_approx_v2 (std::vector<BardataTuple> *out, const QSqlDatabase &database, int start_rowid);

		static int update_zone (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
														const IntervalWeight base_interval, int start_rowid);

		// moving average calculation for all timeframe
		static int update_moving_avg_approx (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																				 const IntervalWeight baseInterval, const IntervalWeight MAInterval, int start_rowid);

		// (since v1.6.6) moving average calculation for all timeframe
		static int update_moving_avg_approx_v2 (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																						const IntervalWeight baseInterval, const IntervalWeight MAInterval, int start_rowid);

		// dist_point, dist_atr in support/resistance
		static int update_sr_distpoint_distatr (const QSqlDatabase &db, int id_threshold, int start_rowid);

		// DistResistance using Max( Min(Resistance) Intersected React )
		// Optimize version by loading necessary data into local variable (Since v1.2.3)
		static int update_dist_resistance_approx_v3 (const QSqlDatabase &db, const QString &database_path, const QString &symbol,
																								 const IntervalWeight base_interval, int id_threshold, int start_rowid, Logger _logger);

		// DistSupport using Min( Max(Support) Intersected React ) (Since v1.2.3)
		static int update_dist_support_approx_v3 (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																							const IntervalWeight base_interval, int id_threshold, int start_rowid, Logger _logger);

		// Resistance Velocity: how many bars it passed since last datetime react
		static int update_resistance_velocity (const QSqlDatabase &database, const IntervalWeight base_interval,
																					 int id_threshold, double reactThreshold, int start_rowid);

		// Support Velocity: how many bars it passed since last datetime react
		static int update_support_velocity (const QSqlDatabase &database, const IntervalWeight base_interval,
																				int id_threshold, double reactThreshold, int start_rowid);

		// calculate how many support/resistance line intersected with current bar
		static int update_sr_line_count (const QSqlDatabase &database, const QString &database_path, const QString &symbol,
																		 const IntervalWeight base_interval, int id_threshold, int start_rowid);

		// dayrange rank approximate
		static int update_dayrange_rank_approx (const QSqlDatabase &database, int start_rowid);

		// include rank for { FGS, FLS, MACD, ATR, DistFS, Dist(FS-Cross) }
		static int update_distOHLC_rank_approx_v2 (std::vector<BardataTuple> *out, const QSqlDatabase &source_database, const QString &database_path,
																							 const QString &symbol, const IntervalWeight base_interval, int start_rowid);

		// revision for DBUpdater V2
		static int update_distOHLC_rank_approx_v2r1
			(std::vector<BardataTuple> *out, const QSqlDatabase &source_database, const QString &database_path,
			 const QString &symbol, const IntervalWeight base_interval, int start_rowid);

		static int update_prevdaily_atr (std::vector<BardataTuple> *out, const QSqlDatabase &database, const QString &database_path, const QString &symbol);

//		static int update_prevdaily_atr_v2 (const QSqlDatabase &database, const QString &database_path, const QString &symbol, int start_rowid);

		/** Calculate CGF, CLF, CGS, CLS with Threshold
		 * Examples:
		 *    if (threshold = 0)        C > F
		 *    if (positive threshold)   C > (F - T)
		 *    if (negative threshold)   C < (F + T)
		 *
		 * Description:
		 *  C = Close
		 *  F = FastAvg
		 *  T = threshold
		 */
		static int update_close_threshold_v3 (std::vector<BardataTuple> *out, const QSqlDatabase &database,
																					int id_threshold, double cgf_threshold, double clf_threshold,
																					double cgs_threshold, double cls_threshold, int start_rowid);

		// Calculate fgs, fls threshold
		//  (F - T) < S
		//  (F + T) > S
		static int update_fgs_fls_threshold_approx (std::vector<BardataTuple> *out, const QSqlDatabase &database,
																								int id_threshold, double fgs_threshold, double fls_threshold, int start_rowid);

		// MACD, RSI, SlowK, SlowD with threshold
		static int update_column_threshold_1 (std::vector<BardataTuple> *out, const QSqlDatabase &database,
																					const QString &column_name, int id_threshold,
																					const QString &operator_1, const QString &value_1,
																					const QString &operator_2, const QString &value_2, int start_rowid);

		// Calculate RSI, SlowK, SlowD rank (since v1.0.4)
		static int update_rsi_slowk_slowd_rank_approx (const QSqlDatabase &database, int start_rowid);


		/** Calculate Rate of Change (ROC) (since v1.0.4)
		 * Rate of Chg = (close / previous_n-th_close ) - 1 * 100;
		 *        n-th = length (input by user, default = 14).
		 */
		static int update_rate_of_change_approx (const QSqlDatabase &database, int length, int start_rowid);

		// calculate currency exchange rate by join with currency table (since v1.0.4)
		static int update_ccy_rate (const QSqlDatabase &database, const QString &ccy_database, int start_rowid);

		static int update_ccy_rate_controller (const QSqlDatabase &database, const QString &database_dir,
																					 const QString &symbol, const IntervalWeight interval, int start_rowid);

		// calculate Dist(FS-Cross) with threshold (since v1.0.4)
		static int update_distfscross_threshold_approx (std::vector<BardataTuple> *out, const QSqlDatabase &database,
																										const IntervalWeight interval, double threshold, int column_id, int start_rowid);

		// @deprecated since v1.6.6- replaced by create_incomplete_bar()
		// calculate incomplete bar: create artificial bar for non-exists bar like holiday case (since v1.0.4)
//    static bool update_incomplete_bar (const QSqlDatabase &database, const QString &database_dir,
//                                       const QString &ccy_database_dir, const QString &symbol,
//                                       const IntervalWeight base_intervals, const IntervalWeight target_interval, Logger _logger);

		// create incomplete bar (since 1.5.8)
		static bool create_incomplete_bar (const QSqlDatabase &database, const QString &database_dir, const QString &input_dir,
																			 const QString &symbol, const IntervalWeight interval,
																			 const QDateTime &lastdatetime, BardataTuple *out, BardataTuple &old, Logger _logger);

		// since 2.0.0
		static bool create_incomplete_bar_v2 (const QSqlDatabase &database, const QString &database_dir, const QString &input_dir,
																					const QString &symbol, const IntervalWeight interval,
																					const QDateTime &lastdatetime, BardataTuple *out, BardataTuple &old, Logger _logger);

		// calculate realtime bar for currency (since v1.2.4)
		static int update_incomplete_bar_ccy (const QString &input_filename, const QString &database_filename);

		static int update_incomplete_bar_ccy_v2(const QString &ccy_name, const QString &input_dir, const QString& database_dir);

		// calculate velocity low (up) and velocity high (down) (since v1.5.0)
		static int update_up_down_mom_approx (std::vector<BardataTuple> *out, const QSqlDatabase &database, const int length, const int start_rowid);

		// calculate (up > down) and (down > up) (since v1.5.0)
		static int update_up_down_threshold_approx (std::vector<BardataTuple> *out, const QSqlDatabase &database, const int length,
																								const int id_threshold, const double threshold, const int start_rowid);
};

#endif // BARDATACALCULATOR_H
