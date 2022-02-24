/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-22 14:41:12
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-22 14:41:12
 */

#include <list>

#include "timer.h"

void TimerArray::unique_timer_timeout()
{
    stop_timer();

    if (timer_list.size() == 0) {
        timer_started = false;
        return;
    }

    double current_time = get_current_time();
    auto &entry = timer_list.front();
    std::list<uint32_t> timeout_pkt;

    while (entry.timeout_time <= current_time) {
        timeout_pkt.push_back(entry.pkt_id);
        timer_list.pop_front();
        
        if (timer_list.size() == 0) break;

        entry = timer_list.front();
        current_time = get_current_time();
    }

    for (auto pkt_id : timeout_pkt) {
        pkt_timeout(pkt_id);
    }

    if (timer_list.size() == 0) {
        timer_started = false;
        return;
    }

    timer_started = true;
    start_timer(timer_list.front().timeout_time - current_time);
}

void TimerArray::new_timer(uint32_t pkt_id, double timeout)
{   
    double current_time = get_current_time();
    double timeout_time = current_time + timeout;

    TimerEntry timer_entry(pkt_id, timeout_time);

    if (timer_list.size() == 0 ||
        timer_list.back().timeout_time <= timeout_time) {
        timer_list.push_back(timer_entry);
    } else {
        bool inserted = false;
        for (auto it = timer_list.begin(); it != timer_list.end(); ++it) {
            if (it->timeout_time > timeout_time) {
                timer_list.insert(it, timer_entry);
                inserted = true;
                break;
            }
        }
        if (!inserted) timer_list.push_back(timer_entry);
    }

    if (!timer_started) {
        timer_started = true;
        start_timer(timer_list.front().timeout_time - current_time);
    }
}