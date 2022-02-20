/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-20 21:41:53
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-21 00:18:52
 */

#include <list>
#include <stdint.h>

struct TimerEntry
{
    uint32_t pkt_id;
    double timeout_time;
    TimerEntry(uint32_t _pkt_id, double _timeout_time):
        pkt_id(_pkt_id), timeout_time(_timeout_time) {}
};

class TimerArray
{
private:
    double (*get_current_time)();
    void (*start_timer)(double timeout);
    void (*stop_timer)();
    void (*pkt_timeout)(uint32_t pkt_id);
    
    std::list<TimerEntry> timer_list;

public:
    TimerArray(double _get_time(), void _start_timer(double),
               void _stop_timer(), void _pkt_timeout(uint32_t)):
        get_current_time(_get_time),
        start_timer(_start_timer),
        stop_timer(_stop_timer),
        pkt_timeout(_pkt_timeout) {}
    
    void unique_timer_timeout();
    void new_timer(uint32_t pkt_id, double timeout);
};

void TimerArray::unique_timer_timeout()
{
    stop_timer();

    double current_time = get_current_time();
    auto &entry = timer_list.front();
    while (entry.timeout_time <= current_time) {
        pkt_timeout(entry.pkt_id);
        timer_list.pop_front();
        entry = timer_list.front();
        current_time = get_current_time();
    }

    start_timer(timer_list.front().timeout_time - current_time);
}

void TimerArray::new_timer(uint32_t pkt_id, double timeout)
{
    stop_timer();
    
    double current_time = get_current_time();
    double timeout_time = current_time + timeout;
    TimerEntry entry(pkt_id, timeout_time);

    bool inserted = false;
    for (auto it = timer_list.begin(); it != timer_list.end(); ++it) {
        if (it->timeout_time > timeout_time) {
            timer_list.insert(it, entry);
            inserted = true;
            break;
        }
    }
    if (!inserted) timer_list.push_back(entry);

    current_time = get_current_time();
    auto &entry = timer_list.front();
    while (entry.timeout_time <= current_time) {
        pkt_timeout(entry.pkt_id);
        timer_list.pop_front();
        entry = timer_list.front();
        current_time = get_current_time();
    }

    start_timer(timer_list.front().timeout_time - current_time);
}