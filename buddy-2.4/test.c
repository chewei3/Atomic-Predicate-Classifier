#include <bdd.h>

int main()
{
    bdd x,y,z;

    bdd_init(1000,100);
    bdd_setvarnum(5);

    x = bdd_ithvar(0);
    y = bdd_ithvar(1);
    z = bdd_addref(bdd_apply(x,y,bddop_and));

    bdd_printtable(z);
    bdd_delref(z);
    bdd_done();
}	
