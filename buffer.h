/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-19 21:19:42
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-20 21:18:50
 */

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <deque>
#include "rdt_struct.h"
#include "checksum.h"

#define PKTID_SIZE 4
#define FUNCODE_SIZE 1
#define PLDSIZE_SIZE 1
#define PLDSIZE_MAX (RDT_PKTSIZE - PKTID_SIZE - FUNCODE_SIZE - PLDSIZE_SIZE - CHECKSUM_SIZE)

class BufferEntry
{
public:
    bool acked;
private:
    uint8_t pld_size;
    FunctionCode fun_code;
    uint32_t pkt_id;
    packet *pkt;

public:
    BufferEntry(bool _acked, uint32_t _pkt_id, FunctionCode _fun_code, uint8_t _pld_size, char *pld):
        acked(acked), pkt_id(_pkt_id), fun_code(_fun_code), pld_size(_pld_size), pkt(new struct packet)
    {
        *(uint32_t *)(pkt->data) = _pkt_id;
        *(uint8_t *)(pkt->data + PKTID_SIZE) = _fun_code;
        *(uint8_t *)(pkt->data + PKTID_SIZE + FUNCODE_SIZE) = _pld_size;
        if (_pld_size > 0)
            memcpy(pkt->data + PKTID_SIZE + FUNCODE_SIZE + PLDSIZE_SIZE, pld, _pld_size);
        sign(pkt);
    }

    packet *get_packet()
    {
        return pkt;
    }

    uint32_t get_packet_id()
    {
        return pkt_id;
    }
};

class BufferArray
{
private:
    size_t start_idx;
    std::deque<BufferEntry> buf;

public:
    BufferArray(): start_idx(0) {}

    BufferEntry &operator[](size_t idx);
    BufferEntry &front();
    void pop_front();
    void erase_before(size_t idx);
    void push_back(BufferEntry &t);
    size_t size();
};


BufferEntry &BufferArray::operator[](size_t idx)
{
    assert(idx >= start_idx);
    return buf[idx - start_idx];
}

BufferEntry &BufferArray::front()
{
    return buf.front();
}

void BufferArray::pop_front()
{
    buf.pop_front();
    ++start_idx;
}

void BufferArray::erase_before(size_t idx)
{
    auto it = buf.begin();
    it += idx - start_idx;
    start_idx = idx;
    buf.erase(buf.begin(), it);
}

void BufferArray::push_back(BufferEntry &t)
{
    buf.push_back(t);
}

size_t BufferArray::size()
{
    return start_idx + buf.size();
}

#endif