/*
 * @Description: 
 * @Autor: Unknown
 * @Date: Unknown
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-23 12:00:33
 */

/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list>

#include "rdt_struct.h"
#include "rdt_receiver.h"

#include "utils/buffer.h"
#include "utils/checksum.h"

static BufferArray integrated_buffer;
static std::list<BufferEntry> seq_buffer;

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
    integrated_buffer.clear();
    seq_buffer.clear();
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    if (!check(pkt)) {
        return;
    }
    
    BufferEntry entry(pkt);
    uint32_t pkt_id = entry.get_packet_id();

    BufferEntry ack_entry(false, pkt_id, PKT_ACK, 0, nullptr);
    Receiver_ToLowerLayer(ack_entry.get_packet());
    delete[] ack_entry.get_packet();

    uint32_t next_integrated_id = integrated_buffer.size();

    if (pkt_id < next_integrated_id) {
        delete[] entry.get_packet();
        return;
    }

    if (pkt_id != next_integrated_id) {
        bool inserted = false;
        for (auto it = seq_buffer.begin(); it != seq_buffer.end(); ++it) {
            if (it->get_packet_id() > pkt_id) {
                seq_buffer.insert(it, entry);
                inserted = true;
                break;
            } else if (it->get_packet_id() == pkt_id) {
                delete[] entry.get_packet();
                return;
            }
        }
        if (!inserted) seq_buffer.push_back(entry);
        return;
    }

    integrated_buffer.push_back(entry);
    next_integrated_id++;

    while (seq_buffer.size() > 0
        && seq_buffer.front().get_packet_id() == next_integrated_id) {
                
        integrated_buffer.push_back(seq_buffer.front());
        seq_buffer.pop_front();
        next_integrated_id++;
    }

    if (integrated_buffer.queue_size() == 0) return;

    uint32_t last_end_id = integrated_buffer.front().get_packet_id();
    for (auto it = integrated_buffer.begin(); it != integrated_buffer.end(); ++it) {
        if (it->get_fun_code() == NEW_MSG) {
            message msg;
            uint32_t size = 0;

            auto msg_end = it + 1;
            bool msg_ended = false;
            for (; msg_end != integrated_buffer.end(); ++msg_end) {
                if (msg_end->get_fun_code() == END_MSG) {
                    last_end_id = it->get_packet_id();
                    msg_ended = true;
                    break;
                } else if (msg_end->get_fun_code() == NORMAL_MSG) {
                    size += msg_end->get_pld_size();
                } else {
                    ASSERT(false);
                }
            }
            if (!msg_ended) break;

            msg.data = new char[size];
            msg.size = size;
            size = 0;
            for (auto msg_part = it + 1; msg_part != msg_end; ++msg_part) {
                memcpy(msg.data + size, pkt->data + PKTID_SIZE + FUNCODE_SIZE + PLDSIZE_SIZE, msg_part->get_pld_size());
                size += msg_part->get_pld_size();
            }

            fprintf(stdout, "At %.2fs: receiver assembled a complete msg ...\n", GetSimulationTime());
            Receiver_ToUpperLayer(&msg);
            delete[] msg.data;
            it = msg_end;
        }
    }
    
    integrated_buffer.erase_before(last_end_id);
}
