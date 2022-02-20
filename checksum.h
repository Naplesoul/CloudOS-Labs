/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-19 21:57:57
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-20 17:05:53
 */

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include <stdint.h>
#include <stddef.h>

#include "rdt_struct.h"

#define CHECKSUM_SIZE 16
#define BUFFER_SIZE (RDT_PKTSIZE - CHECKSUM_SIZE)

inline uint16_t checksum(char *buf)
{
    uint16_t *buf16 = (uint16_t *)buf;
    uint32_t sum = 0;
    for (int i = 0; i < BUFFER_SIZE / 2; ++i) {
        sum += *buf16++;
    }

#if (BUFSIZE % 2 != 0)
    uint16_t last = *(char *)buf16;
    sum += last << 8
#endif

    uint16_t carry = sum >> 16;
    while (carry) {
        sum = (sum & 0xffff) + carry;
        carry = sum >> 16;
    }
    return ~(uint16_t)sum;
}

inline bool check_ok(char *buf, uint16_t checksum)
{
    uint16_t *buf16 = (uint16_t *)buf;
    uint32_t sum = 0;
    for (int i = 0; i < BUFFER_SIZE / 2; ++i) {
        sum += *buf16++;
    }

#if (BUFSIZE % 2 != 0)
    uint16_t last = *(char *)buf16;
    sum += last << 8
#endif

    uint16_t carry = sum >> 16;
    while (carry) {
        sum = (sum & 0xffff) + carry;
        carry = sum >> 16;
    }
    return ((uint16_t)sum + checksum) == 0xffff;
}

void sign(packet *packet)
{
    *(uint16_t *)(packet->data + BUFFER_SIZE) = checksum(packet->data);
}

bool check(packet *packet)
{
    return check_ok(packet->data, *(uint16_t *)(packet->data + BUFFER_SIZE));
}

#endif