#ifndef STREAMPROCESSOR
#define STREAMPROCESSOR

#include "abstractstream.h"
#include "dayrangeobserver.h"
#include "resistancehighobserver.h"
#include "resistancevelocityobserver.h"
#include "supportlowobserver.h"
#include "supportvelocityobserver.h"

class StreamProcessor {

  public:
    // base stream
    void bind(AbstractStream *in) { mStream = in; }

    // extended stream
    void setDailyStream(AbstractStream *stream) { mDaily = stream; }
    void setResistanceStream(AbstractStream *stream) { mResistance = stream; }
    void setSupportStream(AbstractStream *stream) { mSupport = stream; }
    void setResistanceVelocityStream(AbstractStream *stream) { mResVelocity = stream; }
    void setSupportVelocityStream(AbstractStream *stream) { mSupVelocity = stream; }

    void setReactThreshold(int id_threshold, double react_threshold)
    {
      m_id_threshold = id_threshold;
      mReactThreshold = react_threshold;
    }

    void setIntradayFlag(bool b)
    {
      is_intraday = b;
    }

    // process stream
    void exec()
    {
      if (mStream == NULL) return;

      DayRangeObserver dayRangeObserver;
      ResistanceHighObserver resHighObserver;
      ResistanceVelocityObserver resVelocityObserver;
      SupportLowObserver supLowObserver;
      SupportVelocityObserver supVelocityObserver;

      // bind stream to observer
      dayRangeObserver.bind(mStream, mDaily);
      resHighObserver.bind(mStream, mResistance);
      supLowObserver.bind(mStream, mSupport);

      if (m_id_threshold > -1)
      {
        resVelocityObserver.set_threshold(m_id_threshold, mReactThreshold);
        resVelocityObserver.set_intraday_flag(is_intraday);
        resVelocityObserver.bind(mStream, mResVelocity);
        supVelocityObserver.set_threshold(m_id_threshold, mReactThreshold);
        supVelocityObserver.set_intraday_flag(is_intraday);
        supVelocityObserver.bind(mStream, mSupVelocity);
      }

      // process the stream
      while (!mStream->atEnd())
      {
        if (mDaily != NULL)
        {
          dayRangeObserver.update();
        }

        if (mResistance != NULL)
        {
          resHighObserver.update();
        }

        if (mSupport != NULL)
        {
          supLowObserver.update();
        }

        if (m_id_threshold > -1)
        {
          resVelocityObserver.update();
          supVelocityObserver.update();
        }

        mStream->readLine();
      }

      // optional
      dayRangeObserver.flush();
      resHighObserver.flush();
      resVelocityObserver.flush();
      supLowObserver.flush();
      supVelocityObserver.flush();
    }

  private:
    AbstractStream *mStream = nullptr;
    AbstractStream *mDaily = nullptr;
    AbstractStream *mResistance = nullptr;
    AbstractStream *mSupport = nullptr;
    AbstractStream *mResVelocity = nullptr;
    AbstractStream *mSupVelocity = nullptr;
    double mReactThreshold = -1;
    int m_id_threshold = -1;
    bool is_intraday = false;
};

#endif // STREAMPROCESSOR
