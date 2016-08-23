//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#ifdef __linux__

#include <array>
#include <string>
#include <syslog.h>

#include "./base_sink.h"
#include "../common.h"
#include "../details/log_msg.h"
#include "../details/null_mutex.h"


namespace spdlog
{
namespace sinks
{
/**
 * Sink that write to syslog using the `syscall()` library call.
 *
 * Locking is not needed, as `syslog()` itself is thread-safe.
 */
class syslog_sink : public base_sink<details::null_mutex>
{
public:
    //
    syslog_sink(const std::string& ident = "", int syslog_option=0, int syslog_facility=LOG_USER):
        _ident(ident)
    {
        _priorities[static_cast<int>(level::trace)] = LOG_DEBUG;
        _priorities[static_cast<int>(level::debug)] = LOG_DEBUG;
        _priorities[static_cast<int>(level::info)] = LOG_INFO;
        _priorities[static_cast<int>(level::notice)] = LOG_NOTICE;
        _priorities[static_cast<int>(level::warn)] = LOG_WARNING;
        _priorities[static_cast<int>(level::err)] = LOG_ERR;
        _priorities[static_cast<int>(level::critical)] = LOG_CRIT;
        _priorities[static_cast<int>(level::alert)] = LOG_ALERT;
        _priorities[static_cast<int>(level::emerg)] = LOG_EMERG;
        _priorities[static_cast<int>(level::off)] = LOG_INFO;

        //set ident to be program name if empty
        ::openlog(_ident.empty()? nullptr:_ident.c_str(), syslog_option, syslog_facility);
    }
    ~syslog_sink()
    {
        ::closelog();
    }

    syslog_sink(const syslog_sink&) = delete;
    syslog_sink& operator=(const syslog_sink&) = delete;


    void flush() override
    {
    }

protected:
    virtual void _sink_it(const details::log_msg& msg) override
    {
        ::syslog(syslog_prio_from_level(msg), "%s", msg.formatted.str().c_str());
    }


private:
    std::array<int, 10> _priorities;
    //must store the ident because the man says openlog might use the pointer as is and not a string copy
    const std::string _ident;

    //
    // Simply maps spdlog's log level to syslog priority level.
    //
    int syslog_prio_from_level(const details::log_msg &msg) const
    {
        return _priorities[static_cast<int>(msg.level)];
    }
};
}
}

#endif
