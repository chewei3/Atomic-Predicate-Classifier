#ifndef READRULE_H
#define READRULE_H
#define MAX_RULE 10000
#include "bdd.h"
#include "readroute.h"
struct ENTRY {
	unsigned short ID;
	struct RULE* list;
};

struct RULE {
	unsigned int srcIP, dstIP, srcmask, dstmask, srclen, dstlen;
	unsigned int srcPort[2], dstPort[2], proto, action;
	struct RULE* next;
};

extern int *num_acl, *num_rule, rule_none;
extern struct ENTRY **ACL_TABLE;

void rule2all(char *);
#endif