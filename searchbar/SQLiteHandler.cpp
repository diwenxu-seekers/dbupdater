#include "sqlitehandler.h"

QMutex SQLiteHandler::_mutex;
int SQLiteHandler::connection_id = 0;

// Enumeration vars
const QString SQLiteHandler::tempDatabaseName = "tempResult.";
const QString SQLiteHandler::TABLE_NAME_BARDATA = "bardata";
const QString SQLiteHandler::TABLE_NAME_BARDATA_VIEW = "bardataview";
const QString SQLiteHandler::TABLE_NAME_BARDATA_LOOKAHEAD = "bardatalookahead";
const QString SQLiteHandler::TABLE_NAME_RESISTANCE = "resistancedata";
const QString SQLiteHandler::TABLE_NAME_RESISTANCE_DATE = "resistancereactdate";
const QString SQLiteHandler::TABLE_NAME_SUPPORT = "supportdata";
const QString SQLiteHandler::TABLE_NAME_SUPPORT_DATE = "supportreactdate";
const QString SQLiteHandler::COLUMN_NAME_ROWID = "rowid";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT = "_parent";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT_WEEKLY = "_parent_weekly";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT_MONTHLY = "_parent_monthly";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT_PREV = "_parent_prev";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT_PREV_WEEKLY = "_parent_prev_weekly";
const QString SQLiteHandler::COLUMN_NAME_IDPARENT_PREV_MONTHLY = "_parent_prev_monthly";
const QString SQLiteHandler::COLUMN_NAME_IDTHRESHOLD = "id_threshold";
const QString SQLiteHandler::COLUMN_NAME_COMPLETED = "completed";
const QString SQLiteHandler::COLUMN_NAME_DATE = "date_";
const QString SQLiteHandler::COLUMN_NAME_TIME = "time_";
const QString SQLiteHandler::COLUMN_NAME_OPEN = "open_";
const QString SQLiteHandler::COLUMN_NAME_HIGH = "high_";
const QString SQLiteHandler::COLUMN_NAME_LOW = "low_";
const QString SQLiteHandler::COLUMN_NAME_CLOSE = "close_";
const QString SQLiteHandler::COLUMN_NAME_VOLUME = "volume";
const QString SQLiteHandler::COLUMN_NAME_BAR_RANGE = "bar_range";
const QString SQLiteHandler::COLUMN_NAME_INTRADAY_HIGH = "intraday_high";
const QString SQLiteHandler::COLUMN_NAME_INTRADAY_LOW = "intraday_low";
const QString SQLiteHandler::COLUMN_NAME_INTRADAY_RANGE = "intraday_range";
const QString SQLiteHandler::COLUMN_NAME_MACD = "macd";
const QString SQLiteHandler::COLUMN_NAME_MACDAVG = "macdavg";
const QString SQLiteHandler::COLUMN_NAME_MACDDIFF = "macddiff";
const QString SQLiteHandler::COLUMN_NAME_MACD_RANK = "macd_rank";
const QString SQLiteHandler::COLUMN_NAME_MACD_VALUE1 = "macd_value1";
const QString SQLiteHandler::COLUMN_NAME_MACD_VALUE2 = "macd_value2";
const QString SQLiteHandler::COLUMN_NAME_MACD_RANK1 = "macd_rank1";
const QString SQLiteHandler::COLUMN_NAME_MACD_RANK2 = "macd_rank2";
const QString SQLiteHandler::COLUMN_NAME_RSI = "rsi";
const QString SQLiteHandler::COLUMN_NAME_RSI_RANK = "rsi_rank";
const QString SQLiteHandler::COLUMN_NAME_RSI_VALUE1 = "rsi_value1";
const QString SQLiteHandler::COLUMN_NAME_RSI_VALUE2 = "rsi_value2";
const QString SQLiteHandler::COLUMN_NAME_RSI_RANK1 = "rsi_rank1";
const QString SQLiteHandler::COLUMN_NAME_RSI_RANK2 = "rsi_rank2";
const QString SQLiteHandler::COLUMN_NAME_SLOWK = "slowk";
const QString SQLiteHandler::COLUMN_NAME_SLOWK_RANK = "slowk_rank";
const QString SQLiteHandler::COLUMN_NAME_SLOWK_VALUE1 = "slowk_value1";
const QString SQLiteHandler::COLUMN_NAME_SLOWK_VALUE2 = "slowk_value2";
const QString SQLiteHandler::COLUMN_NAME_SLOWK_RANK1 = "slowk_rank1";
const QString SQLiteHandler::COLUMN_NAME_SLOWK_RANK2 = "slowk_rank2";
const QString SQLiteHandler::COLUMN_NAME_SLOWD = "slowd";
const QString SQLiteHandler::COLUMN_NAME_SLOWD_RANK = "slowd_rank";
const QString SQLiteHandler::COLUMN_NAME_SLOWD_VALUE1 = "slowd_value1";
const QString SQLiteHandler::COLUMN_NAME_SLOWD_VALUE2 = "slowd_value2";
const QString SQLiteHandler::COLUMN_NAME_SLOWD_RANK1 = "slowd_rank1";
const QString SQLiteHandler::COLUMN_NAME_SLOWD_RANK2 = "slowd_rank2";
const QString SQLiteHandler::COLUMN_NAME_DISTF = "distf";
const QString SQLiteHandler::COLUMN_NAME_DISTS = "dists";
const QString SQLiteHandler::COLUMN_NAME_DISTFS = "distfs";
const QString SQLiteHandler::COLUMN_NAME_DISTFS_RANK = "distfs_rank";
const QString SQLiteHandler::COLUMN_NAME_N_DISTFS = "n_distfs";
const QString SQLiteHandler::COLUMN_NAME_FASTAVG = "fastavg";
const QString SQLiteHandler::COLUMN_NAME_SLOWAVG = "slowavg";
const QString SQLiteHandler::COLUMN_NAME_DAY10 = "day10";
const QString SQLiteHandler::COLUMN_NAME_DAY50 = "day50";
const QString SQLiteHandler::COLUMN_NAME_WEEK10 = "week10";
const QString SQLiteHandler::COLUMN_NAME_WEEK50 = "week50";
const QString SQLiteHandler::COLUMN_NAME_MONTH10 = "month10";
const QString SQLiteHandler::COLUMN_NAME_MONTH50 = "month50";
const QString SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE = "fastavgslope";
const QString SQLiteHandler::COLUMN_NAME_FASTAVG_SLOPE_RANK = "fastavgslope_rank";
const QString SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE = "slowavgslope";
const QString SQLiteHandler::COLUMN_NAME_SLOWAVG_SLOPE_RANK = "slowavgslope_rank";
const QString SQLiteHandler::COLUMN_NAME_FCROSS = "fcross";
const QString SQLiteHandler::COLUMN_NAME_SCROSS = "scross";
const QString SQLiteHandler::COLUMN_NAME_FGS = "fgs";
const QString SQLiteHandler::COLUMN_NAME_FGS_RANK = "fgs_rank";
const QString SQLiteHandler::COLUMN_NAME_FLS = "fls";
const QString SQLiteHandler::COLUMN_NAME_FLS_RANK = "fls_rank";
const QString SQLiteHandler::COLUMN_NAME_FSCROSS = "fs_cross";
const QString SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS = "distCC_fscross";
const QString SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_ATR = "distcc_fscross_atr";
const QString SQLiteHandler::COLUMN_NAME_DISTCC_FSCROSS_RANK = "distcc_fscross_rank";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL = "candle_uptail";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL = "candle_downtail";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_BODY = "candle_body";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH = "candle_totallength";
const QString SQLiteHandler::COLUMN_NAME_N_UPTAIL = "n_uptail";
const QString SQLiteHandler::COLUMN_NAME_N_DOWNTAIL = "n_downtail";
const QString SQLiteHandler::COLUMN_NAME_N_BODY = "n_body";
const QString SQLiteHandler::COLUMN_NAME_N_TOTALLENGTH = "n_totallength";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_UPTAIL_RANK = "candle_uptail_rank";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_DOWNTAIL_RANK = "candle_downtail_rank";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_BODY_RANK = "candle_body_rank";
const QString SQLiteHandler::COLUMN_NAME_CANDLE_TOTALLENGTH_RANK = "candle_totallength_rank";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_1DAY = "dayrangehigh_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_2DAY = "dayrangehigh_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_3DAY = "dayrangehigh_3day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY = "dayrangehighatr_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY = "dayrangehighatr_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY = "dayrangehighatr_3day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_1DAY = "dayrangehighrank_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_2DAY = "dayrangehighrank_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_RANK_3DAY = "dayrangehighrank_3day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_1DAY = "dayrangelow_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_2DAY = "dayrangelow_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_3DAY = "dayrangelow_3day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY = "dayrangelowatr_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY = "dayrangelowatr_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY = "dayrangelowatr_3day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_1DAY = "dayrangelowrank_1day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_2DAY = "dayrangelowrank_2day";
const QString SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_RANK_3DAY = "dayrangelowrank_3day";
const QString SQLiteHandler::COLUMN_NAME_ATR = "atr";
const QString SQLiteHandler::COLUMN_NAME_ATR_RANK = "atr_rank";
const QString SQLiteHandler::COLUMN_NAME_PREV_DAILY_ATR = "prevdailyatr";
const QString SQLiteHandler::COLUMN_NAME_OPENBAR = "openbar";
const QString SQLiteHandler::COLUMN_NAME_PREV_DATE = "prevdate";
const QString SQLiteHandler::COLUMN_NAME_PREV_TIME = "prevtime";
const QString SQLiteHandler::COLUMN_NAME_PREV_BARCOLOR = "prevbarcolor";
const QString SQLiteHandler::COLUMN_NAME_CGF = "cgf";
const QString SQLiteHandler::COLUMN_NAME_CGF_RANK = "cgf_rank";
const QString SQLiteHandler::COLUMN_NAME_CLF = "clf";
const QString SQLiteHandler::COLUMN_NAME_CLF_RANK = "clf_rank";
const QString SQLiteHandler::COLUMN_NAME_CGS = "cgs";
const QString SQLiteHandler::COLUMN_NAME_CGS_RANK = "cgs_rank";
const QString SQLiteHandler::COLUMN_NAME_CLS = "cls";
const QString SQLiteHandler::COLUMN_NAME_CLS_RANK = "cls_rank";
const QString SQLiteHandler::COLUMN_NAME_HLF = "HLF";
const QString SQLiteHandler::COLUMN_NAME_HLS = "HLS";
const QString SQLiteHandler::COLUMN_NAME_LGF = "LGF";
const QString SQLiteHandler::COLUMN_NAME_LGS = "LGS";
const QString SQLiteHandler::COLUMN_NAME_DISTOF = "distOF";
const QString SQLiteHandler::COLUMN_NAME_DISTOS = "distOS";
const QString SQLiteHandler::COLUMN_NAME_N_DISTOF = "n_distOF";
const QString SQLiteHandler::COLUMN_NAME_N_DISTOS = "n_distOS";
const QString SQLiteHandler::COLUMN_NAME_DISTOF_RANK = "distOF_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTOS_RANK = "distOS_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTHF = "distHF";
const QString SQLiteHandler::COLUMN_NAME_DISTHS = "distHS";
const QString SQLiteHandler::COLUMN_NAME_N_DISTHF = "N_distHF";
const QString SQLiteHandler::COLUMN_NAME_N_DISTHS = "N_distHS";
const QString SQLiteHandler::COLUMN_NAME_DISTHF_RANK = "distHF_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTHS_RANK = "distHS_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTLF = "distLF";
const QString SQLiteHandler::COLUMN_NAME_DISTLS = "distLS";
const QString SQLiteHandler::COLUMN_NAME_N_DISTLF = "n_distLF";
const QString SQLiteHandler::COLUMN_NAME_N_DISTLS = "n_distLS";
const QString SQLiteHandler::COLUMN_NAME_DISTLF_RANK = "distLF_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTLS_RANK = "distLS_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTCF = "distCF";
const QString SQLiteHandler::COLUMN_NAME_DISTCS = "distCS";
const QString SQLiteHandler::COLUMN_NAME_N_DISTCF = "n_distCF";
const QString SQLiteHandler::COLUMN_NAME_N_DISTCS = "n_distCS";
const QString SQLiteHandler::COLUMN_NAME_DISTCF_RANK = "distCF_Rank";
const QString SQLiteHandler::COLUMN_NAME_DISTCS_RANK = "distCS_Rank";
const QString SQLiteHandler::COLUMN_NAME_RES = "res";
const QString SQLiteHandler::COLUMN_NAME_DISTRES = "distres";
const QString SQLiteHandler::COLUMN_NAME_DISTRES_ATR = "distresatr";
const QString SQLiteHandler::COLUMN_NAME_DISTRES_RANK = "distresrank";
const QString SQLiteHandler::COLUMN_NAME_RES_LASTREACTDATE = "reslastreactdate";
const QString SQLiteHandler::COLUMN_NAME_SUP = "sup";
const QString SQLiteHandler::COLUMN_NAME_DISTSUP = "distsup";
const QString SQLiteHandler::COLUMN_NAME_DISTSUP_ATR = "distsupatr";
const QString SQLiteHandler::COLUMN_NAME_DISTSUP_RANK = "distsuprank";
const QString SQLiteHandler::COLUMN_NAME_SUP_LASTREACTDATE = "suplastreactdate";
const QString SQLiteHandler::COLUMN_NAME_DOWNVEL_DISTBAR = "downvel_distbar";
const QString SQLiteHandler::COLUMN_NAME_DOWNVEL_DISTPOINT = "downvel_distpoint";
const QString SQLiteHandler::COLUMN_NAME_DOWNVEL_DISTATR = "downVel_distatr";
const QString SQLiteHandler::COLUMN_NAME_UPVEL_DISTBAR = "upvel_distbar";
const QString SQLiteHandler::COLUMN_NAME_UPVEL_DISTPOINT = "upvel_distpoint";
const QString SQLiteHandler::COLUMN_NAME_UPVEL_DISTATR = "upvel_distatr";
const QString SQLiteHandler::COLUMN_NAME_DAILY_RLINE = "daily_rline";
const QString SQLiteHandler::COLUMN_NAME_DAILY_SLINE = "daily_sline";
const QString SQLiteHandler::COLUMN_NAME_WEEKLY_RLINE = "weekly_rline";
const QString SQLiteHandler::COLUMN_NAME_WEEKLY_SLINE = "weekly_sline";
const QString SQLiteHandler::COLUMN_NAME_MONTHLY_RLINE = "monthly_rline";
const QString SQLiteHandler::COLUMN_NAME_MONTHLY_SLINE = "monthly_sline";
const QString SQLiteHandler::COLUMN_NAME_OPEN_ZONE = "open_zone";
const QString SQLiteHandler::COLUMN_NAME_OPEN_ZONE_60MIN = "open_zone60min";
const QString SQLiteHandler::COLUMN_NAME_OPEN_ZONE_DAILY = "open_zonedaily";
const QString SQLiteHandler::COLUMN_NAME_OPEN_ZONE_WEEKLY = "open_zoneweekly";
const QString SQLiteHandler::COLUMN_NAME_OPEN_ZONE_MONTHLY = "open_zonemonthly";
const QString SQLiteHandler::COLUMN_NAME_HIGH_ZONE = "high_zone";
const QString SQLiteHandler::COLUMN_NAME_HIGH_ZONE_60MIN = "high_zone60min";
const QString SQLiteHandler::COLUMN_NAME_HIGH_ZONE_DAILY = "high_zonedaily";
const QString SQLiteHandler::COLUMN_NAME_HIGH_ZONE_WEEKLY = "high_zoneweekly";
const QString SQLiteHandler::COLUMN_NAME_HIGH_ZONE_MONTHLY = "high_zonemonthly";
const QString SQLiteHandler::COLUMN_NAME_LOW_ZONE = "low_zone";
const QString SQLiteHandler::COLUMN_NAME_LOW_ZONE_60MIN = "low_zone60min";
const QString SQLiteHandler::COLUMN_NAME_LOW_ZONE_DAILY = "low_zonedaily";
const QString SQLiteHandler::COLUMN_NAME_LOW_ZONE_WEEKLY = "low_zoneweekly";
const QString SQLiteHandler::COLUMN_NAME_LOW_ZONE_MONTHLY = "low_zonemonthly";
const QString SQLiteHandler::COLUMN_NAME_CLOSE_ZONE = "close_zone";
const QString SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_60MIN = "close_zone60min";
const QString SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_DAILY = "close_zonedaily";
const QString SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_WEEKLY = "close_zoneweekly";
const QString SQLiteHandler::COLUMN_NAME_CLOSE_ZONE_MONTHLY = "close_zonemonthly";
const QString SQLiteHandler::COLUMN_NAME_REACT_DATE = "react_date";
const QString SQLiteHandler::COLUMN_NAME_REACT_TIME = "react_time";
const QString SQLiteHandler::COLUMN_NAME_RESISTANCE_COUNT = "resistance_count";
const QString SQLiteHandler::COLUMN_NAME_RESISTANCE_DURATION = "resistance_duration";
const QString SQLiteHandler::COLUMN_NAME_RESISTANCE_VALUE = "resistance";
const QString SQLiteHandler::COLUMN_NAME_SUPPORT_COUNT = "support_count";
const QString SQLiteHandler::COLUMN_NAME_SUPPORT_DURATION = "support_duration";
const QString SQLiteHandler::COLUMN_NAME_SUPPORT_VALUE = "support";
const QString SQLiteHandler::COLUMN_NAME_DIST_POINT = "dist_point";
const QString SQLiteHandler::COLUMN_NAME_DIST_ATR = "dist_atr";
const QString SQLiteHandler::COLUMN_NAME_TAGLINE = "tagline";
const QString SQLiteHandler::COLUMN_NAME_HIGHBARS = "highbars";
const QString SQLiteHandler::COLUMN_NAME_LOWBARS = "lowbars";
const QString SQLiteHandler::COLUMN_NAME_CONSEC_T = "consec_t";
const QString SQLiteHandler::COLUMN_NAME_CONSEC_N = "consec_n";
const QString SQLiteHandler::COLUMN_NAME_ROC = "roc";
const QString SQLiteHandler::COLUMN_NAME_ROC_RANK = "roc_rank";
const QString SQLiteHandler::COLUMN_NAME_CCY_RATE = "ccy_rate";
const QString SQLiteHandler::COLUMN_NAME_UP_MOM_10 = "upmom_10";
const QString SQLiteHandler::COLUMN_NAME_UP_MOM_10_RANK = "upmom_10_rank";
const QString SQLiteHandler::COLUMN_NAME_DOWN_MOM_10 = "downmom_10";
const QString SQLiteHandler::COLUMN_NAME_DOWN_MOM_10_RANK = "downmom_10_rank";
const QString SQLiteHandler::COLUMN_NAME_DISTUD_10 = "distud_10";
const QString SQLiteHandler::COLUMN_NAME_DISTUD_10_RANK = "distud_10_rank";
const QString SQLiteHandler::COLUMN_NAME_DISTDU_10 = "distdu_10";
const QString SQLiteHandler::COLUMN_NAME_DISTDU_10_RANK = "distdu_10_rank";
const QString SQLiteHandler::COLUMN_NAME_UGD_10 = "ugd_10";
const QString SQLiteHandler::COLUMN_NAME_UGD_10_RANK = "ugd_10_rank";
const QString SQLiteHandler::COLUMN_NAME_DGU_10 = "dgu_10";
const QString SQLiteHandler::COLUMN_NAME_DGU_10_RANK = "dgu_10_rank";
const QString SQLiteHandler::COLUMN_NAME_UP_MOM_50 = "upmom_50";
const QString SQLiteHandler::COLUMN_NAME_UP_MOM_50_RANK = "upmom_50_rank";
const QString SQLiteHandler::COLUMN_NAME_DOWN_MOM_50 = "downmom_50";
const QString SQLiteHandler::COLUMN_NAME_DOWN_MOM_50_RANK = "downmom_50_rank";
const QString SQLiteHandler::COLUMN_NAME_DISTUD_50 = "distud_50";
const QString SQLiteHandler::COLUMN_NAME_DISTUD_50_RANK = "distud_50_rank";
const QString SQLiteHandler::COLUMN_NAME_DISTDU_50 = "distdu_50";
const QString SQLiteHandler::COLUMN_NAME_DISTDU_50_RANK = "distdu_50_rank";
const QString SQLiteHandler::COLUMN_NAME_UGD_50 = "ugd_50";
const QString SQLiteHandler::COLUMN_NAME_UGD_50_RANK = "ugd_50_rank";
const QString SQLiteHandler::COLUMN_NAME_DGU_50 = "dgu_50";
const QString SQLiteHandler::COLUMN_NAME_DGU_50_RANK = "dgu_50_rank";

/**
 * BarData V2 (2015-01-30)
 * Removed column : OI, ZeroLine, OverBot, OverSld
 * Add column     : DistF, DistS, FGS (F greater than S), FLS (F less than S)
 * Change primary key using (date_, time_)
 *
 * Rev1: 2015-02-01
 * Add column _parent to enable multi resolution search.
 *
 * Rev2: 2015-02-10 (v0.5)
 * Add column resistance_test_point, support_test_point.
 *
 * Rev3: 2015-03-01 (v0.6)
 * Add column for multiple threshold resistance/support test point
 *
 * Rev4: 2015-03-18 - 2015-03-20 (v0.7)
 * Denormalized the table for increase performance
 * Add FastAvg and SlowAvg of the parent
 * Add _parent_prev for store index of previous bar of parent
 * Add DistF/DistS Rank (denormalize)
 *
 * Rev5: 2015-04-19 (v0.8.14)
 * Add DistFS (distance between Fast and Slow)
 *
 * Rev6: 2015-04-21 (v0.8.15)
 * Add FastAvgSlope and SlowAvgSlope, PrevDate, PrevTime
 *
 * Rev7: 2015-04-29 (v0.9.2)
 * Add DistFC, DistFC_Rank, DistSC, DistSC_Rank
 *
 * Rev8: 2015-05-11 (v0.9.4)
 * Add ATR, RSI_GREATER_70, RSI_LESS_30, MACD_GREATER_0, MACD_LESS_0,
 * SLOWK_GREATER_80, SLOWK_LESS_20, SLOWD_GREATER_80, SLOWD_LESS_20
 *
 * Rev9: 2015-05-12 (v0.9.4)
 * Add uptail, downtail, body, totallength, daterange
 *
 * Rev10: 2015-05-19 (v0.9.5)
 * Add DistResistance, DistSupport, Datetime_UB, Datetime_LB
 *
 * Rev11: ??? - 2015-06-14 (v0.10.2)
 * Add Zone, RLine, SLine, Res_1, Sup_1
 * Renamed columns, such as MACD_GREATER_0 into MACD_G0
 *
 * Rev12: ?? - 2015-07-22 (v0.10.7)
 * Add CGF, CLF, CGS, CLF, RSI, SlowK, SlowD threshold column
 * Day10, Day50 realtime value column
 * Zone for OHLC
 *
 * Rev13: 2015-08-04
 * Add Week-10, Week-50, Month-10, Month-50 for realtime calculation
 *
 * Rev14: 2015-08-10
 * Add ResVel, SupVel, ResLastReactDate, SupLastReactDate
 * Remove Zone, ZoneDaily, ZoneWeekly, ZoneMonthly (replaced by OHLC-Zone)
 *
 * Rev15: 2015-08-17 (v0.10.15)
 * Add DayRangeHigh, DayRangeHighATR
 * Add DayRangeLow, DayRangeLowATR
 * Remove CGF, CLF, CGS, CLS (replaced by parameter columns)
 * Remove DateRange (replaced by DayRangeHigh and DayRangeLow)
 *
 * Rev16: 2015-08-27
 * Add DownVel_DistBar, DownVel_DistPoint, DownVel_ATR
 * Add UpVel_DistBar, UpVel_DistPoint, UpVel_ATR
 *
 * Rev17: 2015-10-27
 * Add ROC, ROC_Rank, FastAvgSlope_Rank, SlowAvgSlope_Rank, completed
 *
 * Rev18: 2015-10-29
 * Add rsi_rank, slowk_rank, slowd_rank
 *
 * Rev19: 2016-01-27
 * Add up/down momentum (0.14.4)
 * Remove DistRes, DistResATR, DistResRank
 * Remove DistSup, DistSupATR, DistSupRank
 *
 * Rev20: 2016-03-16 (0.16.4)
 * Add bar_range, intraday_high, intraday_low, intraday_range
 */
const QString SQLiteHandler::SQL_CREATE_TABLE_BARDATA_V2 =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_BARDATA + "("
		"date_	TEXT NOT NULL,"
		"time_	TEXT NOT NULL,"
		"open_	REAL NOT NULL,"
		"high_	REAL NOT NULL,"
		"low_		REAL NOT NULL,"
		"close_	REAL NOT NULL,"
		"bar_range			REAL," // High - Low
		"intraday_high	REAL," // intraday daily high
		"intraday_low	  REAL," // intraday daily low
		"intraday_range	REAL," // intraday High- intraday Low
		"volume			INTEGER,"
		"macd       REAL,"
		"macd_rank  REAL," // for positive (MACD > 0) and negative (MACD < 0)
		"macdavg    REAL,"
		"macddiff   REAL,"
		"rsi        REAL,"
		"rsi_rank   REAL,"
		"slowk      REAL,"
		"slowk_rank REAL,"
		"slowd      REAL,"
		"slowd_rank REAL,"
		"fastavg    REAL,"
		"slowavg    REAL,"
		"distf      REAL,"
		"dists      REAL,"
		"fgs        INTEGER DEFAULT 0,"
		"fgs_rank   REAL,"
		"fls        INTEGER DEFAULT 0,"
		"fls_rank   REAL,"
		"openbar    INTEGER,"
		"completed  INTEGER DEFAULT 1,"
		"_parent              INTEGER DEFAULT 0,"
		"_parent_prev         INTEGER DEFAULT 0,"
		"_parent_monthly      INTEGER DEFAULT 0,"
		"_parent_prev_monthly INTEGER DEFAULT 0,"
		"prevdate     TEXT,"
		"prevtime     TEXT,"
		"prevbarcolor TEXT,"
		"prevdailyatr REAL," // for intra-day: PrevDaily ATR
		"atr          REAL,"
		"atr_rank     REAL,"
		"fastavgslope       REAL,"
		"fastavgslope_rank  REAL,"
		"slowavgslope       REAL,"
		"slowavgslope_rank  REAL,"
		"fcross      INTEGER,"
		"scross      INTEGER,"

		// rate of change
		"roc      REAL,"
		"roc_rank REAL,"

		// currency exchange rate (for @NIY, HHI, HSI)
		"ccy_rate REAL DEFAULT 1.0,"

		// our own calculation for moving average
		"day10    REAL,"
		"day50    REAL,"
		"week10   REAL,"
		"week50   REAL,"
		"month10  REAL,"
		"month50  REAL,"

		"candle_uptail           REAL,"
		"n_uptail                REAL,"
		"candle_uptail_rank      REAL,"
		"candle_downtail         REAL,"
		"n_downtail              REAL,"
		"candle_downtail_rank    REAL,"
		"candle_body             REAL,"
		"n_body                  REAL,"
		"candle_body_rank        REAL,"
		"candle_totallength      REAL,"
		"n_totallength           REAL,"
		"candle_totallength_rank REAL,"

		"dayrangehigh_1day      REAL," // Max(High) - Close
		"dayrangehighatr_1day   REAL," // DayRangeHigh / PrevDaily.ATR
		"dayrangehighrank_1day  REAL,"
		"dayrangelow_1day       REAL," // Min(Low) - Close
		"dayrangelowatr_1day    REAL," // DayRangeLow / PrevDaily.ATR
		"dayrangelowrank_1day   REAL,"
		"dayrangehigh_2day      REAL,"
		"dayrangehighatr_2day   REAL,"
		"dayrangehighrank_2day  REAL,"
		"dayrangelow_2day       REAL,"
		"dayrangelowatr_2day    REAL,"
		"dayrangelowrank_2day   REAL,"
		"dayrangehigh_3day      REAL,"
		"dayrangehighatr_3day   REAL,"
		"dayrangehighrank_3day  REAL,"
		"dayrangelow_3day       REAL,"
		"dayrangelowatr_3day    REAL,"
		"dayrangelowrank_3day   REAL,"

		// Zone for each OHLC
		"open_zone         INTEGER,"
		"open_zone60min    INTEGER,"
		"open_zonedaily    INTEGER,"
		"open_zoneweekly   INTEGER,"
		"open_zonemonthly  INTEGER,"
		"high_zone         INTEGER,"
		"high_zone60min    INTEGER,"
		"high_zonedaily    INTEGER,"
		"high_zoneweekly   INTEGER,"
		"high_zonemonthly  INTEGER,"
		"low_zone          INTEGER,"
		"low_zone60min     INTEGER,"
		"low_zonedaily     INTEGER,"
		"low_zoneweekly    INTEGER,"
		"low_zonemonthly   INTEGER,"
		"close_zone        INTEGER,"
		"close_zone60min   INTEGER,"
		"close_zonedaily   INTEGER,"
		"close_zoneweekly  INTEGER,"
		"close_zonemonthly INTEGER,"

		// tag whether current bar is MA-Cross (1 or 0)
		"fs_cross           INTEGER,"

		// fgs threshold
		"fgs_0        INTEGER DEFAULT 0,"
		"fgs_rank_0   REAL    DEFAULT 0,"
		"fgs_1        INTEGER DEFAULT 0,"
		"fgs_rank_1   REAL    DEFAULT 0,"
		"fgs_2        INTEGER DEFAULT 0,"
		"fgs_rank_2   REAL    DEFAULT 0,"
		"fgs_3        INTEGER DEFAULT 0,"
		"fgs_rank_3   REAL    DEFAULT 0,"
		"fgs_4        INTEGER DEFAULT 0,"
		"fgs_rank_4   REAL    DEFAULT 0,"

		// fls threshold
		"fls_0        INTEGER DEFAULT 0,"
		"fls_rank_0   REAL    DEFAULT 0,"
		"fls_1        INTEGER DEFAULT 0,"
		"fls_rank_1   REAL    DEFAULT 0,"
		"fls_2        INTEGER DEFAULT 0,"
		"fls_rank_2   REAL    DEFAULT 0,"
		"fls_3        INTEGER DEFAULT 0,"
		"fls_rank_3   REAL    DEFAULT 0,"
		"fls_4        INTEGER DEFAULT 0,"
		"fls_rank_4   REAL    DEFAULT 0,"

		// Distance between Close since MA-Cross - Close of current bar
		"distcc_fscross       REAL," // (Close_curr - Close_since_FS_cross)
		"distcc_fscross_atr   REAL," // DistCC_FSCross / ATR
		"distcc_fscross_rank  REAL,"

		"distcc_fscross_0       REAL,"
		"distcc_fscross_atr_0   REAL,"
		"distcc_fscross_rank_0  REAL,"
		"distcc_fscross_1       REAL,"
		"distcc_fscross_atr_1   REAL,"
		"distcc_fscross_rank_1  REAL,"
		"distcc_fscross_2       REAL,"
		"distcc_fscross_atr_2   REAL,"
		"distcc_fscross_rank_2  REAL,"
		"distcc_fscross_3       REAL,"
		"distcc_fscross_atr_3   REAL,"
		"distcc_fscross_rank_3  REAL,"
		"distcc_fscross_4       REAL,"
		"distcc_fscross_atr_4   REAL,"
		"distcc_fscross_rank_4  REAL,"

		"distof       REAL," // (Open - FastAvg)
		"n_distof     REAL,"
		"distof_rank  REAL,"
		"distos       REAL," // (Open - SlowAvg)
		"n_distos     REAL,"
		"distos_rank  REAL,"

		"disthf       REAL," // (High - FastAvg)
		"n_disthf     REAL,"
		"disthf_rank  REAL,"
		"disths       REAL," // (High - SlowAvg)
		"n_disths     REAL,"
		"disths_rank  REAL,"

		"distlf       REAL," // (Low - FastAvg)
		"n_distlf     REAL,"
		"distlf_rank  REAL,"
		"distls       REAL," // (Low - SlowAvg)
		"n_distls     REAL,"
		"distls_rank  REAL,"

		"distcf       REAL," // (Close - FastAvg)
		"n_distcf     REAL,"
		"distcf_rank  REAL,"
		"distcs       REAL," // (Close - SlowAvg)
		"n_distcs     REAL,"
		"distcs_rank  REAL,"

		"distfs       REAL," // (FastAvg - SlowAvg)
		"n_distfs     REAL,"
		"distfs_rank  REAL,"

		// macd threshold
		"macd_value1_1  INTEGER DEFAULT 0," // MACD < 0
		"macd_rank1_1   REAL,"
		"macd_value2_1  INTEGER DEFAULT 0," // MACD > 0
		"macd_rank2_1   REAL,"
		"macd_value1_2  INTEGER DEFAULT 0,"
		"macd_rank1_2   REAL,"
		"macd_value2_2  INTEGER DEFAULT 0,"
		"macd_rank2_2   REAL,"
		"macd_value1_3  INTEGER DEFAULT 0,"
		"macd_rank1_3   REAL,"
		"macd_value2_3  INTEGER DEFAULT 0,"
		"macd_rank2_3   REAL,"

		// rsi threshold
		"rsi_value1_1   INTEGER DEFAULT 0," // RSI < 30
		"rsi_rank1_1    REAL,"
		"rsi_value2_1   INTEGER DEFAULT 0," // RSI > 70
		"rsi_rank2_1    REAL,"
		"rsi_value1_2   INTEGER DEFAULT 0,"
		"rsi_rank1_2    REAL,"
		"rsi_value2_2   INTEGER DEFAULT 0,"
		"rsi_rank2_2    REAL,"
		"rsi_value1_3   INTEGER DEFAULT 0,"
		"rsi_rank1_3    REAL,"
		"rsi_value2_3   INTEGER DEFAULT 0,"
		"rsi_rank2_3    REAL,"

		// slowk threshold
		"slowk_value1_1   INTEGER DEFAULT 0," // SlowK < 20
		"slowk_rank1_1    REAL,"
		"slowk_value2_1   INTEGER DEFAULT 0," // SlowK > 80
		"slowk_rank2_1    REAL,"
		"slowk_value1_2   INTEGER DEFAULT 0,"
		"slowk_rank1_2    REAL,"
		"slowk_value2_2   INTEGER DEFAULT 0,"
		"slowk_rank2_2    REAL,"
		"slowk_value1_3   INTEGER DEFAULT 0,"
		"slowk_rank1_3    REAL,"
		"slowk_value2_3   INTEGER DEFAULT 0,"
		"slowk_rank2_3    REAL,"

		// slowd threshold
		"slowd_value1_1   INTEGER DEFAULT 0," // SlowD < 20
		"slowd_rank1_1    REAL,"
		"slowd_value2_1   INTEGER DEFAULT 0," // SlowD > 80
		"slowd_rank2_1    REAL,"
		"slowd_value1_2   INTEGER DEFAULT 0,"
		"slowd_rank1_2    REAL,"
		"slowd_value2_2   INTEGER DEFAULT 0,"
		"slowd_rank2_2    REAL,"
		"slowd_value1_3   INTEGER DEFAULT 0,"
		"slowd_rank1_3    REAL,"
		"slowd_value2_3   INTEGER DEFAULT 0,"
		"slowd_rank2_3    REAL,"

		"cgf_0        INTEGER DEFAULT 0," // Close > Fast
		"cgf_rank_0   REAL    DEFAULT 0,"
		"clf_0        INTEGER DEFAULT 0," // Close < Fast
		"clf_rank_0   REAL    DEFAULT 0,"
		"cgs_0        INTEGER DEFAULT 0," // Close > Slow
		"cgs_rank_0   REAL    DEFAULT 0,"
		"cls_0        INTEGER DEFAULT 0," // Close < Slow
		"cls_rank_0   REAL    DEFAULT 0,"

		"cgf_1        INTEGER DEFAULT 0,"
		"cgf_rank_1   REAL    DEFAULT 0,"
		"clf_1        INTEGER DEFAULT 0,"
		"clf_rank_1   REAL    DEFAULT 0,"
		"cgs_1        INTEGER DEFAULT 0,"
		"cgs_rank_1   REAL    DEFAULT 0,"
		"cls_1        INTEGER DEFAULT 0,"
		"cls_rank_1   REAL    DEFAULT 0,"

		"cgf_2        INTEGER DEFAULT 0,"
		"cgf_rank_2   REAL    DEFAULT 0,"
		"clf_2        INTEGER DEFAULT 0,"
		"clf_rank_2   REAL    DEFAULT 0,"
		"cgs_2        INTEGER DEFAULT 0,"
		"cgs_rank_2   REAL    DEFAULT 0,"
		"cls_2        INTEGER DEFAULT 0,"
		"cls_rank_2   REAL    DEFAULT 0,"

		"cgf_3        INTEGER DEFAULT 0,"
		"cgf_rank_3   REAL    DEFAULT 0,"
		"clf_3        INTEGER DEFAULT 0,"
		"clf_rank_3   REAL    DEFAULT 0,"
		"cgs_3        INTEGER DEFAULT 0,"
		"cgs_rank_3   REAL    DEFAULT 0,"
		"cls_3        INTEGER DEFAULT 0,"
		"cls_rank_3   REAL    DEFAULT 0,"

		"cgf_4        INTEGER DEFAULT 0,"
		"cgf_rank_4   REAL    DEFAULT 0,"
		"clf_4        INTEGER DEFAULT 0,"
		"clf_rank_4   REAL    DEFAULT 0,"
		"cgs_4        INTEGER DEFAULT 0,"
		"cgs_rank_4   REAL    DEFAULT 0,"
		"cls_4        INTEGER DEFAULT 0,"
		"cls_rank_4   REAL    DEFAULT 0,"

		"hlf INTEGER DEFAULT 0," // #Bars Below Fast (H < F)
		"hls INTEGER DEFAULT 0," // #Bars Below Slow (H < S)
		"lgf INTEGER DEFAULT 0," // #Bars Above Fast (L > F)
		"lgs INTEGER DEFAULT 0," // #Bars Above Slow (L > S)

		"res_1                REAL,"
		"reslastreactdate_1   TEXT,"
		"downvel_distbar_1    INT," // distance (bars)
//    "distres_1            REAL," // Min(Resistance) - Close
//    "distresatr_1         REAL," // DistRes / ATR
//    "distresrank_1        REAL,"
//    "downvel_distpoint_1  REAL," // distance (point): High - Close
//    "downvel_distatr_1    REAL," // distance (point) / previous ATR (realtime ATR)

		"res_2                REAL,"
		"reslastreactdate_2   TEXT,"
		"downvel_distbar_2    INT,"
//    "distres_2            REAL,"
//    "distresatr_2         REAL,"
//    "distresrank_2        REAL,"
//    "downvel_distpoint_2  REAL,"
//    "downvel_distatr_2    REAL,"

		"res_3                REAL,"
		"reslastreactdate_3   TEXT,"
		"downvel_distbar_3    INT,"
//    "distres_3            REAL,"
//    "distresatr_3         REAL,"
//    "distresrank_3        REAL,"
//    "downvel_distpoint_3  REAL,"
//    "downvel_distatr_3    REAL,"

		"res_4                REAL,"
		"reslastreactdate_4   TEXT,"
		"downvel_distbar_4    INT,"
//    "distres_4            REAL,"
//    "distresatr_4         REAL,"
//    "distresrank_4        REAL,"
//    "downvel_distpoint_4  REAL,"
//    "downvel_distatr_4    REAL,"

		"res_5                REAL,"
		"reslastreactdate_5   TEXT,"
		"downvel_distbar_5    INT,"
//    "distres_5            REAL,"
//    "distresatr_5         REAL,"
//    "distresrank_5        REAL,"
//    "downvel_distpoint_5  REAL,"
//    "downvel_distatr_5    REAL,"

		"sup_1                REAL,"
		"suplastreactdate_1   TEXT,"
		"upvel_distbar_1      INT,"
//    "distsup_1            REAL," // Max(Support) - Close
//    "distsupatr_1         REAL," // DistSup / ATR
//    "distsuprank_1        REAL,"
//    "upvel_distpoint_1    REAL,"
//    "upvel_distatr_1      REAL,"

		"sup_2                REAL,"
		"suplastreactdate_2   TEXT,"
		"upvel_distbar_2      INT,"
//    "distsup_2            REAL,"
//    "distsupatr_2         REAL,"
//    "distsuprank_2        REAL,"
//    "upvel_distpoint_2    REAL,"
//    "upvel_distatr_2      REAL,"

		"sup_3                REAL,"
		"suplastreactdate_3   TEXT,"
		"upvel_distbar_3      INT,"
//    "distsup_3            REAL,"
//    "distsupatr_3         REAL,"
//    "distsuprank_3        REAL,"
//    "upvel_distpoint_3    REAL,"
//    "upvel_distatr_3      REAL,"

		"sup_4                REAL,"
		"suplastreactdate_4   TEXT,"
		"upvel_distbar_4      INT,"
//    "distsup_4            REAL,"
//    "distsupatr_4         REAL,"
//    "distsuprank_4        REAL,"
//    "upvel_distpoint_4    REAL,"
//    "upvel_distatr_4      REAL,"

		"sup_5                REAL,"
		"suplastreactdate_5   TEXT,"
		"upvel_distbar_5      INT,"
//    "distsup_5            REAL,"
//    "distsupatr_5         REAL,"
//    "distsuprank_5        REAL,"
//    "upvel_distpoint_5    REAL,"
//    "upvel_distatr_5      REAL,"

		// show how many number of resistance/support line that intersect with current bar

		"daily_rline_1 INTEGER,"
		"daily_sline_1 INTEGER,"
		"daily_rline_2 INTEGER,"
		"daily_sline_2 INTEGER,"
		"daily_rline_3 INTEGER,"
		"daily_sline_3 INTEGER,"
		"daily_rline_4 INTEGER,"
		"daily_sline_4 INTEGER,"
		"daily_rline_5 INTEGER,"
		"daily_sline_5 INTEGER,"

		"weekly_rline_1 INTEGER,"
		"weekly_sline_1 INTEGER,"
		"weekly_rline_2 INTEGER,"
		"weekly_sline_2 INTEGER,"
		"weekly_rline_3 INTEGER,"
		"weekly_sline_3 INTEGER,"
		"weekly_rline_4 INTEGER,"
		"weekly_sline_4 INTEGER,"
		"weekly_rline_5 INTEGER,"
		"weekly_sline_5 INTEGER,"

		"monthly_rline_1 INTEGER,"
		"monthly_sline_1 INTEGER,"
		"monthly_rline_2 INTEGER,"
		"monthly_sline_2 INTEGER,"
		"monthly_rline_3 INTEGER,"
		"monthly_sline_3 INTEGER,"
		"monthly_rline_4 INTEGER,"
		"monthly_sline_4 INTEGER,"
		"monthly_rline_5 INTEGER,"
		"monthly_sline_5 INTEGER,"

		// up momentum & down momentum
		"upmom_10         REAL,"
		"upmom_10_rank    REAL,"
		"downmom_10       REAL,"
		"downmom_10_rank  REAL,"
		"distud_10        REAL,"
		"distud_10_rank   REAL,"
		"distdu_10        REAL,"
		"distdu_10_rank   REAL,"

		"ugd_10_0       INTEGER DEFAULT 0,"
		"ugd_10_rank_0  REAL    DEFAULT 0,"
		"dgu_10_0       INTEGER DEFAULT 0,"
		"dgu_10_rank_0  REAL    DEFAULT 0,"

		"ugd_10_1       INTEGER DEFAULT 0,"
		"ugd_10_rank_1  REAL,"
		"dgu_10_1       INTEGER DEFAULT 0,"
		"dgu_10_rank_1  REAL,"

		"ugd_10_2       INTEGER DEFAULT 0,"
		"ugd_10_rank_2  REAL,"
		"dgu_10_2       INTEGER DEFAULT 0,"
		"dgu_10_rank_2  REAL,"

		"ugd_10_3       INTEGER DEFAULT 0,"
		"ugd_10_rank_3  REAL,"
		"dgu_10_3       INTEGER DEFAULT 0,"
		"dgu_10_rank_3  REAL,"

		"ugd_10_4       INTEGER DEFAULT 0,"
		"ugd_10_rank_4  REAL,"
		"dgu_10_4       INTEGER DEFAULT 0,"
		"dgu_10_rank_4  REAL,"

		"upmom_50         REAL,"
		"upmom_50_rank    REAL,"
		"downmom_50       REAL,"
		"downmom_50_rank  REAL,"
		"distud_50        REAL,"
		"distud_50_rank   REAL,"
		"distdu_50        REAL,"
		"distdu_50_rank   REAL,"

		"ugd_50_0       INTEGER DEFAULT 0,"
		"ugd_50_rank_0  REAL    DEFAULT 0,"
		"dgu_50_0       INTEGER DEFAULT 0,"
		"dgu_50_rank_0  REAL    DEFAULT 0,"

		"ugd_50_1       INTEGER DEFAULT 0,"
		"ugd_50_rank_1  REAL,"
		"dgu_50_1       INTEGER DEFAULT 0,"
		"dgu_50_rank_1  REAL,"

		"ugd_50_2       INTEGER DEFAULT 0,"
		"ugd_50_rank_2  REAL,"
		"dgu_50_2       INTEGER DEFAULT 0,"
		"dgu_50_rank_2  REAL,"

		"ugd_50_3       INTEGER DEFAULT 0,"
		"ugd_50_rank_3  REAL,"
		"dgu_50_3       INTEGER DEFAULT 0,"
		"dgu_50_rank_3  REAL,"

		"ugd_50_4       INTEGER DEFAULT 0,"
		"ugd_50_rank_4  REAL,"
		"dgu_50_4       INTEGER DEFAULT 0,"
		"dgu_50_rank_4  REAL,"

		"PRIMARY KEY(date_ ASC,time_ ASC));";

/**
 * Resistance & Support V1
 * 2015-04-02 : add column duration
 */
const QString SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_V1 =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_RESISTANCE + "("
	"date_ TEXT NOT NULL,"
	"time_ TEXT NOT NULL,"
	"react_date TEXT,"
	"react_time TEXT,"
	"resistance_count INTEGER,"
	"resistance_duration INTEGER,"
	"resistance REAL,"
	"last_duration INTEGER,"
	"first_duration INTEGER,"
	"id_threshold INTEGER,"
	"dist_point REAL,"
	"dist_atr REAL,"
	"tagline INTEGER DEFAULT 0,"
	"tagline2 INTEGER DEFAULT 0,"
	"highbars INTEGER,"
	"consec_t INTEGER,"
	"consec_n INTEGER,"
	"PRIMARY KEY(id_threshold, date_ asc, time_ asc, resistance desc));";

const QString SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_V1 =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_SUPPORT + "("
	"date_ TEXT NOT NULL,"
	"time_ TEXT NOT NULL,"
	"react_date TEXT,"
	"react_time TEXT,"
	"support_count INTEGER,"
	"support_duration INTEGER,"
	"support REAL,"
	"last_duration INTEGER,"
	"first_duration INTEGER,"
	"id_threshold INTEGER,"
	"dist_point REAL,"
	"dist_atr REAL,"
	"tagline INTEGER DEFAULT 0,"
	"tagline2 INTEGER DEFAULT 0,"
	"lowbars INTEGER,"
	"consec_t INTEGER,"
	"consec_n INTEGER,"
	"PRIMARY KEY(id_threshold, date_ asc, time_ asc, support desc));";

const QString SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_DATE_V1 =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_RESISTANCE_DATE + "("
	"rid INTEGER NOT NULL,"
	"react_date TEXT NOT NULL,"
	"PRIMARY KEY(rid asc, react_date desc));";

const QString SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_DATE_V1 =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_SUPPORT_DATE + "("
	"rid INTEGER NOT NULL,"
	"react_date TEXT NOT NULL,"
	"PRIMARY KEY(rid asc, react_date desc));";

/** @deprecated: Resistance & Support Denormalized version
 * 2015-03-20: Created to prestore detail view of value
 */
/*
const QString SQLiteHandler::SQL_CREATE_TABLE_RESISTANCE_TAB =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_RESISTANCE_TAB + "("
	"index_ INTEGER NOT NULL,"
	"date_ TEXT NOT NULL,"
	"time_ TEXT NOT NULL,"
	"column_count INTEGER DEFAULT 0,"
	"v0 REAL,"
	"PRIMARY KEY(index_, date_, time_));";

const QString SQLiteHandler::SQL_CREATE_TABLE_SUPPORT_TAB =
	"CREATE TABLE IF NOT EXISTS " + SQLiteHandler::TABLE_NAME_SUPPORT_TAB + "("
	"index_ INTEGER NOT NULL,"
	"date_ TEXT NOT NULL,"
	"time_ TEXT NOT NULL,"
	"column_count INTEGER DEFAULT 0,"
	"v0 REAL,"
	"PRIMARY KEY(index_, date_, time_));";
*/


//
// create index
//
const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT =
	"CREATE INDEX IF NOT EXISTS _bardata_parent ON bardata(" + COLUMN_NAME_IDPARENT + ");";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV =
	"CREATE INDEX IF NOT EXISTS _bardata_parent_prev ON bardata(" + COLUMN_NAME_IDPARENT_PREV + ");";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_MONTHLY =
	"CREATE INDEX IF NOT EXISTS _bardata_parent_monthly ON bardata(" + COLUMN_NAME_IDPARENT_MONTHLY + ");";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PARENT_PREV_MONTHLY =
	"CREATE INDEX IF NOT EXISTS _bardata_parent_prev_monthly ON bardata(" + COLUMN_NAME_IDPARENT_PREV_MONTHLY + ");";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_PREVDATETIME =
	"CREATE INDEX IF NOT EXISTS _bardata_prevdatetime ON bardata(" + COLUMN_NAME_PREV_DATE + "," + COLUMN_NAME_PREV_TIME + ");";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_ATR =
	"CREATE INDEX IF NOT EXISTS _bardata_atr ON bardata(atr);";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_MACD =
	"CREATE INDEX IF NOT EXISTS _bardata_macd ON bardata(macd);";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_RSI =
	"CREATE INDEX IF NOT EXISTS _bardata_rsi ON bardata(rsi);";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_SLOWK =
	"CREATE INDEX IF NOT EXISTS _bardata_slowk ON bardata(slowk);";

const QString SQLiteHandler::SQL_CREATE_INDEX_BARDATA_SLOWD =
	"CREATE INDEX IF NOT EXISTS _bardata_slowd ON bardata(slowd);";

const QString SQLiteHandler::SQL_CREATE_INDEX_RESISTANCE_RID =
	"CREATE INDEX IF NOT EXISTS _resistance_rid ON resistancereactdate(rid, react_date);";

const QString SQLiteHandler::SQL_CREATE_INDEX_SUPPORT_RID =
	"CREATE INDEX IF NOT EXISTS _support_rid ON supportreactdate(rid, react_date);";

//
// Bardata Function
//
const QString SQLiteHandler::SQL_SELECT_BARDATA_MAX_ROWID = "SELECT MAX(" + COLUMN_NAME_ROWID + ") FROM " + TABLE_NAME_BARDATA;
const QString SQLiteHandler::SQL_SELECT_BARDATA_DESC = "SELECT * FROM " + TABLE_NAME_BARDATA + " ORDER BY date_ DESC, time_ DESC";

QString SQLiteHandler::SQL_LAST_FROM_ROWID (int start_from_rowid)
{
	return "SELECT * FROM " + SQLiteHandler::TABLE_NAME_BARDATA +
				 " WHERE " + SQLiteHandler::COLUMN_NAME_ROWID + ">=" + QString::number(start_from_rowid) +
				 " ORDER BY date_ DESC, time_ DESC";
}


//
// Resistance and Support Function
//
const QString SQLiteHandler::SQL_INSERT_RESISTANCE_V1 =
	"insert or ignore into " + TABLE_NAME_RESISTANCE +
	"(date_, time_, react_date, react_time, resistance_count, resistance,"
	 "resistance_duration, last_duration, first_duration, id_threshold)"
	" values(?,?,?,?,?, ?,?,?,?,?)";

const QString SQLiteHandler::SQL_INSERT_SUPPORT_V1 =
	"insert or ignore into " + TABLE_NAME_SUPPORT +
	"(date_, time_, react_date, react_time, support_count, support,"
	"support_duration, last_duration, first_duration, id_threshold)"
	" values(?,?,?,?,?, ?,?,?,?,?)";


// select bar from d-Date and t-Time backward to previous n-bars
QString SQLiteHandler::SQL_SELECT_BARDATA_LIMIT (const QDate &d, const QTime &t, int limit)
{
	QString date_value = d.toString("yyyy-MM-dd");
	QString time_value = t.toString("HH:mm");
	return
		"SELECT * FROM " + TABLE_NAME_BARDATA +
		" WHERE " +
				COLUMN_NAME_DATE + "<" + date_value + " OR(" +
				COLUMN_NAME_DATE + "=" + date_value + " AND " +
				COLUMN_NAME_TIME + "<=" + time_value +
		")ORDER BY t_date DESC, t_time DESC"
		" LIMIT " + QString::number(limit);
}

QString SQLiteHandler::SQL_ROWCOUNT_WHERE (const QString &tableName, const QString &whereArgs)
{
	QString sql = "SELECT COUNT(1) FROM " + tableName;
	if (!whereArgs.isEmpty()) sql += " WHERE " + whereArgs;
	return sql;
}

QString SQLiteHandler::SQL_SELECT_EXISTS (const QString &tableName, const QString &whereArgs)
{
	return "SELECT 1 FROM " + tableName + " WHERE " + whereArgs + " LIMIT 1;";
}

QString SQLiteHandler::SQL_SELECT_BARDATA_MAXDATETIME (QString whereArgs = "")
{
	if (!whereArgs.isEmpty()) whereArgs = " where " + whereArgs;
	return
		"select " + COLUMN_NAME_DATE + "||' '||" + COLUMN_NAME_TIME +
		" from " + TABLE_NAME_BARDATA + whereArgs +
		" order by " + COLUMN_NAME_DATE + " desc," + COLUMN_NAME_TIME + " desc limit 1;";
}

QString SQLiteHandler::SQL_SELECT_BARDATA_MINDATETIME (QString whereArgs = "")
{
	if (!whereArgs.isEmpty()) whereArgs = " where " + whereArgs;
	return
		"select " + COLUMN_NAME_DATE + "||' '||" + COLUMN_NAME_TIME +
		" from " + TABLE_NAME_BARDATA + whereArgs +
		" order by " + COLUMN_NAME_DATE + " asc," + COLUMN_NAME_TIME + " asc limit 1;";
}

QString SQLiteHandler::SQL_SELECT_DATETIME (const QDate &d, const QTime &t)
{
	return "select * from " + TABLE_NAME_BARDATA + " where " +
					COLUMN_NAME_DATE + "<'" + d.toString("yyyy-MM-dd") + "' or (" +
					COLUMN_NAME_DATE + "='" + d.toString("yyyy-MM-dd") + "' and "+
					COLUMN_NAME_TIME + "<='" + t.toString("HH:mm:ss") + "')"+
					" order by date_ desc, time_ desc";
}

QString SQLiteHandler::SQL_SELECT_BARDATA_BUILDER (const QStringList &projection, const QString &where_args)
{
	QString sql;
	if (projection.isEmpty())
	{
		sql = "SELECT " + COLUMN_NAME_ROWID + ",* FROM " + TABLE_NAME_BARDATA;
	}
	else
	{
		sql = "SELECT " + projection.join(",") + " FROM " + TABLE_NAME_BARDATA;
	}

	if (!where_args.isEmpty())
		sql += " WHERE " + where_args;

	return sql;
}

// TODO: clean this function and decouple dependency
// main function for sql string builder
QString SQLiteHandler::SQL_SELECT_BARDATA_BUILDER_JOIN (const QStringList &projection,
																												const QStringList &database_alias,
																												const QString &where_args,
																												const QStringList &SRjoinAlias,
																												const QVector<bool> &SRjoin,
																												const QString &target_alias, // TODO: remove this args in next version
																												bool count_query,
																												const QVector<int> &pricePrevBar,
																												const QVector<IntervalWeight> &pricePrevBarInterval,
																												const IntervalWeight projectionInterval)
{
	QString sql = "";
	QString resistance_table = TABLE_NAME_RESISTANCE;
	QString support_table = TABLE_NAME_SUPPORT;

	if (count_query)
	{
		sql = "SELECT COUNT(DISTINCT " + target_alias + ".ROWID) FROM ";
	}
	else
	{
		if (projection.isEmpty())
		{
			sql = "SELECT " + COLUMN_NAME_ROWID + ",* FROM ";
		}
		else
		{
			sql = "SELECT " + projection.join(",") + " FROM ";
//      sql = "SELECT " + target_alias + ".ROWID," + projection.join(",") + " FROM ";
		}
	}

	int start_idx = 0;
	sql += TABLE_NAME_BARDATA + " A";

	QString curr_table;
	QString prev_table;
	QString prev_alias = "A";

	for (int i = database_alias.length()-1; i >= 1 ; --i)
	{
//    if (database_alias[start_idx].isEmpty()) {
//      prev_table = "A";
//    } else {
//      prev_table = database_alias[start_idx] + "." + TABLE_NAME_BARDATA;
//    }

		curr_table = database_alias[i] + "." + TABLE_NAME_BARDATA;
		prev_table = prev_alias;

		if (database_alias[i].contains("Daily")) prev_alias = "R1";
		else if (database_alias[i].contains("Weekly")) prev_alias = "R2";
		else if (database_alias[i].contains("Monthly")) prev_alias = "R3";
		else if (database_alias[i].contains("60min")) prev_alias = "R4";
		else if (database_alias[i].contains("5min")) prev_alias = "R5";

		if (curr_table.contains("Monthly")) {
//      sql += " LEFT JOIN " + curr_table + " " + prev_alias + " ON " +
//                             prev_alias + "." + COLUMN_NAME_ROWID + "=" +
//                             "A." + COLUMN_NAME_IDPARENT_MONTHLY;

			sql += " LEFT JOIN " + curr_table + " " + prev_alias + " ON " +
														 prev_alias + ".rowid=" +
						 "(CASE WHEN A." + COLUMN_NAME_IDPARENT_MONTHLY + "=0 THEN "+
												"A." + COLUMN_NAME_IDPARENT_PREV_MONTHLY + " ELSE " +
												"A." + COLUMN_NAME_IDPARENT_MONTHLY + " END)";
		}
		else if (curr_table.contains("Daily") || curr_table.contains("Weekly")) {
			sql += " LEFT JOIN " + curr_table + " " + prev_alias + " ON " +
														 prev_alias + ".rowid=" +
						 "(CASE WHEN " + prev_table + "._parent=0 THEN " +
														 prev_table + "._parent_prev ELSE " +
														 prev_table + "._parent END)";
		}
		else {
			sql += " LEFT JOIN " + curr_table + " " + prev_alias + " ON " +
														 prev_alias + ".rowid =" + prev_table + "._parent" ;
		}

		start_idx = i;
	}

	if (pricePrevBar.size() > 0)
	{
		QString s = "";
		QString secondAlias = "";
		QString databaseAlias = "";

		for (int i = 0; i < pricePrevBar.size(); ++i)
		{
			s = "A" + QString::number(i + 1);

			if (pricePrevBarInterval[i] == projectionInterval)
			{
				secondAlias = "A";
				databaseAlias = "";
			}
			else
			{
				if (pricePrevBarInterval[i] == WEIGHT_DAILY) { secondAlias = "R1"; databaseAlias = "dbDaily."; }
				else if (pricePrevBarInterval[i] == WEIGHT_WEEKLY) { secondAlias = "R2"; databaseAlias = "dbWeekly."; }
				else if (pricePrevBarInterval[i] == WEIGHT_MONTHLY) { secondAlias = "R3"; databaseAlias = "dbMonthly."; }
				else if (pricePrevBarInterval[i] == WEIGHT_60MIN) { secondAlias = "R4"; databaseAlias = "db60min."; }
				else if (pricePrevBarInterval[i] == WEIGHT_5MIN) { secondAlias = "R5"; databaseAlias = "db5min."; }
			}

			sql += " JOIN " + databaseAlias + TABLE_NAME_BARDATA + " " + s + " ON " +
								s + ".rowid=" + secondAlias + ".rowid-" + QString::number(pricePrevBar[i]);
		}
	}

	QString SRAlias = NULL;
	QStringList SAlias;
	QStringList RAlias;
	QString Pre_SRAlias;
	int N = SRjoin.size();
	bool resistance_flag = false;
	bool support_flag = false;

	for (int i = 0; i < N; ++i)
	{
		// multi interval
		if (!SRjoinAlias[i].isEmpty() && !SRjoinAlias[i].startsWith("#"))
		{
			SRAlias = SRjoinAlias[i] + ".";
			if (SRjoinAlias[i] == "dbDaily") Pre_SRAlias = "R1";
			else if (SRjoinAlias[i] == "dbWeekly") Pre_SRAlias = "R2";
			else if (SRjoinAlias[i] == "dbMonthly") Pre_SRAlias = "R3";

			if (SRjoin[i])
			{
				if (!RAlias.contains(SRjoinAlias[i]))
				{
					sql += " LEFT JOIN " + SRAlias + resistance_table + " ON " +
						SRAlias + resistance_table + "." + COLUMN_NAME_DATE + "=" + Pre_SRAlias + "." + COLUMN_NAME_PREV_DATE + " AND " +
						SRAlias + resistance_table + "." + COLUMN_NAME_TIME + "=" + Pre_SRAlias + "." + COLUMN_NAME_PREV_TIME;
					RAlias.push_back(SRjoinAlias[i]);
				}
			}
			else
			{
				if (!SAlias.contains(SRjoinAlias[i]))
				{
					sql += " LEFT JOIN " + SRAlias + support_table + " ON " +
						SRAlias + support_table + "." + COLUMN_NAME_DATE + "=" + Pre_SRAlias + "." + COLUMN_NAME_PREV_DATE + " AND " +
						SRAlias + support_table + "." + COLUMN_NAME_TIME + "=" + Pre_SRAlias + "." + COLUMN_NAME_PREV_TIME;
					SAlias.push_back(SRjoinAlias[i]);
				}
			}
		}
		// single interval
		else
		{
			if (SRjoin[i] && !resistance_flag)
			{
				sql += " LEFT JOIN " + resistance_table + " RT ON " +
							 "RT." + COLUMN_NAME_DATE + "=A." + COLUMN_NAME_PREV_DATE + " AND " +
							 "RT." + COLUMN_NAME_TIME + "=A." + COLUMN_NAME_PREV_TIME;
				resistance_flag = true;
			}
			else if (!support_flag)
			{
				sql += " LEFT JOIN " + support_table + " ST ON " +
							 "ST." + COLUMN_NAME_DATE + "=A." + COLUMN_NAME_PREV_DATE + " AND " +
							 "ST." + COLUMN_NAME_TIME + "=A." + COLUMN_NAME_PREV_TIME;
				support_flag = true;
			}
		}
	}

	if (!where_args.isEmpty())
	{
		sql += " WHERE " + where_args + " AND " +
											 target_alias + "." + SQLiteHandler::COLUMN_NAME_ROWID + " IS NOT NULL";
	}

	if (!count_query)
	{
		sql += " GROUP BY " + target_alias + "." + SQLiteHandler::COLUMN_NAME_ROWID;
	}

	sql += " ORDER BY " + target_alias + "." + SQLiteHandler::COLUMN_NAME_ROWID + " ASC";
	return sql;
}


// @deprecated: Update _parent with rowid of parent table to indexing the resolution
/*QStringList SQLiteHandler::SQL_UPDATE_BARDATA_PARENT_INDEX_V2(const QString &parent_database, const int &weight, const int &start_rowid) {
	QString child_date = TABLE_NAME_BARDATA + "." + COLUMN_NAME_DATE;
	QString child_time = TABLE_NAME_BARDATA + "." + COLUMN_NAME_TIME;
	QString child_start_from_rowid = start_rowid > 0? "ROWID>"+ QString::number(start_rowid)+" AND ":"";
	QString child_parent_equal_to_zero = COLUMN_NAME_IDPARENT + "=0";
	QString child_parent_monthly_equal_to_zero = COLUMN_NAME_IDPARENT_MONTHLY + "=0";
	QString parent_table = parent_database + "." + TABLE_NAME_BARDATA + " B ";
	QString parent_date = "B." + COLUMN_NAME_DATE;
	QString parent_time = "B." + COLUMN_NAME_TIME;
	// Derived column in temporary table
	QString parent_rowid = "B._ROWID";
	QString parent_date_LB = "B.date_LB";
	QString parent_time_LB = "B.time_LB";
	QString parent_dateprev = "B.date_prev";
	QString parent_monthyear = "B.date_MMYYYY";
	QString parent_intersect_prev_day = "B.intersect_prev_day";
	QStringList sql;

	// Weekly, Daily, 60min
	if (weight >= 1440) {
//     1. BASE CONSTRAINT
//        (child_date > parent_date_LB) AND (child_date <= parent_date) AND
//        (child_time <= parent_time)
//     2. PREVIOUS DAY AFTER CLOSING HOUR
//        (child_date = parent_date_LB) AND (child_time > parent_time)
		sql +=
			"UPDATE " + TABLE_NAME_BARDATA +
			" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
			" FROM " + parent_table +
			" WHERE " +
				child_date + "<=" + parent_date + " AND " +
				child_date + ">"  + parent_date_LB + " AND " +
				child_time + "<=" + parent_time + " LIMIT 1)"
			" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
			" AND EXISTS(SELECT 1 FROM " + parent_table +
			" WHERE " +
				child_date + "<=" + parent_date + " AND " +
				child_date + ">"  + parent_date_LB + " AND " +
				child_time + "<=" + parent_time + " LIMIT 1);";

		sql +=
			"UPDATE " + TABLE_NAME_BARDATA +
			" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
			" FROM " + parent_table +
			" WHERE " +
				child_date + "=" + parent_date_LB + " AND " +
				child_time + ">" + parent_time + " LIMIT 1)"
			" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
			" AND EXISTS(SELECT 1 FROM " + parent_table +
			" WHERE " +
				child_date + "=" + parent_date_LB + " AND " +
				child_time + ">" + parent_time + " LIMIT 1);";

		// Weekly
		if (weight >= 43200) {
			// Monthly: check child_MMYYYY = parent_MMYYYY
			sql +=
				"UPDATE " + TABLE_NAME_BARDATA +
				" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
				" FROM " + parent_table +
				" WHERE strftime('%m%Y'," + child_date + ")=" + parent_monthyear + " LIMIT 1)"
				" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
				" AND EXISTS(SELECT 1 FROM " + parent_table +
				" WHERE strftime('%m%Y'," + child_date + ")=" + parent_monthyear + " LIMIT 1);";
		}
	}
	// 5min, 1min
	else {
//     Because one-shot query is slow, we distribute the constraint into 3 query.
//     The query sequence should begin with highest affected rows.
//     Tip: put most tight condition first check.

//     1. BASE CONSTRAINT
//        (child_date = parent_date) AND
//        (child_time > parent_time_LB) AND (child_time <= parent_time)
//     2. PREVIOUS DAY INTERSECT
//        (child_date = parent_prev_date) AND
//        (parent_time_LB > parent_time) AND (child_time > parent_time_LB)
//     3. EQUAL TIME
//        (child_date = parent_date) AND (child_time = parent_time)

		// Equal time
		sql +=
			"UPDATE " + TABLE_NAME_BARDATA +
			" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
			" FROM " + parent_table +
			" WHERE " +
			child_date + "=" + parent_date + " AND " +
			child_time + "=" + parent_time + " LIMIT 1)"
			" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
			" AND EXISTS(SELECT 1 FROM " + parent_table +
			" WHERE " +
			child_date + "=" + parent_date + " AND " +
			child_time + "=" + parent_time + " LIMIT 1);";

		// Base constraint
		sql +=
			"UPDATE " + TABLE_NAME_BARDATA +
			" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
			" FROM " + parent_table +
			" WHERE " +
			child_date + "="  + parent_date + " AND " +
			child_time + ">"  + parent_time_LB + " AND " +
			child_time + "<=" + parent_time + " LIMIT 1)"
			" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
			" AND EXISTS(SELECT 1 FROM " + parent_table +
			" WHERE " +
			child_date + "="  + parent_date + " AND " +
			child_time + ">"  + parent_time_LB + " AND " +
			child_time + "<=" + parent_time + " LIMIT 1);";

		// TODO: improve this query, this is bottleneck for refresh
		// Previous day intersect
//    sql +=
//      "UPDATE " + TABLE_NAME_BARDATA +
//      " SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
//      " FROM " + parent_table +
//      " WHERE " +
//      parent_time_LB + ">" + parent_time + " AND " +
//      child_time     + ">" + parent_time_LB + " AND " +
//      child_date     + "=" + parent_dateprev + " LIMIT 1)"
//      " WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
//      " AND EXISTS(SELECT 1 FROM " + parent_table +
//      " WHERE " +
//      parent_time_LB + ">" + parent_time + " AND " +
//      child_time     + ">" + parent_time_LB + " AND " +
//      child_date     + "=" + parent_dateprev + " LIMIT 1);";

		// Previous day intersect V2
		sql +=
			"UPDATE " + TABLE_NAME_BARDATA +
			" SET "+ COLUMN_NAME_IDPARENT + "=(SELECT " + parent_rowid +
			" FROM " + parent_table +
			" WHERE " +
			parent_intersect_prev_day + "=1 AND " +
			child_date + "=" + parent_dateprev + " AND " +
			child_time + ">" + parent_time_LB + " LIMIT 1)"
			" WHERE " + child_start_from_rowid + child_parent_equal_to_zero +
			" AND EXISTS(SELECT 1 FROM " + parent_table +
			" WHERE " +
			parent_intersect_prev_day + "=1 AND " +
			child_date + "=" + parent_dateprev + " AND " +
			child_time + ">" + parent_time_LB + " LIMIT 1);";
	}

	// _parent_monthly
	sql +=
		"update " + TABLE_NAME_BARDATA +
		" set "+ COLUMN_NAME_IDPARENT_MONTHLY + "="
		"(select rowid from dbMonthly.bardata a " +
		" where strftime('%m%Y'," + child_date + ")=strftime('%m%Y',a.date_) LIMIT 1)"
		" where " + child_start_from_rowid + child_parent_monthly_equal_to_zero + " and" +
		" exists(select 1 from dbMonthly.bardata a " +
		" where strftime('%m%Y'," + child_date + ")=strftime('%m%Y',a.date_) LIMIT 1);";

	return sql;
}*/
