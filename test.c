#include <bdd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define bits 16
BDD encodePrefix(uint32_t IP, uint8_t len, int32_t var[], uint8_t num_bits) {
    int i;
    int index = num_bits - len; // len=15; index=32-15=17
    // BDD tmpnode = IP & (1 << index) ? bdd_addref(var[3 - index]) : bdd_addref(bdd_not(var[3 - index]));
    BDD tmpnode = IP & (1 << index) ? var[bits - 1 - index] : bdd_addref(bdd_not(var[bits - 1 - index]));

    for (i = index + 1; i < num_bits; i++) {
        BDD tmpnode2 = (IP & (1 << i)) ? var[bits - 1 - i] : bdd_addref(bdd_not(var[bits - 1 - i]));
        BDD tmpnode3 = bdd_addref(bdd_apply(tmpnode, tmpnode2, bddop_and));
        bdd_delref(tmpnode);
        bdd_delref(tmpnode2);
        tmpnode = tmpnode3;
    }
    return tmpnode;
}

int main()
{
    bdd_init(10000, 10000);
    bdd_setvarnum(16);
    // BDD tmp;
    // BDD fwded = bddfalse, not_fwded = bddtrue;
    // BDD entrybdd, toadd, altmp;
    BDD x[16];
    for (int i = 0; i < 16; ++i) {
        x[i] = bdd_ithvar(i);
    }
    // bdd_printall();
    // printf("sdgfjoeawisjgoiweag\n\n\n");
    // BDD tmpnode = x[0];
    // bdd_printdot(tmpnode);
    // BDD tmpnode2 = x[1];
    // bdd_printdot(tmpnode2);
    // bdd_delref(tmpnode);
    // bdd_delref(tmpnode2);
    // bdd_printdot(x[0]);
    BDD entry = encodePrefix(445, 16, x, bits);
    bdd_printdot(entry);
    // bdd_printtable(entrybdd);
    // printf("sdgfjoeawisjgoiweag\n\n\n");
    // bdd_printall();
    // not_fwded = bdd_addref(bdd_not(fwded));
    // toadd = bdd_addref(bdd_apply(entrybdd, not_fwded, bddop_and));
    // bdd_delref(not_fwded);
    // altmp = bdd_addref(bdd_apply(fwded, entrybdd, bddop_or));
    // bdd_delref(fwded);
    // fwded = altmp;
    // bdd_delref(entrybdd);
    // bdd_printtable(z);
    // bdd_delref(z);

    // bdd_printtable(z);
    // bdd_done();
    // a aa = {.a = 1};
    // f(&aa);
    // printf("%d\n", aa.a);
//    int *aaa[50];
    // aaa[0]=(int *)malloc(sizeof(int));
    // *(aaa[0]) = 123;
//    printf("asdasd:%d\n", *(aaa[0]));
    // int i=0,j=0,x=0;
    // x=(i++) + (i++);
    // i=i=i+1;
    // printf("%d\n", i);
    return 0;
}
