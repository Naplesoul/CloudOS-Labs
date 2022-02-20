/*
 * @Description: 
 * @Autor: Unknown
 * @Date: Unknown
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-20 23:12:42
 */

/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  4 byte  ->|<-  1  byte  ->|<-  1 byte  ->|<- the rest (max 120 byte) ->|<-  2 byte  ->|
 *       |  packet  ID  | function code | payload size |<-         payload         ->|   checksum   |
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

#include "buffer.h"
#include "checksum.h"
#include "timer.h"

static BufferArray buffer;
static size_t window_start_id;
static size_t window_end_id;

inline void send(size_t pkt_id)
{
    Sender_ToLowerLayer(buffer[pkt_id].get_packet());
}

inline FunctionCode get_fun_code(packet *pkt)
{
    int fun_code = *(uint8_t *)(pkt->data + PKTID_SIZE);
    return (FunctionCode)fun_code;
}

inline uint32_t get_pkt_id(packet *pkt)
{
    return *(uint32_t *)(pkt->data);
}

inline void fillup_window()
{
    while (window_end_id - window_start_id < WINDOW_SIZE - 1
        && window_end_id < buffer.size() - 1) {
        send(++window_end_id);
    }
}

void timeout(uint32_t pkt_id)
{

}

static TimerArray timer_array(GetSimulationTime, Sender_StartTimer, Sender_StopTimer, timeout);

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    uint32_t pkt_num = msg->size / PLDSIZE_MAX;
    uint32_t pkt_id = buffer.size();
    
    BufferEntry header_entry(false, pkt_id++, NEW_MSG, 0, nullptr);
    buffer.push_back(header_entry);

    for (uint32_t i = 0; i < pkt_num; ++i) {
        BufferEntry entry(false, pkt_id++, NORMAL_MSG, PLDSIZE_MAX, msg->data + PLDSIZE_MAX * i);
        buffer.push_back(entry);
    }

    uint32_t remain_data_size = msg->size % PLDSIZE_MAX;
    if (remain_data_size != 0) {
        BufferEntry entry(false, pkt_id++, NORMAL_MSG, remain_data_size, msg->data + PLDSIZE_MAX * pkt_num);
        buffer.push_back(entry);
    }

    BufferEntry end_entry(false, pkt_id, END_MSG, 0, nullptr);
    buffer.push_back(end_entry);

    fillup_window();
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{   
    FunctionCode fun_code = get_fun_code(pkt);
    uint32_t pkt_id = get_pkt_id(pkt);

    if (fun_code == PKT_ACK) {
        buffer[pkt_id].acked = true;
        while (buffer.front().acked) {
            window_start_id = buffer.front().get_packet_id() + 1;
            buffer.pop_front();
        }

        fillup_window();
    } else if (fun_code == PKT_LOST) {
        send(pkt_id);
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    timer_array.unique_timer_timeout();
}
