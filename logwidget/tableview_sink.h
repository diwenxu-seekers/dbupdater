#ifndef TABLEVIEW_SINK_H
#define TABLEVIEW_SINK_H

#include "spdlog/sinks/base_sink.h"
#include "spdlog/details/null_mutex.h"
#include "logmsgwidget.h"


namespace spdlog
{
namespace sinks
{

template<class Mutex>
class tableview_sink : public base_sink < Mutex >
{
public:
    explicit tableview_sink(LogMsgWidget* widget) : _widget(widget) { }

    void flush() override { }

protected:
    void _sink_it(const details::log_msg& msg) override
    {
        LogMsgModel::LogMsg message = { static_cast<LogMsgModel::LogLevel>(msg.level),
            QDateTime::fromMSecsSinceEpoch(std::chrono::duration_cast<std::chrono::milliseconds>(msg.time.time_since_epoch()).count()),
            QString(msg.raw.c_str())
        };

        _widget->append(message);
    }

private:
    LogMsgWidget* _widget = NULL;
};

typedef tableview_sink<std::mutex> tableview_sink_mt;
typedef tableview_sink<details::null_mutex> tableview_sink_st;

} // namespace sinks
} // namespace spdlog

#endif // TABLEVIEW_SINK_H
