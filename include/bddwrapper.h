// #include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "readroute.h"
#include "readrule.h"

#define protocolBits 8
#define portBits 16
#define ipBits 32
#define totalBits 104

extern int protocol[protocolBits];
extern int srcPort[portBits];
extern int dstPort[portBits];
extern int srcIP[ipBits];
extern int dstIP[ipBits];

extern void init_field_var();
extern void parseDevice();