// DayRangeObserver V2 (2016-04-04)
// Revise bug in dayrange where during holiday they mis-align and give incorrect dayrange after holiday date
// current version is not require to join with daily database anymore
// instead, it require prevdailyatr has been updated
// so make sure PrevDailyATR calculated before

#ifndef DAYRANGEOBSERVER
#define DAYRANGEOBSERVER

#include "abstractstream.h"
#include "sqlitehandler.h"

#include <QVariantList>

class DayRangeObserver {

	public:
		DayRangeObserver(AbstractStream *in = nullptr, AbstractStream *parent = nullptr)
		{
			mStream = in;
			mDailyStream = parent;
			max_high_1d = -1;
			max_high_2d = -1;
			max_high_3d = -1;
			min_low_1d = std::numeric_limits<double>::max();
			min_low_2d = min_low_1d;
			min_low_3d = min_low_1d;

			// initialize column projection
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_1DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_1DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_1DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_1DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_2DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_2DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_2DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_2DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_3DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_HIGH_ATR_3DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_3DAY);
			projection.push_back(SQLiteHandler::COLUMN_NAME_DAYRANGE_LOW_ATR_3DAY);
		}

		~DayRangeObserver()
		{
			flush();
		}

		void bind(AbstractStream *base_stream, AbstractStream *daily_stream)
		{
			mStream = base_stream;
			mDailyStream = daily_stream;
		}

		void update()
		{
			if (mStream == NULL || mDailyStream == NULL) return;

			// once -- get daily time only
			if (daily_time == ""  && !mDailyStream->atEnd())
			{
				daily_time = mDailyStream->getTimeString();
			}

			QString base_time = mStream->getTimeString();

			int rowid = mStream->getRowid();
			double _high = mStream->getHigh();
			double _low = mStream->getLow();
			double _close = mStream->getClose();
			double prev_daily_atr = mStream->getPrevDailyATR();

			bool is_openbar = (prev_time <= daily_time &&
				(base_time > daily_time || (prev_date < mStream->getDateString())));

			if (is_openbar)
			{
				// old
//        max_high_3d = std::fmax(std::fmax(max_high_2d, max_high_1d), _high);
//        max_high_2d = std::fmax(max_high_1d, _high);
//        max_high_1d = _high;

//        min_low_3d = std::fmin(std::fmin(min_low_2d, min_low_1d), _low);
//        min_low_2d = std::fmin(min_low_1d, _low);
//        min_low_1d = _low;

				// new
				max_high_3d = max_high_2d;
				max_high_2d = max_high_1d;
				max_high_1d = _high;

				min_low_3d = min_low_2d;
				min_low_2d = min_low_1d;
				min_low_1d = _low;
			}
			else
			{
				// old
//				max_high_1d = std::fmax(max_high_1d, _high);
//				max_high_2d = std::fmax(max_high_1d, max_high_2d);
//				max_high_3d = std::fmax(max_high_2d, max_high_3d);

//				min_low_1d = std::fmin(min_low_1d, _low);
//				min_low_2d = std::fmin(min_low_1d, min_low_2d);
//				min_low_3d = std::fmin(min_low_2d, min_low_3d);

				// new
				max_high_1d = std::fmax(max_high_1d, _high);
				min_low_1d  = std::fmin(min_low_1d, _low);
			}

			double dayrange_h1 = max_high_1d - _close;
			double dayrange_h2 = max_high_2d - _close;
			double dayrange_h3 = max_high_3d - _close;
			double dayrange_l1 = _close - min_low_1d;
			double dayrange_l2 = _close - min_low_2d;
			double dayrange_l3 = _close - min_low_3d;

			if (prev_daily_atr > 0.0)
			{
				QString _dayrange_high_1 = QString::number(dayrange_h1,'f',5);
				QString _dayrange_high_2 = "";
				QString _dayrange_high_3 = "";
				QString _dayrange_high_atr_1 = QString::number(dayrange_h1 / prev_daily_atr,'f',6);
				QString _dayrange_high_atr_2 = "";
				QString _dayrange_high_atr_3 = "";
				QString _dayrange_low_1 = QString::number(dayrange_l1,'f',5);
				QString _dayrange_low_2 = "";
				QString _dayrange_low_3 = "";
				QString _dayrange_low_atr_1 = QString::number(dayrange_l1 / prev_daily_atr,'f',6);
				QString _dayrange_low_atr_2 = "";
				QString _dayrange_low_atr_3 = "";

				if (max_high_2d > -1)
				{
					_dayrange_high_2     = QString::number(dayrange_h2,'f',5);
					_dayrange_low_2      = QString::number(dayrange_l2,'f',5);
					_dayrange_high_atr_2 = QString::number(dayrange_h2 / prev_daily_atr,'f',6);
					_dayrange_low_atr_2  = QString::number(dayrange_l2 / prev_daily_atr,'f',6);
				}

				if (max_high_3d > -1)
				{
					_dayrange_high_3		 = QString::number(dayrange_h3,'f',5);
					_dayrange_low_3			 = QString::number(dayrange_l3,'f',5);
					_dayrange_high_atr_3 = QString::number(dayrange_h3 / prev_daily_atr,'f',6);
					_dayrange_low_atr_3  = QString::number(dayrange_l3 / prev_daily_atr,'f',6);
				}

				if (_dayrange_high_atr_3 != "")
				{
					_rowid += rowid;
					dayrange_high_1     += _dayrange_high_1;
					dayrange_low_1      += _dayrange_low_1;
					dayrange_high_atr_1 += _dayrange_high_atr_1;
					dayrange_low_atr_1  += _dayrange_low_atr_1;

					dayrange_high_2			+= _dayrange_high_2;
					dayrange_low_2			+= _dayrange_low_2;
					dayrange_high_atr_2 += _dayrange_high_atr_2;
					dayrange_low_atr_2  += _dayrange_low_atr_2;

					dayrange_high_3 += _dayrange_high_3;
					dayrange_low_3 += _dayrange_low_3;
					dayrange_high_atr_3 += _dayrange_high_atr_3;
					dayrange_low_atr_3 += _dayrange_low_atr_3;
				}
			}

			prev_time = base_time;
			prev_date = mStream->getDateString();

			if (_rowid.size() >= 20000) saveList();
		}

		void flush()
		{
			if (!_rowid.empty()) saveList();
		}

	private:
		AbstractStream *mStream = nullptr;
		AbstractStream *mDailyStream = nullptr;
		QVariantList _rowid;
		QVariantList dayrange_high_1, dayrange_high_atr_1;
		QVariantList dayrange_high_2, dayrange_high_atr_2;
		QVariantList dayrange_high_3, dayrange_high_atr_3;
		QVariantList dayrange_low_1, dayrange_low_atr_1;
		QVariantList dayrange_low_2, dayrange_low_atr_2;
		QVariantList dayrange_low_3, dayrange_low_atr_3;
		QStringList projection;
		QString daily_time = "";
		QString prev_date = "";
		QString prev_time = "";
		double max_high_1d, min_low_1d;
		double max_high_2d, min_low_2d;
		double max_high_3d, min_low_3d;

		void clearList()
		{
			_rowid.clear();
			dayrange_high_1.clear();
			dayrange_high_2.clear();
			dayrange_high_3.clear();
			dayrange_high_atr_1.clear();
			dayrange_high_atr_2.clear();
			dayrange_high_atr_3.clear();
			dayrange_low_1.clear();
			dayrange_low_2.clear();
			dayrange_low_3.clear();
			dayrange_low_atr_1.clear();
			dayrange_low_atr_2.clear();
			dayrange_low_atr_3.clear();
		}

		void saveList()
		{
			mStream->save(
				projection, projection.size(),
				dayrange_high_1,
				dayrange_high_atr_1,
				dayrange_low_1,
				dayrange_low_atr_1,
				dayrange_high_2,
				dayrange_high_atr_2,
				dayrange_low_2,
				dayrange_low_atr_2,
				dayrange_high_3,
				dayrange_high_atr_3,
				dayrange_low_3,
				dayrange_low_atr_3,
				_rowid);

			clearList();
		}
};

#endif // DAYRANGEOBSERVER
