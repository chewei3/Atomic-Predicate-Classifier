#include "bddwrapper.h"

int protocol[protocolBits];
int srcPort[portBits];
int dstPort[portBits];
int srcIP[ipBits];
int dstIP[ipBits];

static int var_init = 0;
static int fwd_predicate_size = 0;
void init_field_var() {
	int i, shift = 0;
	bdd_init(10000, 10000);
	bdd_setvarnum(totalBits);

	for (i = 0; i < ipBits; i++) {
		srcIP[i] = bdd_ithvar(i);
	}
	shift += ipBits;
	for (i = 0; i < ipBits; i++) {
		dstIP[i] = bdd_ithvar(i + shift); // 0:MSB 31:LSB
	}
	shift += portBits;
	for (i = 0; i < portBits; i++) {
		srcPort[i] = bdd_ithvar(i + shift);
	}
	shift += portBits;
	for (i = 0; i < portBits; i++) {
		dstPort[i] = bdd_ithvar(i + shift);
	}
	shift += protocolBits;
	for (i = 0; i < protocolBits; i++) {
		protocol[i] = bdd_ithvar(i + shift);
	}
	var_init = 1;
}

// stdlib qsort
int comPrefixLen(const void *pa, const void *pb) {
	struct PREFIX *a = (struct PREFIX *)pa;
	struct PREFIX *b = (struct PREFIX *)pb;

	if (a->len < b->len)
		return 1;
	else if (a->len == b->len)
		return 0;
	else
		return -1;
}

/*
	Encode from LSB to MSB
	var[0] = MSB, var[num_bits-1] = LSB
*/
BDD encodePrefix(uint32_t IP, uint8_t len, int32_t var[], uint8_t num_bits) {
	int i;
	int index = num_bits - len; // len=15; index=32-15=17
	BDD tmpnode = IP & (1 << index) ? var[num_bits - 1 - index] : bdd_addref(bdd_not(var[num_bits - 1 - index]));

	for (i = index + 1; i < num_bits; i++) {
		BDD tmpnode2 = (IP & (1 << i)) ? var[num_bits - 1 - i] : bdd_addref(bdd_not(var[num_bits - 1 - i]));
		BDD tmpnode3 = bdd_addref(bdd_apply(tmpnode, tmpnode2, bddop_and));
		bdd_delref(tmpnode);
		bdd_delref(tmpnode2);
		tmpnode = tmpnode3;
	}
	return tmpnode;
}

BDD encodeSrcIPPrefix(uint32_t IP, uint8_t len) {
	return len ? encodePrefix(IP, len, srcIP, ipBits) : bddtrue;
}

BDD encodeDstIPPrefix(uint32_t IP, uint8_t len) {
	return len ? encodePrefix(IP, len, dstIP, ipBits) : bddtrue;
}

BDD encodeSrcPortPrefix(uint32_t IP, uint8_t len) {
	if (!len) {
		return bddtrue;
	}
	return encodePrefix(IP, len, srcPort, ipBits);
}

BDD encodeDstPortPrefix(uint32_t IP, uint8_t len) {
	if (!len) {
		return bddtrue;
	}
	return encodePrefix(IP, len, dstIP, ipBits);
}

BDD encodeProtocolPrefix(uint32_t IP) {
	return encodePrefix(IP, protocolBits, protocol, ipBits);
}

// Algorithm 2: Converting a forwarding table to forwarding predicates
BDD *getfwbdds(struct PREFIX *table, int num_prefix, int uni_port) {
	int i;
	BDD fwded = bddfalse;
	BDD *fwdbdds = (BDD *)calloc(sizeof(BDD), uni_port);

	if (!var_init) {
		printf("error: not init var\n");
		exit(0);
	}

	for (i = 0; i < num_prefix; i++) {
		BDD entrybdd = encodeDstIPPrefix(table[i].IP, table[i].len);
		BDD not_fwded = bdd_addref(bdd_not(fwded));
		BDD toadd = bdd_addref(bdd_apply(entrybdd, not_fwded, bddop_and));
		bdd_delref(not_fwded);
		BDD altmp = bdd_addref(bdd_apply(fwded, entrybdd, bddop_or));
		bdd_delref(fwded);
		fwded = altmp;
		bdd_delref(entrybdd);

		int portID = table[i].portID;
		if (fwdbdds[portID]) {
			BDD oldbdd = fwdbdds[portID];
			BDD newbdd = bdd_addref(bdd_apply(oldbdd, toadd, bddop_or));
			bdd_delref(toadd);
			bdd_delref(oldbdd);
			fwdbdds[portID] = newbdd;
		}
		else {
			fwdbdds[portID] = toadd;
			fwd_predicate_size++;
		}
	}

	return fwdbdds;
}

BDD *computeFWBDDs(int tableID) {
	// stdlib qsort, ascending order by length
	qsort(PREFIX_TABLE[tableID], num_prefix[tableID], sizeof(struct PREFIX), comPrefixLen);

	return getfwbdds(PREFIX_TABLE[tableID], num_prefix[tableID], uni_port[tableID]);
}

BDD *converACLRule(struct RULE acl) {
	/*
		0: any, 6: TCP, 17: UDP
		classbench may have many protocols
	*/
	int protocol = bddtrue;
	if (!acl.proto) {
		encodeProtocolPrefix(acl.proto);
	}

	int src_port = bddtrue;
	if (!(acl.srcPort[1] - acl.srcPort == 65535)) {
		// DRPC
	}
}
BDD *computeACLBDDs(int tableID) {
	int i;
	int res = bddfalse;
	int denyBuffer = bddfalse;
	int denyBuffernot = bddtrue;

	for (i = 0; i < num_acl[tableID]; i++) {
		struct RULE *rule = ACL_TABLE[tableID][i].list;
		while (!head) {
			int rule_bdd = converACLRule(rule);

			rule = rule->next;
		}
	}
}
inline unsigned long long int rdtsc() {
	unsigned long long int x;
	asm   volatile ("rdtsc" : "=A" (x));
	return x;
}
struct Hashtable {
	struct Hashtable *next;
	int BDDid;
	int APnum;
};

void parseDevice() {
	int i;
	init_field_var();

	BDD **fwbdds;
	fwbdds = (BDD **)malloc(num_router * sizeof(BDD *));
	unsigned long long int begin, end;
	for (i = 0; i < num_router; i++) {
		printf("router %d\n", i);
		begin = rdtsc();
		fwbdds[i] = computeFWBDDs(i);
		end   = rdtsc();
		if (end > begin) {
			printf("computing %d bdds takes %5f ns\n", i, ((end - begin) / 3.2f));
		}
		else {
			printf("computing %d bdds takes %5f ns\n", i, ((begin - end) / 3.2f));
		}
	}

	if (!rule_none) {
		BDD **aclbdds;
		aclbdds = (BDD **)malloc(num_router * sizeof(BDD *));
		for (i = 0; i < num_router; i++) {
			computeACLBDDs(i);
		}
	}
	// compute atomic predicate
	// struct Hashtable *done = (struct Hashtable *)malloc(sizeof(struct Hashtable) * (1 << fwd_predicate_size));
	// // for debug
	// if (!done) {
	// 	printf("malloc error:var done\n");
	// 	exit(0);
	// }
	// for (i = 0; i < num_router; i++) {
	// 	get_bddID(index, );
	// }
	// BDD **aclbdds;
	// aclbdds = (BDD **)malloc(num_router * sizeof(BDD *));
	// if (!rule_none) {
	// 	for (i = 0; i < num_router; i++) {
	// 		begin = rdtsc();
	// 		aclbdds[i] = computeACLBDDs(i);
	// 		end   = rdtsc();
	// 	}
	// }
}