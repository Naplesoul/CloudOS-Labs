/*
 * @Description: 
 * @Autor: Unknown
 * @Date: Unknown
 * @LastEditors: Weihang Shen
 * @LastEditTime: 2022-02-21 00:24:35
 */

/*
 * FILE: rdt_struct.h
 * DESCRIPTION: The header file for basic data structures.
 * NOTE: Do not touch this file!
 */


#ifndef _RDT_STRUCT_H_
#define _RDT_STRUCT_H_

#include <stdint.h>

/* sanity check utility */
#define ASSERT(x) \
    if (!(x)) { \
        fprintf(stdout, "## at file %s line %d: assertion fails\n", __FILE__, __LINE__); \
        exit(-1); \
    }

/* a message is a data unit passed between the upper layer and the rdt layer at 
   the sender */
struct message {
    int size;
    char *data;
};

/* a packet is a data unit passed between rdt layer and the lower layer, each 
   packet has a fixed size */
#define RDT_PKTSIZE 128

struct packet {
    char data[RDT_PKTSIZE];
};

#define WINDOW_SIZE 100
#define RT_TIMEOUT 0.5

enum FunctionCode
{
    PKT_ACK = 1, NORMAL_MSG = 2, NEW_MSG = 4, END_MSG = 8
};

#endif  /* _RDT_STRUCT_H_ */
