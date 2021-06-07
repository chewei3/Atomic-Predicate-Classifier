#include "bddwrapper.h"

BDD protocol[protocolBits];
BDD srcPort[portBits];
BDD dstPort[portBits];
BDD srcIP[ipBits];
BDD dstIP[ipBits];

static int var_init = 0;
static int fwd_predicate_size = 0;
void init_field_var() {
	int i, shift = 0;
	bdd_init(10000, 10000);
	bdd_setvarnum(totalBits);

	// 0:MSB 31:LSB
	for (i = 0; i < ipBits; i++) {
		srcIP[i] = bdd_ithvar(i); //0~31
	}
	shift += ipBits;
	for (i = 0; i < ipBits; i++) {
		dstIP[i] = bdd_ithvar(i + shift); // 32~63
	}
	shift += ipBits;
	for (i = 0; i < portBits; i++) {
		srcPort[i] = bdd_ithvar(i + shift); // 64~79
	}
	shift += portBits;
	for (i = 0; i < portBits; i++) {
		dstPort[i] = bdd_ithvar(i + shift); // 80~95
	}
	shift += portBits;
	for (i = 0; i < protocolBits; i++) {
		protocol[i] = bdd_ithvar(i + shift); // 96~103
	}
	var_init = 1;
}

// stdlib qsort
int comPrefixLen(const void *pa, const void *pb) {
	struct PREFIX *a = (struct PREFIX *)pa;
	struct PREFIX *b = (struct PREFIX *)pb;

	return b->len - a->len;
	// if (a->len > b->len)
	// 	return -1;
	// else if (a->len == b->len)
	// 	return 0;
	// else
	// 	return 1;
}

/*
	Encode from LSB to MSB
	var[0] = MSB, var[num_bits-1] = LSB
*/
BDD encodePrefix(uint32_t IP, uint8_t len, int32_t var[], uint8_t num_bits) {
	int i;
	int index = num_bits - len; // len=15; index=32-15=17
	BDD tmpnode = IP & (1 << index) ? bdd_addref(var[num_bits - 1 - index]) : bdd_addref(bdd_not(var[num_bits - 1 - index]));

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
	return len ? encodePrefix(IP, len, srcPort, portBits) : bddtrue;
}

BDD encodeDstPortPrefix(uint32_t IP, uint8_t len) {
	return len ? encodePrefix(IP, len, dstIP, portBits) : bddtrue;
}

BDD encodeProtocolPrefix(uint32_t IP) {
	return encodePrefix(IP, protocolBits, protocol, protocolBits);
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

BDD *computeFWBDDs(int32_t tableID) {
	// stdlib qsort, Descending order by length
	qsort(PREFIX_TABLE[tableID], num_prefix[tableID], sizeof(struct PREFIX), comPrefixLen);

	return getfwbdds(PREFIX_TABLE[tableID], num_prefix[tableID], uni_port[tableID]);
}

uint32_t ctz(uint32_t n) {
	uint8_t  bits = 0;
	uint32_t x = n;

	if (x) {
		/* assuming `x` has 32 bits: lets count the low order 0 bits in batches */
		/* mask the 16 low order bits, add 16 and shift them out if they are all 0 */
		if (!(x & 0x0000FFFF)) { bits += 16; x >>= 16; }
		/* mask the 8 low order bits, add 8 and shift them out if they are all 0 */
		if (!(x & 0x000000FF)) { bits +=  8; x >>=  8; }
		/* mask the 4 low order bits, add 4 and shift them out if they are all 0 */
		if (!(x & 0x0000000F)) { bits +=  4; x >>=  4; }
		/* mask the 2 low order bits, add 2 and shift them out if they are all 0 */
		if (!(x & 0x00000003)) { bits +=  2; x >>=  2; }
		/* mask the low order bit and add 1 if it is 0 */
		bits += (x & 1) ^ 1;
	}
	return bits;
}


uint32_t DRPC(uint32_t l, uint32_t u, uint32_t bits, range *r) {
	uint32_t zeros = ctz(l);
	if (!zeros) {
		r->prefix[r->count_prefix] = l;
		r->len[r->count_prefix++] = bits;
		return l + 1;
	}
	else {
		while (l + (1 << zeros) > u + 1) {
			zeros--;
		}
		r->prefix[r->count_prefix] = l;
		r->len[r->count_prefix++] = bits - zeros;
		return l + (1 << zeros);
	}
}

BDD converACLRule(struct RULE *acl) {
	/*
		0: any, 6: TCP, 17: UDP
		classbench may have many protocols
	*/

	BDD field_bdd[5]; // protocol, src_port, dst_port, src_IP, dst_IP
	for (int i = 0; i < 5; ++i)
		field_bdd[i] = bddtrue;

	// field_bdd[0] = bddtrue;
	if (acl->proto) {
		field_bdd[0] = encodeProtocolPrefix(acl->proto);
	}

	// field_bdd[1] = bddtrue;
	if (!(acl->srcPort[1] - acl->srcPort[0] == 65535)) {
		// DRPC
		range r = {
			.count_prefix = 0
		};
		uint32_t l = acl->srcPort[0];
		uint32_t u = acl->srcPort[1];

		while (l <= u) {
			l = DRPC(l, u, portBits, &r);
		}

		BDD tmpnode = encodeSrcPortPrefix(r.prefix[0], r.len[0]);
		for (int i = 1; i < r.count_prefix; ++i) {
			BDD tmpnode2 = encodeSrcPortPrefix(r.prefix[i], r.len[i]);
			BDD tmpnode3 = bdd_addref(bdd_apply(tmpnode, tmpnode2, bddop_or));
			bdd_delref(tmpnode);
			bdd_delref(tmpnode2);
			tmpnode = tmpnode3;
		}
		field_bdd[1] = tmpnode;
	}

	// field_bdd[2] = bddtrue;
	if (!(acl->dstPort[1] - acl->dstPort[0] == 65535)) {
		// DRPC
		range r = {
			.count_prefix = 0
		};
		uint32_t l = acl->dstPort[0];
		uint32_t u = acl->dstPort[1];

		while (l <= u) {
			l = DRPC(l, u, portBits, &r);
		}
		BDD tmpnode = encodeDstPortPrefix(r.prefix[0], r.len[0]);
		for (int i = 1; i < r.count_prefix; ++i) {
			BDD tmpnode2 = encodeDstPortPrefix(r.prefix[i], r.len[i]);
			BDD tmpnode3 = bdd_addref(bdd_apply(tmpnode, tmpnode2, bddop_or));
			bdd_delref(tmpnode);
			bdd_delref(tmpnode2);
			tmpnode = tmpnode3;
		}
		field_bdd[2] = tmpnode;
	}

	field_bdd[3] = encodeSrcIPPrefix(acl->srcIP, acl->srclen);
	field_bdd[4] = encodeDstIPPrefix(acl->dstIP, acl->dstlen);

	BDD res = bddtrue;
	for (int i = 0; i < 5; ++i) {
		if (i == 0) {
			res = bdd_addref(field_bdd[i]);
		}
		else {
			if (field_bdd[i] == bddtrue) {
				continue;
			}
			if (field_bdd[i] == bddfalse) {
				bdd_delref(res);
				printf("field %d is bddfalse\n", i);
				res = bddfalse;
				break;
			}
			BDD tmpnode2 = bdd_addref(bdd_apply(res, field_bdd[i], bddop_and));
			bdd_delref(res);
			res = tmpnode2;
		}
	}
	return res;
}

BDD computeACLBDD(int32_t tableID) {
	int i;
	BDD res = bddfalse;
	BDD denyBuffer = bddfalse;
	BDD denyBuffernot = bddtrue;

	for (i = 0; i < num_acl[tableID]; i++) {
		struct RULE *rule = ACL_TABLE[tableID][i].list;
		while (rule) {
			BDD rule_bdd = converACLRule(rule);

			if (rule->action == 1) { // permit rule
				if (res == bddfalse) {
					if (denyBuffer == bddfalse) {
						res = rule_bdd;
					}
					else {
						BDD tmpnode = bdd_addref(bdd_apply(rule_bdd, denyBuffernot, bddop_and));
						res = tmpnode;
						bdd_delref(rule_bdd);
					}
				}
				else {
					if (denyBuffer == bddfalse) {
						BDD tmpnode = bdd_addref(bdd_apply(res, rule_bdd, bddop_or));
						bdd_delref(res);
						bdd_delref(rule_bdd);
						res = tmpnode;
					}
					else {
						BDD tmpnode = bdd_addref(bdd_apply(rule_bdd, denyBuffernot, bddop_and));
						bdd_delref(rule_bdd);

						BDD tmpnode2 = bdd_addref(bdd_apply(res, tmpnode, bddop_or));
						bdd_delref(res);
						bdd_delref(tmpnode);
						res = tmpnode2;
					}
				}
			}
			else { // deny rule
				if (denyBuffer == bddfalse) {
					denyBuffer = rule_bdd;
					denyBuffernot = bdd_addref(bdd_not(denyBuffer));
				}
				else {
					BDD tmpnode = bdd_addref(bdd_apply(denyBuffer, rule_bdd, bddop_or));
					bdd_delref(rule_bdd);
					bdd_delref(denyBuffer);
					denyBuffer = tmpnode;
					bdd_delref(denyBuffernot);
					denyBuffernot = bdd_addref(bdd_not(denyBuffer));
				}
			}
			rule = rule->next;
		}
	}
	return res;
}

inline unsigned long long int rdtsc() {
	unsigned long long int x;
	asm   volatile ("rdtsc" : "=A" (x));
	return x;
}

void parseDevice() {
	int i;
	init_field_var();

	// BDD **fwbdds = NULL;
	// fwbdds = (BDD **)malloc(num_router * sizeof(BDD *));
	// unsigned long long int begin, end;
	// for (i = 0; i < num_router; i++) {
	// 	printf("router %d\n", i);
	// 	begin = rdtsc();
	// 	fwbdds[i] = computeFWBDDs(i);
	// 	end   = rdtsc();
	// 	if (end > begin) {
	// 		printf("computing %d bdds takes %5f ns\n", i, ((end - begin) / 3.2f));
	// 	}
	// 	else {
	// 		printf("computing %d bdds takes %5f ns\n", i, ((begin - end) / 3.2f));
	// 	}
	// }

	BDD *aclbdds = NULL;
	if (!rule_none) {
		aclbdds = (BDD *)malloc(num_router * sizeof(BDD));
		for (i = 0; i < num_router; i++) {
			aclbdds[i] = computeACLBDD(i);
			// printf("%d-th acl_predicate:\n", i);
			// bdd_printtable(aclbdds[i]);
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