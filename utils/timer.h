/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-20 21:41:53
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-22 14:41:27
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