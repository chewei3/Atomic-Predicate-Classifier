#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "readroute.h"
#include "readrule.h"
#include "bddwrapper.h"

int main(int argc, char *argv[]) {
	char net_name[20];

	// 額外判斷 real network or classbench
	if (argc <= 1) {
		strcpy(net_name, "router_name");
	}
	else if (argc == 2) {
		strcpy(net_name, argv[1]);
	}
	else {
		printf("usage: ./a.out router_name/table\n");
	}
	route2all(net_name);
	rule2all(net_name);
	parseDevice();

	bdd_done();
	return 0;
}
