/*
 * @Description: 
 * @Autor: Unknown
 * @Date: Unknown
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-28 00:30:45
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

#include "utils/buffer.h"
#include "utils/checksum.h"
#include "utils/timer.h"

static BufferArray buffer;
static int64_t window_start_id;
static int64_t window_end_id;
static size_t msg_count;

inline void send(size_t pkt_id);

void timeout(uint32_t pkt_id)
{
    if (pkt_id < window_start_id) return;
    if (buffer[pkt_id].acked) return;

    // did not receieve ack, resend
    printf("At %.2fs: sender pkt %u timeout ...\n", GetSimulationTime(), pkt_id);
    send(pkt_id);
}

static TimerArray timer_array(GetSimulationTime, Sender_StartTimer, Sender_StopTimer, timeout);

inline void send(size_t pkt_id)
{
    fprintf(stdout, "At %.2fs: sender sending pkt %lu ...\n", GetSimulationTime(), pkt_id);
    packet *pkt = buffer[pkt_id].get_packet();
    sign(pkt);
    Sender_ToLowerLayer(pkt);
    timer_array.new_timer(pkt_id, RT_TIMEOUT);
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
    while ((window_end_id - window_start_id < WINDOW_SIZE - 1)
        && (window_end_id + 1 < (int64_t)buffer.size())) {
        ++window_end_id;
        send(window_end_id);
    }
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
    buffer.clear();
    window_start_id = -1;
    window_end_id = -1;
    msg_count = 0;
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
    window_start_id = -1;
    window_end_id = -1;
    msg_count = 0;
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{   
    if (msg->size <= 0) return;
    
    fprintf(stdout, "At %.2fs: sender received msg %lu, size = %d ...\n", GetSimulationTime(), msg_count++, msg->size);
    uint32_t pkt_num = msg->size / PLDSIZE_MAX;
    uint32_t pkt_id = buffer.size();

    auto start_pkt = buffer.end();
    --start_pkt;

    // cut msg into several packets
    for (uint32_t i = 0; i < pkt_num; ++i) {
        BufferEntry entry(false, pkt_id++, NORMAL_MSG, PLDSIZE_MAX, msg->data + PLDSIZE_MAX * i);
        buffer.push_back(entry);
    }

    uint32_t remain_data_size = msg->size % PLDSIZE_MAX;
    if (remain_data_size != 0) {
        BufferEntry entry(false, pkt_id++, NORMAL_MSG, remain_data_size, msg->data + PLDSIZE_MAX * pkt_num);
        buffer.push_back(entry);
    }

    // tag the first with NEW_MSG bit and the last with END_MSG bit
    (++start_pkt)->add_fun_code(NEW_MSG);
    buffer.back().add_fun_code(END_MSG);

    fillup_window();
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{   
    if (!check(pkt)) {
        fprintf(stdout, "At %.2fs: sender received a broken pkt ... \n", GetSimulationTime());
        return;
    }

    FunctionCode fun_code = get_fun_code(pkt);
    uint32_t pkt_id = get_pkt_id(pkt);
    fprintf(stdout, "At %.2fs: sender received a pkt from receiver ... ", GetSimulationTime());
    
    if (pkt_id < window_start_id || pkt_id > window_end_id) return;

    if (fun_code & PKT_ACK) {
        fprintf(stdout, "It's an ACK for pkt %u\n", pkt_id);
        buffer[pkt_id].acked = true;
        while (buffer.queue_size() > 0 && buffer.front().acked) {
            window_start_id = buffer.front().get_packet_id() + 1;
            fprintf(stdout, "At %.2fs: sender increase window_start_id to %ld ...\n", GetSimulationTime(), window_start_id);
            buffer.pop_front();
        }

        fillup_window();
    } else {
        ASSERT(false);
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    timer_array.unique_timer_timeout();
}
