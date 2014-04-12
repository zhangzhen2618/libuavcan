/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#pragma once

#include <uavcan/error.hpp>
#include <uavcan/linked_list.hpp>
#include <uavcan/transport/dispatcher.hpp>

namespace uavcan
{

class UAVCAN_EXPORT Scheduler;

class UAVCAN_EXPORT DeadlineHandler : public LinkedListNode<DeadlineHandler>, Noncopyable
{
    MonotonicTime deadline_;

protected:
    Scheduler& scheduler_;

    explicit DeadlineHandler(Scheduler& scheduler)
        : scheduler_(scheduler)
    { }

    virtual ~DeadlineHandler() { stop(); }

public:
    virtual void handleDeadline(MonotonicTime current) = 0;

    void startWithDeadline(MonotonicTime deadline);
    void startWithDelay(MonotonicDuration delay);

    void stop();

    bool isRunning() const;

    MonotonicTime getDeadline() const { return deadline_; }
    Scheduler& getScheduler() const { return scheduler_; }
};


class UAVCAN_EXPORT DeadlineScheduler : Noncopyable
{
    LinkedListRoot<DeadlineHandler> handlers_;  // Ordered by deadline, lowest first

public:
    void add(DeadlineHandler* mdh);
    void remove(DeadlineHandler* mdh);
    bool doesExist(const DeadlineHandler* mdh) const;
    unsigned getNumHandlers() const { return handlers_.getLength(); }

    MonotonicTime pollAndGetMonotonicTime(ISystemClock& sysclock);
    MonotonicTime getEarliestDeadline() const;
};


class UAVCAN_EXPORT Scheduler : Noncopyable
{
    enum { DefaultDeadlineResolutionMs = 5 };
    enum { MinDeadlineResolutionMs = 1 };
    enum { MaxDeadlineResolutionMs = 100 };

    enum { DefaultCleanupPeriodMs = 1000 };
    enum { MinCleanupPeriodMs = 10 };
    enum { MaxCleanupPeriodMs = 10000 };

    DeadlineScheduler deadline_scheduler_;
    Dispatcher dispatcher_;
    MonotonicTime prev_cleanup_ts_;
    MonotonicDuration deadline_resolution_;
    MonotonicDuration cleanup_period_;
    bool inside_spin_;

    MonotonicTime computeDispatcherSpinDeadline(MonotonicTime spin_deadline) const;
    void pollCleanup(MonotonicTime mono_ts, uint32_t num_frames_processed_with_last_spin);

public:
    Scheduler(ICanDriver& can_driver, IPoolAllocator& allocator, ISystemClock& sysclock, IOutgoingTransferRegistry& otr)
        : dispatcher_(can_driver, allocator, sysclock, otr)
        , prev_cleanup_ts_(sysclock.getMonotonic())
        , deadline_resolution_(MonotonicDuration::fromMSec(DefaultDeadlineResolutionMs))
        , cleanup_period_(MonotonicDuration::fromMSec(DefaultCleanupPeriodMs))
        , inside_spin_(false)
    { }

    int spin(MonotonicTime deadline);

    DeadlineScheduler& getDeadlineScheduler() { return deadline_scheduler_; }

    Dispatcher& getDispatcher()             { return dispatcher_; }
    const Dispatcher& getDispatcher() const { return dispatcher_; }

    ISystemClock& getSystemClock()         { return dispatcher_.getSystemClock(); }
    MonotonicTime getMonotonicTime() const { return dispatcher_.getSystemClock().getMonotonic(); }
    UtcTime getUtcTime()             const { return dispatcher_.getSystemClock().getUtc(); }

    MonotonicDuration getDeadlineResolution() const { return deadline_resolution_; }
    void setDeadlineResolution(MonotonicDuration res)
    {
        res = std::min(res, MonotonicDuration::fromMSec(MaxDeadlineResolutionMs));
        res = std::max(res, MonotonicDuration::fromMSec(MinDeadlineResolutionMs));
        deadline_resolution_ = res;
    }

    MonotonicDuration getCleanupPeriod() const { return cleanup_period_; }
    void setCleanupPeriod(MonotonicDuration period)
    {
        period = std::min(period, MonotonicDuration::fromMSec(MaxCleanupPeriodMs));
        period = std::max(period, MonotonicDuration::fromMSec(MinCleanupPeriodMs));
        cleanup_period_ = period;
    }
};

}
