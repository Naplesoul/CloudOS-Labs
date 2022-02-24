/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-19 21:19:42
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-23 00:14:04
 */

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <deque>
#include <memory>

#include "../rdt_struct.h"
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
    std::shared_ptr<packet> pkt;

public:
    BufferEntry(bool _acked, uint32_t _pkt_id, FunctionCode _fun_code, uint8_t _pld_size, char *pld);
    BufferEntry(packet *_pkt);
    packet *get_packet();
    uint32_t get_packet_id();
    FunctionCode get_fun_code();
    uint8_t get_pld_size();
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
    BufferEntry &back();
    void pop_front();
    void erase_before(size_t idx);
    void push_back(BufferEntry &t);
    size_t size();
    size_t queue_size();
    std::deque<BufferEntry>::iterator begin();
    std::deque<BufferEntry>::iterator end();
    void clear();
    size_t get_start_id();
};

#endif