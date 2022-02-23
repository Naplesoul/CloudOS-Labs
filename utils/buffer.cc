/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-22 14:25:16
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-23 00:54:12
 */

#include "buffer.h"
#include "checksum.h"

BufferEntry::BufferEntry(bool _acked, uint32_t _pkt_id, FunctionCode _fun_code, uint8_t _pld_size, char *pld):
    acked(_acked), pld_size(_pld_size), fun_code(_fun_code), pkt_id(_pkt_id), pkt(new packet)
{
    *(uint32_t *)(pkt->data) = _pkt_id;
    *(uint8_t *)(pkt->data + PKTID_SIZE) = _fun_code;
    *(uint8_t *)(pkt->data + PKTID_SIZE + FUNCODE_SIZE) = _pld_size;
    if (_pld_size > 0)
        memcpy(pkt->data + PKTID_SIZE + FUNCODE_SIZE + PLDSIZE_SIZE, pld, _pld_size);
    sign(pkt);
}

BufferEntry::BufferEntry(packet *_pkt)
{
    acked = true;
    pld_size = *(uint8_t *)(_pkt->data + PKTID_SIZE + FUNCODE_SIZE);
    fun_code = (FunctionCode)(*(uint8_t *)(_pkt->data + PKTID_SIZE));
    pkt_id = *(uint32_t *)(_pkt->data);
    pkt = new packet;
    memcpy(pkt->data, _pkt->data, RDT_PKTSIZE);
}

packet *BufferEntry::get_packet()
{
    return pkt;
}

uint32_t BufferEntry::get_packet_id()
{
    return pkt_id;
}

FunctionCode BufferEntry::get_fun_code()
{
    return fun_code;
}

uint8_t BufferEntry::get_pld_size()
{
    return pld_size;
}


BufferEntry &BufferArray::operator[](size_t idx)
{
    assert(idx >= start_idx);
    return buf[idx - start_idx];
}

BufferEntry &BufferArray::front()
{
    return buf.front();
}

BufferEntry &BufferArray::back()
{
    return buf.back();
}

void BufferArray::pop_front()
{
    // delete buf.front().get_packet();
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
    
size_t BufferArray::queue_size()
{
    return buf.size();
}

std::deque<BufferEntry>::iterator BufferArray::begin()
{
    return buf.begin();
}

std::deque<BufferEntry>::iterator BufferArray::end()
{
    return buf.end();
}

void BufferArray::clear()
{
    start_idx = 0;
    buf.clear();
}

size_t BufferArray::get_start_id()
{
    return start_idx;
}