#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bdd.h>
#include "readrule.h"

#define FREE(ptr) {free(ptr); ptr=NULL;}
int *num_acl, *num_rule, rule_none;
struct ENTRY **ACL_TABLE;
static int totalrule = 0;
static void set_rule(char *str, int tableID, int id) {
    char *s, tok[] = "',";
    int  c;

    str += 3;
    ACL_TABLE[tableID][num_acl[tableID]].ID = id;

    struct RULE head;
    struct RULE *listPtr = &head;
    s = strtok(str, tok);
    unsigned int len, tmp;
    while (1) {
        c = -1;

        if (!strcmp(s, "transport_src_end")) {
            listPtr->next = (struct RULE *) malloc (sizeof(struct RULE));
            listPtr->next->next = NULL;
            num_rule[tableID]++;

            totalrule++;
            c = 0;
        }
        if (!strcmp(s, "ip_protocol"        )) c = 1;
        if (!strcmp(s, "transport_src_begin")) c = 2;
        if (!strcmp(s, "src_ip"             )) c = 3;
        if (!strcmp(s, "dst_ip"             )) c = 4;
        if (!strcmp(s, "src_ip_mask"        )) c = 5;
        if (!strcmp(s, "dst_ip_mask"        )) c = 6;
        if (!strcmp(s, "transport_dst_begin")) c = 7;
        if (!strcmp(s, "transport_dst_end"  )) c = 8;
        if (!strcmp(s, "action"             )) c = 9;

        s = strtok(NULL, tok);
        s += 2;

        switch (c) {
        case 0:
            listPtr = (listPtr->next) ? listPtr->next : listPtr;
            sscanf(s, "%u", &listPtr->srcPort[1]);
            break;
        case 1:
            sscanf(s, "%u", &listPtr->proto);
            break;
        case 2:
            sscanf(s, "%u", &listPtr->srcPort[0]);
            break;
        case 3:
            sscanf(s, "%u", &listPtr->srcIP);
            break;
        case 4:
            sscanf(s, "%u", &listPtr->dstIP);
            break;
        case 5:
            sscanf(s, "%u", &listPtr->srcmask);
            len = 32, tmp = listPtr->srcmask;
            while (tmp) {
                len--;
                tmp >>= 1;
            }
            listPtr->srclen = len;
            break;
        case 6:
            sscanf(s, "%u", &listPtr->dstmask);
            len = 32, tmp = listPtr->dstmask;
            while (tmp) {
                len--;
                tmp >>= 1;
            }
            listPtr->dstlen = len;
            break;
        case 7:
            sscanf(s, "%u", &listPtr->dstPort[0]);
            break;
        case 8:
            sscanf(s, "%u", &listPtr->dstPort[1]);
            break;
        case 9:
            listPtr->action = (!strcmp(s, "True")) ? 1 : 0;
            break;
        default:
            break;
        }

        s = strtok(NULL, tok);
        s = strtok(NULL, tok);

        if (!s) break;
    }

    if (listPtr->next) listPtr->next = NULL;
    ACL_TABLE[tableID][num_acl[tableID]].list = head.next;
    num_acl[tableID]++;
}

static void read_rule(char *table_name, int tableID) {
    FILE *fp = fopen(table_name, "r");
    char str[50000];

    while (fgets(str, 50000, fp) != NULL) {
        num_acl[tableID]++;
    }

    rewind(fp);

    ACL_TABLE[tableID] = (struct ENTRY *) malloc (( num_acl[tableID] - 2) / 2 * sizeof(struct ENTRY));
    num_acl[tableID] = 0;

    fgets(str, 50000, fp);

    while (fgets(str, 50000, fp) != NULL) {
        if (str[0] == '=') break;

        if (str[0] != '[') {
            int id = atoi(str);

            fgets(str, 50000, fp);
            set_rule(str, tableID, id);
        }
    }

    fclose(fp);
}

static void computeACLBDDs(int num_entry, struct ENTRY *table) {
    int i;

    for (i = 0; i < num_entry; i++) {
        //printf("%d %d\n", table[i].ID, table[i].list);
        struct RULE *ptr = table[i].list;
        unsigned int m, l, r;
        int len;

        while (ptr) {
            totalrule++;
            len = 32;
            m = ptr->srcmask;

            while (m) {
                len--;
                m >>= 1;
            }

            ptr->srclen = len;

            len = 32;
            m = ptr->dstmask;

            while (m) {
                len--;
                m >>= 1;
            }

            ptr->dstlen = len;


            ptr = ptr->next;
        }
    }
}

static void set_filter(char *table_name, int tableID) {
    read_rule(table_name, tableID);
    printf("router %d: %d rules\n", tableID, num_rule[tableID]);
}

static void read_filter(char *input_file) {
    FILE *fp;
    char str[100];
    int  i;

    if (!name_set) {
        num_router = 0;
        fp = fopen(input_file, "r");

        while (fgets(str, 100, fp) != NULL) {
            router_name[num_router] = (char *) malloc (20 * sizeof(char));
            sscanf(str, "%s\n", router_name[num_router++]);
        }

        fclose(fp);
    }

    ACL_TABLE = (struct ENTRY **)malloc(num_router * sizeof(struct ENTRY *));
    num_acl = (int *)malloc(num_router * sizeof(int));
    num_rule = (int *)malloc(num_router * sizeof(int));
    memset(num_acl , 0, num_router * sizeof(int));
    memset(num_rule, 0, num_router * sizeof(int));

    for (i = 0; i < num_router; i++) {
        sprintf(str, "rule/%s", router_name[i]);

        fp = fopen(str, "r");
        if (fp == NULL) {
            rule_none = 1;
            FREE(ACL_TABLE);
            FREE(num_acl);
            FREE(num_acl);
            printf("err: No rule table found\n");
            return;
        }

        set_filter(str, i);
    }

    printf("total rule:%d\n", totalrule);
}
void rule2all(char *input_file) {
    printf("Reading Rule table...\n");
    read_filter(input_file);
    printf("=== Done ===\n");
}