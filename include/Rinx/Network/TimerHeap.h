#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H

#include <vector>
#include <memory>
#include <unordered_map>
#include "Timer.h"

namespace Rinx {

class RxTimerHeap
{
    friend RxTimer;
    static constexpr TimerID TimerIDMax=std::numeric_limits<std::uint64_t>::max();
public:
    RxTimerHeap();

    /// @brief check whether there is timer has expired, if any, call the timeout callback
    /// @return the number of timers expired
    int check_timer_expiry();

    size_t get_timer_num() const;

    /// @brief get how many milliseconds from now till the first timeout happends
    /// @return 0 if no timer is going to timeout
    uint64_t get_timeout_interval();

private:
    struct heap_entry{
        uint64_t expire_time;
        RxTimer *timer;
    };

    TimerID add_timer(uint64_t expiry_time,RxTimer *timer);

    void remove_timer(TimerID id);
    void remove_timer(RxTimer *timer);

    ///---heap helper functions
    void heap_init();

    void heap_sift_up(size_t index);
    void heap_sift_down(size_t index);

    void pop_heap_top();
    heap_entry* get_heap_top();

    void swap_heap_entry(heap_entry &entry_a,heap_entry &entry_b);

private:
    TimerID _next_timer_id;

    std::vector<heap_entry> _timer_heap;
    std::unordered_map<TimerID,RxTimer *> _timer_id_map;

};

} //namespace Rinx

#endif // TIMERQUEUE_H
