#include <bdd.h>
#include <stdio.h>
#include <stdlib.h>
BDD encodePrefix(uint32_t IP, uint8_t len, int32_t var[], uint8_t num_bits) {
    int i;
    int index = num_bits - len; // len=15; index=32-15=17
    BDD tmpnode = IP & (1 << index) ? var[31 - index] : bdd_addref(bdd_not(var[31 - index]));

    for (i = index + 1; i < num_bits; i++) {
        BDD tmpnode2 = (IP & (1 << i)) ? var[31 - i] : bdd_addref(bdd_not(var[31 - i]));
        BDD tmpnode3 = bdd_addref(bdd_apply(tmpnode, tmpnode2, bddop_and));
        bdd_delref(tmpnode);
        bdd_delref(tmpnode2);
        tmpnode = tmpnode3;
    }
    return tmpnode;
}
int main()
{
    bdd_init(1000,100);
    bdd_setvarnum(5);
    BDD tmp;
    BDD fwded = bddfalse, not_fwded = bddtrue;
    BDD entrybdd, toadd, altmp;
    BDD x[4];
    for (int i = 0; i < 4; ++i) {
        x[i]=bdd_ithvar(i);
    }
    
    entrybdd = encodeDstIPPrefix(110, 3);
    not_fwded = bdd_addref(bdd_not(fwded));
    toadd = bdd_addref(bdd_apply(entrybdd, not_fwded, bddop_and));
    bdd_delref(not_fwded);
    altmp = bdd_addref(bdd_apply(fwded, entrybdd, bddop_or));
    bdd_delref(fwded);
    fwded = altmp;
    bdd_delref(entrybdd);
    // bdd_printtable(z);
    bdd_delref(z);  

    bdd_printtable(z);
    bdd_done();

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
