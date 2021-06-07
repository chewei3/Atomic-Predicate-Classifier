// #include <stdio.h>
#define protocolBits 8
#define portBits 16
#define ipBits 32
#define totalBits 104

#ifndef BDDWRAPPER_H
#define BDDWRAPPER_H
#include <stdlib.h>
#include <stdint.h>
#include "readroute.h"
#include "readrule.h"



// for DRPC
typedef struct {
	uint32_t prefix[30], count_prefix;
	uint8_t  len[30];
} range;

extern int protocol[protocolBits];
extern int srcPort[portBits];
extern int dstPort[portBits];
extern int srcIP[ipBits];
extern int dstIP[ipBits];

extern void init_field_var();
extern void parseDevice();
#endif