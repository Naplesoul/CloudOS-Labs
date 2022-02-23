/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-22 14:41:12
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-22 14:41:12
 */

#include "timer.h"

void TimerArray::unique_timer_timeout()
{
    stop_timer();

    if (timer_list.size() == 0) return;

    double current_time = get_current_time();
    auto &entry = timer_list.front();
    while (entry.timeout_time <= current_time) {
        pkt_timeout(entry.pkt_id);
        timer_list.pop_front();
        
        if (timer_list.size() == 0) break;
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
    TimerEntry timer_entry(pkt_id, timeout_time);

    bool inserted = false;
    for (auto it = timer_list.begin(); it != timer_list.end(); ++it) {
        if (it->timeout_time > timeout_time) {
            timer_list.insert(it, timer_entry);
            inserted = true;
            break;
        }
    }
    if (!inserted) timer_list.push_back(timer_entry);

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