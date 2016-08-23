#ifndef BARDATADEFINITION
#define BARDATADEFINITION

#include <string>

namespace supportresistance {
	static const std::string TABLE_NAME_RESISTANCE = "resistancedata";
	static const std::string TABLE_NAME_RESISTANCE_DATE = "resistancereactdate";
	static const std::string TABLE_NAME_SUPPORT = "supportdata";
	static const std::string TABLE_NAME_SUPPORT_DATE = "supportreactdate";
	static const std::string COLUMN_NAME_REACT_DATE = "react_date";
	static const std::string COLUMN_NAME_REACT_TIME = "react_time";
	static const std::string COLUMN_NAME_RESISTANCE_COUNT = "resistance_count";
	static const std::string COLUMN_NAME_RESISTANCE_DURATION = "resistance_duration";
	static const std::string COLUMN_NAME_RESISTANCE_VALUE = "resistance";
	static const std::string COLUMN_NAME_SUPPORT_COUNT = "support_count";
	static const std::string COLUMN_NAME_SUPPORT_DURATION = "support_duration";
	static const std::string COLUMN_NAME_SUPPORT_VALUE = "support";
	static const std::string COLUMN_NAME_DIST_POINT = "dist_point";
	static const std::string COLUMN_NAME_DIST_ATR = "dist_atr";
	static const std::string COLUMN_NAME_TAGLINE = "tagline";
	static const std::string COLUMN_NAME_HIGHBARS = "highbars";
	static const std::string COLUMN_NAME_LOWBARS = "lowbars";
	static const std::string COLUMN_NAME_CONSEC_T = "consec_t";
	static const std::string COLUMN_NAME_CONSEC_N = "consec_n";
}

namespace bardata {
	static const std::string TABLE_NAME_BARDATA = "bardata";
	static const std::string TABLE_NAME_BARDATA_VIEW = "bardataview";
	static const std::string TABLE_NAME_BARDATA_LOOKAHEAD = "bardatalookahead";
	static const std::string COLUMN_NAME_ROWID = "rowid";
	static const std::string COLUMN_NAME_IDPARENT = "_parent";
	static const std::string COLUMN_NAME_IDPARENT_WEEKLY = "_parent_weekly";
	static const std::string COLUMN_NAME_IDPARENT_MONTHLY = "_parent_monthly";
	static const std::string COLUMN_NAME_IDPARENT_PREV = "_parent_prev";
	static const std::string COLUMN_NAME_IDPARENT_PREV_WEEKLY = "_parent_prev_weekly";
	static const std::string COLUMN_NAME_IDPARENT_PREV_MONTHLY = "_parent_prev_monthly";
	static const std::string COLUMN_NAME_IDTHRESHOLD = "id_threshold";
	static const std::string COLUMN_NAME_COMPLETED = "completed";
	static const std::string COLUMN_NAME_DATE = "date_";
	static const std::string COLUMN_NAME_TIME = "time_";
	static const std::string COLUMN_NAME_OPEN = "open_";
	static const std::string COLUMN_NAME_HIGH = "high_";
	static const std::string COLUMN_NAME_LOW = "low_";
	static const std::string COLUMN_NAME_CLOSE = "close_";
	static const std::string COLUMN_NAME_BAR_RANGE = "bar_range";
	static const std::string COLUMN_NAME_INTRADAY_HIGH = "intraday_high";
	static const std::string COLUMN_NAME_INTRADAY_LOW = "intraday_low";
	static const std::string COLUMN_NAME_INTRADAY_RANGE = "intraday_range";
	static const std::string COLUMN_NAME_VOLUME = "volume";
	static const std::string COLUMN_NAME_MACD = "macd";
	static const std::string COLUMN_NAME_MACDAVG = "macdavg";
	static const std::string COLUMN_NAME_MACDDIFF = "macddiff";
	static const std::string COLUMN_NAME_RSI = "rsi";
	static const std::string COLUMN_NAME_RSI_RANK = "rsi_rank";
	static const std::string COLUMN_NAME_SLOWK = "slowk";
	static const std::string COLUMN_NAME_SLOWK_RANK = "slowk_rank";
	static const std::string COLUMN_NAME_SLOWD = "slowd";
	static const std::string COLUMN_NAME_SLOWD_RANK = "slowd_rank";
	static const std::string COLUMN_NAME_DISTF = "distf";
	static const std::string COLUMN_NAME_DISTS = "dists";
	static const std::string COLUMN_NAME_DISTFS = "distfs";
	static const std::string COLUMN_NAME_DISTFS_RANK = "distfs_rank";
	static const std::string COLUMN_NAME_N_DISTFS = "n_distfs";
	static const std::string COLUMN_NAME_FASTAVG = "fastavg";
	static const std::string COLUMN_NAME_SLOWAVG = "slowavg";
	static const std::string COLUMN_NAME_DAY10 = "day10";
	static const std::string COLUMN_NAME_DAY50 = "day50";
	static const std::string COLUMN_NAME_WEEK10 = "week10";
	static const std::string COLUMN_NAME_WEEK50 = "week50";
	static const std::string COLUMN_NAME_MONTH10 = "month10";
	static const std::string COLUMN_NAME_MONTH50 = "month50";
	static const std::string COLUMN_NAME_FASTAVG_SLOPE = "fastavgslope";
	static const std::string COLUMN_NAME_FASTAVG_SLOPE_RANK = "fastavgslope_rank";
	static const std::string COLUMN_NAME_SLOWAVG_SLOPE = "slowavgslope";
	static const std::string COLUMN_NAME_SLOWAVG_SLOPE_RANK = "slowavgslope_rank";
	static const std::string COLUMN_NAME_FCROSS = "fcross";
	static const std::string COLUMN_NAME_SCROSS = "scross";
	static const std::string COLUMN_NAME_FGS = "fgs";
	static const std::string COLUMN_NAME_FLS = "fls";
	static const std::string COLUMN_NAME_FGS_RANK = "fgs_rank";
	static const std::string COLUMN_NAME_FLS_RANK = "fls_rank";
	static const std::string COLUMN_NAME_FSCROSS = "fs_cross";
	static const std::string COLUMN_NAME_DISTCC_FSCROSS = "distcc_fscross";
	static const std::string COLUMN_NAME_DISTCC_FSCROSS_ATR = "distcc_fscross_atr";
	static const std::string COLUMN_NAME_DISTCC_FSCROSS_RANK = "distcc_fscross_rank";
	static const std::string COLUMN_NAME_CANDLE_UPTAIL = "candle_uptail";
	static const std::string COLUMN_NAME_CANDLE_DOWNTAIL = "candle_downtail";
	static const std::string COLUMN_NAME_CANDLE_BODY = "candle_body";
	static const std::string COLUMN_NAME_CANDLE_TOTALLENGTH = "candle_totallength";
	static const std::string COLUMN_NAME_N_UPTAIL = "n_uptail";
	static const std::string COLUMN_NAME_N_DOWNTAIL = "n_downtail";
	static const std::string COLUMN_NAME_N_BODY = "n_body";
	static const std::string COLUMN_NAME_N_TOTALLENGTH = "n_totallength";
	static const std::string COLUMN_NAME_CANDLE_UPTAIL_RANK = "candle_uptail_rank";
	static const std::string COLUMN_NAME_CANDLE_DOWNTAIL_RANK = "candle_downtail_rank";
	static const std::string COLUMN_NAME_CANDLE_BODY_RANK = "candle_body_rank";
	static const std::string COLUMN_NAME_CANDLE_TOTALLENGTH_RANK = "candle_totallength_rank";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_1DAY = "dayrangehigh_1day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_2DAY = "dayrangehigh_2day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_3DAY = "dayrangehigh_3day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY = "DayRangeHighATR_1Day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY = "DayRangeHighATR_2Day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY = "DayRangeHighATR_3Day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY = "DayRangeHighRank_1Day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY = "DayRangeHighRank_2Day";
	static const std::string COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY = "DayRangeHighRank_3Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_1DAY = "DayRangeLow_1Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_2DAY = "DayRangeLow_2Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_3DAY = "DayRangeLow_3Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY = "DayRangeLowATR_1Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY = "DayRangeLowATR_2Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY = "DayRangeLowATR_3Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY = "DayRangeLowRank_1Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY = "DayRangeLowRank_2Day";
	static const std::string COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY = "DayRangeLowRank_3Day";
	static const std::string COLUMN_NAME_ATR = "atr";
	static const std::string COLUMN_NAME_ATR_RANK = "atr_rank";
	static const std::string COLUMN_NAME_PREV_DAILY_ATR = "prevdailyatr";
	static const std::string COLUMN_NAME_OPENBAR = "openbar";
	static const std::string COLUMN_NAME_PREV_DATE = "prevdate";
	static const std::string COLUMN_NAME_PREV_TIME = "prevtime";
	static const std::string COLUMN_NAME_PREV_BARCOLOR = "prevbarcolor";
	static const std::string COLUMN_NAME_MACD_RANK = "macd_rank";
	static const std::string COLUMN_NAME_MACD_VALUE1 = "macd_value1";
	static const std::string COLUMN_NAME_MACD_VALUE2 = "macd_value2";
	static const std::string COLUMN_NAME_MACD_RANK1 = "macd_rank1";
	static const std::string COLUMN_NAME_MACD_RANK2 = "macd_rank2";
	static const std::string COLUMN_NAME_RSI_VALUE1 = "rsi_value1";
	static const std::string COLUMN_NAME_RSI_VALUE2 = "rsi_value2";
	static const std::string COLUMN_NAME_RSI_RANK1 = "rsi_rank1";
	static const std::string COLUMN_NAME_RSI_RANK2 = "rsi_rank2";
	static const std::string COLUMN_NAME_SLOWK_VALUE1 = "slowk_value1";
	static const std::string COLUMN_NAME_SLOWK_VALUE2 = "slowk_value2";
	static const std::string COLUMN_NAME_SLOWK_RANK1 = "slowk_rank1";
	static const std::string COLUMN_NAME_SLOWK_RANK2 = "slowk_rank2";
	static const std::string COLUMN_NAME_SLOWD_VALUE1 = "slowd_value1";
	static const std::string COLUMN_NAME_SLOWD_VALUE2 = "slowd_value2";
	static const std::string COLUMN_NAME_SLOWD_RANK1 = "slowd_rank1";
	static const std::string COLUMN_NAME_SLOWD_RANK2 = "slowd_rank2";
	static const std::string COLUMN_NAME_CGF = "cgf";
	static const std::string COLUMN_NAME_CGF_RANK = "cgf_rank";
	static const std::string COLUMN_NAME_CLF = "clf";
	static const std::string COLUMN_NAME_CLF_RANK = "clf_rank";
	static const std::string COLUMN_NAME_CGS = "cgs";
	static const std::string COLUMN_NAME_CGS_RANK = "cgs_rank";
	static const std::string COLUMN_NAME_CLS = "cls";
	static const std::string COLUMN_NAME_CLS_RANK = "cls_rank";
	static const std::string COLUMN_NAME_HLF = "hlf";
	static const std::string COLUMN_NAME_HLS = "hls";
	static const std::string COLUMN_NAME_LGF = "lgf";
	static const std::string COLUMN_NAME_LGS = "lgs";
	static const std::string COLUMN_NAME_DISTOF = "distof";
	static const std::string COLUMN_NAME_DISTOS = "distos";
	static const std::string COLUMN_NAME_N_DISTOF = "n_distof";
	static const std::string COLUMN_NAME_N_DISTOS = "n_distos";
	static const std::string COLUMN_NAME_DISTOF_RANK = "distof_rank";
	static const std::string COLUMN_NAME_DISTOS_RANK = "distos_rank";
	static const std::string COLUMN_NAME_DISTHF = "disthf";
	static const std::string COLUMN_NAME_DISTHS = "disths";
	static const std::string COLUMN_NAME_N_DISTHF = "n_disthf";
	static const std::string COLUMN_NAME_N_DISTHS = "n_disths";
	static const std::string COLUMN_NAME_DISTHF_RANK = "disthf_rank";
	static const std::string COLUMN_NAME_DISTHS_RANK = "disths_rank";
	static const std::string COLUMN_NAME_DISTLF = "distlf";
	static const std::string COLUMN_NAME_DISTLS = "distls";
	static const std::string COLUMN_NAME_N_DISTLF = "n_distlf";
	static const std::string COLUMN_NAME_N_DISTLS = "n_distls";
	static const std::string COLUMN_NAME_DISTLF_RANK = "distlf_rank";
	static const std::string COLUMN_NAME_DISTLS_RANK = "distls_rank";
	static const std::string COLUMN_NAME_DISTCF = "distcf";
	static const std::string COLUMN_NAME_DISTCS = "distcs";
	static const std::string COLUMN_NAME_N_DISTCF = "n_distcf";
	static const std::string COLUMN_NAME_N_DISTCS = "n_distcs";
	static const std::string COLUMN_NAME_DISTCF_RANK = "distcf_rank";
	static const std::string COLUMN_NAME_DISTCS_RANK = "distcs_rank";
	static const std::string COLUMN_NAME_RES = "res"; // XX
	static const std::string COLUMN_NAME_DISTRES = "distres";
	static const std::string COLUMN_NAME_DISTRES_ATR = "distresatr";
	static const std::string COLUMN_NAME_DISTRES_RANK = "distresrank";
	static const std::string COLUMN_NAME_RES_LASTREACTDATE = "resLastreactdate";
	static const std::string COLUMN_NAME_SUP = "sup";
	static const std::string COLUMN_NAME_DISTSUP = "distsup";
	static const std::string COLUMN_NAME_DISTSUP_ATR = "distsupatr";
	static const std::string COLUMN_NAME_DISTSUP_RANK = "distsuprank";
	static const std::string COLUMN_NAME_SUP_LASTREACTDATE = "suplastreactdate";
	static const std::string COLUMN_NAME_DOWNVEL_DISTBAR = "downvel_distbar";
	static const std::string COLUMN_NAME_DOWNVEL_DISTPOINT = "downvel_distpoint";
	static const std::string COLUMN_NAME_DOWNVEL_DISTATR = "downvel_distatr";
	static const std::string COLUMN_NAME_UPVEL_DISTBAR = "upvel_distbar";
	static const std::string COLUMN_NAME_UPVEL_DISTPOINT = "upvel_distpoint";
	static const std::string COLUMN_NAME_UPVEL_DISTATR = "upvel_distatr";
	static const std::string COLUMN_NAME_DAILY_RLINE = "daily_rline";
	static const std::string COLUMN_NAME_DAILY_SLINE = "daily_sline";
	static const std::string COLUMN_NAME_WEEKLY_RLINE = "weekly_rline";
	static const std::string COLUMN_NAME_WEEKLY_SLINE = "weekly_sline";
	static const std::string COLUMN_NAME_MONTHLY_RLINE = "monthly_rline";
	static const std::string COLUMN_NAME_MONTHLY_SLINE = "monthly_sline";
	static const std::string COLUMN_NAME_OPEN_ZONE = "open_zone";
	static const std::string COLUMN_NAME_OPEN_ZONE_60MIN = "open_zone60min";
	static const std::string COLUMN_NAME_OPEN_ZONE_DAILY = "open_zonedaily";
	static const std::string COLUMN_NAME_OPEN_ZONE_WEEKLY = "open_zoneweekly";
	static const std::string COLUMN_NAME_OPEN_ZONE_MONTHLY = "open_zonemonthly";
	static const std::string COLUMN_NAME_HIGH_ZONE = "high_Zone";
	static const std::string COLUMN_NAME_HIGH_ZONE_60MIN = "high_zone60min";
	static const std::string COLUMN_NAME_HIGH_ZONE_DAILY = "high_zonedaily";
	static const std::string COLUMN_NAME_HIGH_ZONE_WEEKLY = "high_zoneweekly";
	static const std::string COLUMN_NAME_HIGH_ZONE_MONTHLY = "high_zonemonthly";
	static const std::string COLUMN_NAME_LOW_ZONE = "low_Zone";
	static const std::string COLUMN_NAME_LOW_ZONE_60MIN = "low_zone60min";
	static const std::string COLUMN_NAME_LOW_ZONE_DAILY = "low_zonedaily";
	static const std::string COLUMN_NAME_LOW_ZONE_WEEKLY = "low_zoneweekly";
	static const std::string COLUMN_NAME_LOW_ZONE_MONTHLY = "low_zonemonthly";
	static const std::string COLUMN_NAME_CLOSE_ZONE = "close_zone";
	static const std::string COLUMN_NAME_CLOSE_ZONE_60MIN = "close_zone60min";
	static const std::string COLUMN_NAME_CLOSE_ZONE_DAILY = "close_zonedaily";
	static const std::string COLUMN_NAME_CLOSE_ZONE_WEEKLY = "close_zoneweekly";
	static const std::string COLUMN_NAME_CLOSE_ZONE_MONTHLY = "close_zonemonthly";
	static const std::string COLUMN_NAME_ROC = "roc";
	static const std::string COLUMN_NAME_ROC_RANK = "roc_rank";
	static const std::string COLUMN_NAME_CCY_RATE = "ccy_rate";
	static const std::string COLUMN_NAME_UP_MOM_10 = "upmom_10";
	static const std::string COLUMN_NAME_UP_MOM_10_RANK = "upmom_10_rank";
	static const std::string COLUMN_NAME_DOWN_MOM_10 = "downmom_10";
	static const std::string COLUMN_NAME_DOWN_MOM_10_RANK = "downmom_10_rank";
	static const std::string COLUMN_NAME_DISTUD_10 = "distud_10";
	static const std::string COLUMN_NAME_DISTUD_10_RANK = "distud_10_rank";
	static const std::string COLUMN_NAME_DISTDU_10 = "distdu_10";
	static const std::string COLUMN_NAME_DISTDU_10_RANK = "distdu_10_rank";
	static const std::string COLUMN_NAME_UGD_10 = "ugd_10";
	static const std::string COLUMN_NAME_UGD_10_RANK = "ugd_10_rank";
	static const std::string COLUMN_NAME_DGU_10 = "dgu_10";
	static const std::string COLUMN_NAME_DGU_10_RANK = "dgu_10_rank";
	static const std::string COLUMN_NAME_UP_MOM_50 = "upmom_50";
	static const std::string COLUMN_NAME_UP_MOM_50_RANK = "upmom_50_rank";
	static const std::string COLUMN_NAME_DOWN_MOM_50 = "downmom_50";
	static const std::string COLUMN_NAME_DOWN_MOM_50_RANK = "downmom_50_rank";
	static const std::string COLUMN_NAME_DISTUD_50 = "distud_50";
	static const std::string COLUMN_NAME_DISTUD_50_RANK = "distud_50_rank";
	static const std::string COLUMN_NAME_DISTDU_50 = "distdu_50";
	static const std::string COLUMN_NAME_DISTDU_50_RANK = "distdu_50_rank";
	static const std::string COLUMN_NAME_UGD_50 = "ugd_50";
	static const std::string COLUMN_NAME_UGD_50_RANK = "ugd_50_rank";
	static const std::string COLUMN_NAME_DGU_50 = "dgu_50";
	static const std::string COLUMN_NAME_DGU_50_RANK = "dgu_50_rank";

	static const char* begin_transaction =
		"PRAGMA journal_mode = OFF;"
		"PRAGMA synchronous = OFF;"
		"PRAGMA cache_size = 50000;"
		"BEGIN TRANSACTION;";

	static const char* end_transaction = "COMMIT;";

	static const char* create_extended_full_index =
		"CREATE INDEX IF NOT EXISTS _bardata_parent_prev_monthly ON bardata(_parent_prev_monthly);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross ON bardata(fs_cross, distfs);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd ON bardata(macd);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi ON bardata(rsi);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk ON bardata(slowk);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd ON bardata(slowd);"
		"CREATE INDEX IF NOT EXISTS _bardata_atr ON bardata(atr);"
		"CREATE INDEX IF NOT EXISTS _bardata_roc ON bardata(roc);"
		"CREATE INDEX IF NOT EXISTS _bardata_completed ON bardata(completed);"
		"CREATE INDEX IF NOT EXISTS _bardata_fslope ON bardata(fastavgslope);"
		"CREATE INDEX IF NOT EXISTS _bardata_sslope ON bardata(slowavgslope);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs ON bardata(fgs);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls ON bardata(fls);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs0 ON bardata(fgs_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls0 ON bardata(fls_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs1 ON bardata(fgs_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls1 ON bardata(fls_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs2 ON bardata(fgs_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls2 ON bardata(fls_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs3 ON bardata(fgs_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls3 ON bardata(fls_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs4 ON bardata(fgs_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_fls4 ON bardata(fls_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_uptail ON bardata(candle_uptail);"
		"CREATE INDEX IF NOT EXISTS _bardata_downtail ON bardata(candle_downtail);"
		"CREATE INDEX IF NOT EXISTS _bardata_body ON bardata(candle_body);"
		"CREATE INDEX IF NOT EXISTS _bardata_totallength ON bardata(candle_totallength);"
		"CREATE INDEX IF NOT EXISTS _bardata_distfs ON bardata(n_distfs);"
		"CREATE INDEX IF NOT EXISTS _bardata_distof ON bardata(n_distof);"
		"CREATE INDEX IF NOT EXISTS _bardata_distos ON bardata(n_distos);"
		"CREATE INDEX IF NOT EXISTS _bardata_disthf ON bardata(n_disthf);"
		"CREATE INDEX IF NOT EXISTS _bardata_disths ON bardata(n_disths);"
		"CREATE INDEX IF NOT EXISTS _bardata_distlf ON bardata(n_distlf);"
		"CREATE INDEX IF NOT EXISTS _bardata_distls ON bardata(n_distls);"
		"CREATE INDEX IF NOT EXISTS _bardata_distcf ON bardata(n_distcf);"
		"CREATE INDEX IF NOT EXISTS _bardata_distcs ON bardata(n_distcs);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh1 ON bardata(dayrangehighatr_1day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh2 ON bardata(dayrangehighatr_2day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh3 ON bardata(dayrangehighatr_3day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel1 ON bardata(dayrangelowatr_1day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel2 ON bardata(dayrangelowatr_2day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel3 ON bardata(dayrangelowatr_3day);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf0 ON bardata(cgf_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_clf0 ON bardata(clf_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs0 ON bardata(cgs_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_cls0 ON bardata(cls_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf1 ON bardata(cgf_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_clf1 ON bardata(clf_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs1 ON bardata(cgs_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_cls1 ON bardata(cls_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf2 ON bardata(cgf_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_clf2 ON bardata(clf_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs2 ON bardata(cgs_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_cls2 ON bardata(cls_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf3 ON bardata(cgf_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_clf3 ON bardata(clf_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs3 ON bardata(cgs_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_cls3 ON bardata(cls_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf2 ON bardata(cgf_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_clf2 ON bardata(clf_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs2 ON bardata(cgs_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_cls2 ON bardata(cls_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v11 ON bardata(macd_value1_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v21 ON bardata(macd_value2_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v12 ON bardata(macd_value1_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v22 ON bardata(macd_value2_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v13 ON bardata(macd_value1_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v23 ON bardata(macd_value2_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v11 ON bardata(rsi_value1_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v21 ON bardata(rsi_value2_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v12 ON bardata(rsi_value1_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v22 ON bardata(rsi_value2_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v13 ON bardata(rsi_value1_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v23 ON bardata(rsi_value2_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v11 ON bardata(slowk_value1_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v21 ON bardata(slowk_value2_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v12 ON bardata(slowk_value1_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v22 ON bardata(slowk_value2_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v13 ON bardata(slowk_value1_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v23 ON bardata(slowk_value2_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v11 ON bardata(slowd_value1_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v21 ON bardata(slowd_value2_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v12 ON bardata(slowd_value1_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v22 ON bardata(slowd_value2_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v13 ON bardata(slowd_value1_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v23 ON bardata(slowd_value2_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr ON bardata(distcc_fscross_atr);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr0 ON bardata(distcc_fscross_atr_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr1 ON bardata(distcc_fscross_atr_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr2 ON bardata(distcc_fscross_atr_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr3 ON bardata(distcc_fscross_atr_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr4 ON bardata(distcc_fscross_atr_4);"
		"CREATE INDEX IF NOT EXISTS _bardata_upmom_10 ON bardata(upmom_10);"
		"CREATE INDEX IF NOT EXISTS _bardata_downmom_10 ON bardata(downmom_10);"
		"CREATE INDEX IF NOT EXISTS _bardata_distud_10 ON bardata(distud_10);"
		"CREATE INDEX IF NOT EXISTS _bardata_distdu_10 ON bardata(distdu_10);"
		"CREATE INDEX IF NOT EXISTS _bardata_ugd_10_0 ON bardata(ugd_10_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_dgu_10_0 ON bardata(dgu_10_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_upmom_50 ON bardata(upmom_50);"
		"CREATE INDEX IF NOT EXISTS _bardata_downmom_50 ON bardata(downmom_50);"
		"CREATE INDEX IF NOT EXISTS _bardata_distud_50 ON bardata(distud_50);"
		"CREATE INDEX IF NOT EXISTS _bardata_distdu_50 ON bardata(distdu_50);"
		"CREATE INDEX IF NOT EXISTS _bardata_ugd_50_0 ON bardata(ugd_50_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_dgu_50_0 ON bardata(dgu_50_0);"
	;

	// @experimental
	/*static const char* create_extended_partial_index =
		"CREATE INDEX IF NOT EXISTS _bardata_macd ON bardata(macd, macd_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi ON bardata(rsi, rsi_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk ON bardata(slowk, slowk_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd ON bardata(slowd, slowd_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_atr ON bardata(atr, atr_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_roc ON bardata(roc);"
		"CREATE INDEX IF NOT EXISTS _bardata_completed ON bardata(completed);"
		"CREATE INDEX IF NOT EXISTS _bardata_fslope ON bardata(fastavgslope);"
		"CREATE INDEX IF NOT EXISTS _bardata_sslope ON bardata(slowavgslope);"
		"CREATE INDEX IF NOT EXISTS _bardata_fgs ON bardata(fgs) where fgs > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_fls ON bardata(fls) where fls > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_uptail ON bardata(candle_uptail, n_uptail, candle_uptail_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_downtail ON bardata(candle_downtail, n_downtail, candle_downtail_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_body ON bardata(candle_body, n_body, candle_body_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_totallength ON bardata(candle_totallength, n_totallength, candle_totallength_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distfs ON bardata(n_distfs, distfs_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distof ON bardata(n_distof, distof_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distos ON bardata(n_distos, distos_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_disthf ON bardata(n_disthf, disthf_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_disths ON bardata(n_disths, disths_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distlf ON bardata(n_distlf, distlf_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distls ON bardata(n_distls, distls_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distcf ON bardata(n_distcf, distcf_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_distcs ON bardata(n_distcs, distcs_rank);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh1 ON bardata(dayrangehighatr_1day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh2 ON bardata(dayrangehighatr_2day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangeh3 ON bardata(dayrangehighatr_3day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel1 ON bardata(dayrangelowatr_1day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel2 ON bardata(dayrangelowatr_2day);"
		"CREATE INDEX IF NOT EXISTS _bardata_drangel3 ON bardata(dayrangelowatr_3day);"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf0 ON bardata(cgf_0) where cgf_0 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_clf0 ON bardata(clf_0) where clf_0 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs0 ON bardata(cgs_0) where cgs_0 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cls0 ON bardata(cls_0) where cls_0 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf1 ON bardata(cgf_1) where cgf_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_clf1 ON bardata(clf_1) where clf_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs1 ON bardata(cgs_1) where cgs_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cls1 ON bardata(cls_1) where cls_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf2 ON bardata(cgf_2) where cgf_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_clf2 ON bardata(clf_2) where clf_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs2 ON bardata(cgs_2) where cgs_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cls2 ON bardata(cls_2) where cls_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf3 ON bardata(cgf_3) where cgf_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_clf3 ON bardata(clf_3) where clf_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs3 ON bardata(cgs_3) where cgs_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cls3 ON bardata(cls_3) where cls_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgf2 ON bardata(cgf_4) where cgf_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_clf2 ON bardata(clf_4) where clf_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cgs2 ON bardata(cgs_4) where cgs_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_cls2 ON bardata(cls_4) where cls_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v11 ON bardata(macd_value1_1) where macd_value1_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v21 ON bardata(macd_value2_1) where macd_value2_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v12 ON bardata(macd_value1_2) where macd_value1_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v22 ON bardata(macd_value2_2) where macd_value2_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v13 ON bardata(macd_value1_3) where macd_value1_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_macd_v23 ON bardata(macd_value2_3) where macd_value2_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v11 ON bardata(rsi_value1_1) where rsi_value1_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v21 ON bardata(rsi_value2_1) where rsi_value2_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v12 ON bardata(rsi_value1_2) where rsi_value1_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v22 ON bardata(rsi_value2_2) where rsi_value2_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v13 ON bardata(rsi_value1_3) where rsi_value1_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_rsi_v23 ON bardata(rsi_value2_3) where rsi_value2_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v11 ON bardata(slowk_value1_1) where slowk_value1_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v21 ON bardata(slowk_value2_1) where slowk_value2_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v12 ON bardata(slowk_value1_2) where slowk_value1_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v22 ON bardata(slowk_value2_2) where slowk_value2_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v13 ON bardata(slowk_value1_3) where slowk_value1_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowk_v23 ON bardata(slowk_value2_3) where slowk_value2_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v11 ON bardata(slowd_value1_1) where slowd_value1_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v21 ON bardata(slowd_value2_1) where slowd_value2_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v12 ON bardata(slowd_value1_2) where slowd_value1_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v22 ON bardata(slowd_value2_2) where slowd_value2_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v13 ON bardata(slowd_value1_3) where slowd_value1_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_slowd_v23 ON bardata(slowd_value2_3) where slowd_value2_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_resatr1 ON bardata(distresatr_1) where distresatr_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_resatr2 ON bardata(distresatr_2) where distresatr_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_resatr3 ON bardata(distresatr_3) where distresatr_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_resatr4 ON bardata(distresatr_4) where distresatr_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_resatr5 ON bardata(distresatr_5) where distresatr_5 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_supatr1 ON bardata(distsupatr_1) where distsupatr_1 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_supatr2 ON bardata(distsupatr_2) where distsupatr_2 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_supatr3 ON bardata(distsupatr_3) where distsupatr_3 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_supatr4 ON bardata(distsupatr_4) where distsupatr_4 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_supatr5 ON bardata(distsupatr_5) where distsupatr_5 > 0;"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr ON bardata(distcc_fscross_atr);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr0 ON bardata(distcc_fscross_atr_0);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr1 ON bardata(distcc_fscross_atr_1);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr2 ON bardata(distcc_fscross_atr_2);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr3 ON bardata(distcc_fscross_atr_3);"
		"CREATE INDEX IF NOT EXISTS _bardata_fscross_atr4 ON bardata(distcc_fscross_atr_4);"
		"CREATE INDEX IF NOT EXISTS _resistance_count ON resistancedata(date_, resistance, id_threshold, resistance_count) where resistance_count >= 3;"
		"CREATE INDEX IF NOT EXISTS _resistance_reactdate ON resistancereactdate(rid, react_date);"
		"CREATE INDEX IF NOT EXISTS _support_count ON supportdata(date_, support, id_threshold, support_count) where support_count >= 3;"
		"CREATE INDEX IF NOT EXISTS _support_reactdate ON supportreactdate(rid, react_date);";*/

	static const char* drop_extended_index =
		"DROP INDEX IF EXISTS _bardata_fscross;"
		"DROP INDEX IF EXISTS _bardata_macd;"
		"DROP INDEX IF EXISTS _bardata_rsi;"
		"DROP INDEX IF EXISTS _bardata_slowk;"
		"DROP INDEX IF EXISTS _bardata_slowd;"
		"DROP INDEX IF EXISTS _bardata_atr;"
		"DROP INDEX IF EXISTS _bardata_roc;"
		"DROP INDEX IF EXISTS _bardata_completed;"
		"DROP INDEX IF EXISTS _bardata_fslope;"
		"DROP INDEX IF EXISTS _bardata_sslope;"
		"DROP INDEX IF EXISTS _bardata_fgs;"
		"DROP INDEX IF EXISTS _bardata_fls;"
		"DROP INDEX IF EXISTS _bardata_fgs0;"
		"DROP INDEX IF EXISTS _bardata_fls0;"
		"DROP INDEX IF EXISTS _bardata_fgs1;"
		"DROP INDEX IF EXISTS _bardata_fls1;"
		"DROP INDEX IF EXISTS _bardata_fgs2;"
		"DROP INDEX IF EXISTS _bardata_fls2;"
		"DROP INDEX IF EXISTS _bardata_fgs3;"
		"DROP INDEX IF EXISTS _bardata_fls3;"
		"DROP INDEX IF EXISTS _bardata_fgs4;"
		"DROP INDEX IF EXISTS _bardata_fls4;"
		"DROP INDEX IF EXISTS _bardata_uptail;"
		"DROP INDEX IF EXISTS _bardata_downtail;"
		"DROP INDEX IF EXISTS _bardata_body;"
		"DROP INDEX IF EXISTS _bardata_totallength;"
		"DROP INDEX IF EXISTS _bardata_distfs;"
		"DROP INDEX IF EXISTS _bardata_distof;"
		"DROP INDEX IF EXISTS _bardata_distos;"
		"DROP INDEX IF EXISTS _bardata_disthf;"
		"DROP INDEX IF EXISTS _bardata_disths;"
		"DROP INDEX IF EXISTS _bardata_distlf;"
		"DROP INDEX IF EXISTS _bardata_distls;"
		"DROP INDEX IF EXISTS _bardata_distcf;"
		"DROP INDEX IF EXISTS _bardata_distcs;"
		"DROP INDEX IF EXISTS _bardata_drangeh1;"
		"DROP INDEX IF EXISTS _bardata_drangeh2;"
		"DROP INDEX IF EXISTS _bardata_drangeh3;"
		"DROP INDEX IF EXISTS _bardata_drangel1;"
		"DROP INDEX IF EXISTS _bardata_drangel2;"
		"DROP INDEX IF EXISTS _bardata_drangel3;"
		"DROP INDEX IF EXISTS _bardata_cgf0;"
		"DROP INDEX IF EXISTS _bardata_clf0;"
		"DROP INDEX IF EXISTS _bardata_cgs0;"
		"DROP INDEX IF EXISTS _bardata_cls0;"
		"DROP INDEX IF EXISTS _bardata_cgf1;"
		"DROP INDEX IF EXISTS _bardata_clf1;"
		"DROP INDEX IF EXISTS _bardata_cgs1;"
		"DROP INDEX IF EXISTS _bardata_cls1;"
		"DROP INDEX IF EXISTS _bardata_cgf2;"
		"DROP INDEX IF EXISTS _bardata_clf2;"
		"DROP INDEX IF EXISTS _bardata_cgs2;"
		"DROP INDEX IF EXISTS _bardata_cls2;"
		"DROP INDEX IF EXISTS _bardata_cgf3;"
		"DROP INDEX IF EXISTS _bardata_clf3;"
		"DROP INDEX IF EXISTS _bardata_cgs3;"
		"DROP INDEX IF EXISTS _bardata_cls3;"
		"DROP INDEX IF EXISTS _bardata_cgf2;"
		"DROP INDEX IF EXISTS _bardata_clf2;"
		"DROP INDEX IF EXISTS _bardata_cgs2;"
		"DROP INDEX IF EXISTS _bardata_cls2;"
		"DROP INDEX IF EXISTS _bardata_macd_v11;"
		"DROP INDEX IF EXISTS _bardata_macd_v21;"
		"DROP INDEX IF EXISTS _bardata_macd_v12;"
		"DROP INDEX IF EXISTS _bardata_macd_v22;"
		"DROP INDEX IF EXISTS _bardata_macd_v13;"
		"DROP INDEX IF EXISTS _bardata_macd_v23;"
		"DROP INDEX IF EXISTS _bardata_rsi_v11;"
		"DROP INDEX IF EXISTS _bardata_rsi_v21;"
		"DROP INDEX IF EXISTS _bardata_rsi_v12;"
		"DROP INDEX IF EXISTS _bardata_rsi_v22;"
		"DROP INDEX IF EXISTS _bardata_rsi_v13;"
		"DROP INDEX IF EXISTS _bardata_rsi_v23;"
		"DROP INDEX IF EXISTS _bardata_slowk_v11;"
		"DROP INDEX IF EXISTS _bardata_slowk_v21;"
		"DROP INDEX IF EXISTS _bardata_slowk_v12;"
		"DROP INDEX IF EXISTS _bardata_slowk_v22;"
		"DROP INDEX IF EXISTS _bardata_slowk_v13;"
		"DROP INDEX IF EXISTS _bardata_slowk_v23;"
		"DROP INDEX IF EXISTS _bardata_slowd_v11;"
		"DROP INDEX IF EXISTS _bardata_slowd_v21;"
		"DROP INDEX IF EXISTS _bardata_slowd_v12;"
		"DROP INDEX IF EXISTS _bardata_slowd_v22;"
		"DROP INDEX IF EXISTS _bardata_slowd_v13;"
		"DROP INDEX IF EXISTS _bardata_slowd_v23;"
		"DROP INDEX IF EXISTS _bardata_resatr1;"
		"DROP INDEX IF EXISTS _bardata_resatr2;"
		"DROP INDEX IF EXISTS _bardata_resatr3;"
		"DROP INDEX IF EXISTS _bardata_resatr4;"
		"DROP INDEX IF EXISTS _bardata_resatr5;"
		"DROP INDEX IF EXISTS _bardata_supatr1;"
		"DROP INDEX IF EXISTS _bardata_supatr2;"
		"DROP INDEX IF EXISTS _bardata_supatr3;"
		"DROP INDEX IF EXISTS _bardata_supatr4;"
		"DROP INDEX IF EXISTS _bardata_supatr5;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr0;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr1;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr2;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr3;"
		"DROP INDEX IF EXISTS _bardata_fscross_atr4;"
		"DROP INDEX IF EXISTS _resistance_count;"
		"DROP INDEX IF EXISTS _resistance_reactdate;"
		"DROP INDEX IF EXISTS _support_count;"
		"DROP INDEX IF EXISTS _support_reactdate;"
		"DROP INDEX IF EXISTS _bardata_upmom_10;"
		"DROP INDEX IF EXISTS _bardata_downmom_10;"
		"DROP INDEX IF EXISTS _bardata_distud_10;"
		"DROP INDEX IF EXISTS _bardata_distdu_10;"
		"DROP INDEX IF EXISTS _bardata_ugd_10_0;"
		"DROP INDEX IF EXISTS _bardata_dgu_10_0;"
		"DROP INDEX IF EXISTS _bardata_upmom_50;"
		"DROP INDEX IF EXISTS _bardata_downmom_50;"
		"DROP INDEX IF EXISTS _bardata_distud_50;"
		"DROP INDEX IF EXISTS _bardata_distdu_50;"
		"DROP INDEX IF EXISTS _bardata_ugd_50_0;"
		"DROP INDEX IF EXISTS _bardata_dgu_50_0;";
}

#endif // BARDATADEFINITION
