/*
 * @Description: 
 * @Autor: Weihang Shen
 * @Date: 2022-02-19 21:57:57
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-22 14:24:28
 */

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_

#include "../rdt_struct.h"

#define CHECKSUM_SIZE 2
#define BUFFER_SIZE (RDT_PKTSIZE - CHECKSUM_SIZE)

inline uint16_t checksum(char *buf);
inline bool check_ok(char *buf, uint16_t checksum);
void sign(packet *packet);
bool check(packet *packet);

#endif