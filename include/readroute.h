#ifndef READROUTE_H
#define READROUTE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bdd.h>
#include "bddwrapper.h"
#define MAX_ROUTER   100


struct PREFIX {
    unsigned int  IP;
    unsigned char len, portID;
    char *interface;
};

extern int srcIP[ipBits];
extern int dstIP[ipBits];
extern int srcPort[portBits];
extern int dstPort[portBits];
extern int protocol[protocolBits];
extern int  num_router, name_set;
extern char *router_name[MAX_ROUTER];
extern struct PREFIX **PREFIX_TABLE;
extern int *num_prefix, *uni_port, route_none;

void route2all(char *);
void read_all_route();

#endif
