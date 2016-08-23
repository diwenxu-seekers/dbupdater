//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once
//
// base sink templated over a mutex (either dummy or realy)
// concrete implementation should only overrid the _sink_it method.
// all locking is taken care of here so no locking needed by the implementors..
//

#include<string>
#include<mutex>
#include<atomic>
#include "./sink.h"
#include "../formatter.h"
#include "../common.h"
#include "../details/log_msg.h"


namespace spdlog
{
namespace sinks
{
template<class Mutex>
class base_sink:public sink
{
public:
    base_sink():_level(-1) {}
    virtual ~base_sink() = default;

    base_sink(const base_sink&) = delete;
    base_sink& operator=(const base_sink&) = delete;

    void log(const details::log_msg& msg) override
    {

        if (_level.load(std::memory_order_relaxed) > msg.level)
            return;

        {
            std::lock_guard<Mutex> lock(_mutex);
            _sink_it(msg);
        }
    }

    void set_level(level::level_enum log_level) override
    {
        _level.store(log_level);
    }

    level::level_enum get_level() override
    {
        return static_cast<level::level_enum>(_level.load(std::memory_order_relaxed));
    }


protected:
    virtual void _sink_it(const details::log_msg& msg) = 0;
    Mutex _mutex;
    std::atomic<int> _level;
};
}
}
