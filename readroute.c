#include "readroute.h"

int  num_router, name_set, route_none;
char *router_name[MAX_ROUTER];
struct PREFIX **PREFIX_TABLE;
int *num_prefix, *uni_port;

static void set_prefix(char *str, int tableID, char *port[]) {
    int i, c = 0;

    for (i = 0; i < 100; i++) {
        if (str[i] == '\'') {
            if (c == 1) {
                str[i] = '\0';
                break;
            }
            else {
                c = 1;
            }
        }
    }
    str++;

    int index = num_prefix[tableID];
    PREFIX_TABLE[tableID][index].interface = (char *) malloc (100 * sizeof(char));
    sscanf(str, "%u, %hhu, '%s", &PREFIX_TABLE[tableID][index].IP, \
           &PREFIX_TABLE[tableID][index].len, PREFIX_TABLE[tableID][index].interface);

    if (!strlen(PREFIX_TABLE[tableID][index].interface)) {
        PREFIX_TABLE[tableID][index].portID = 0;
    }
    else {
        for (i = 0; i < uni_port[tableID]; i++) {
            if (!strcmp(PREFIX_TABLE[tableID][index].interface, port[i])) {
                PREFIX_TABLE[tableID][index].portID = i + 1;
                break;
            }
        }

        if (i == uni_port[tableID]) {
            port[uni_port[tableID]] = PREFIX_TABLE[tableID][index].interface;
            PREFIX_TABLE[tableID][index].portID = ++uni_port[tableID];
        }
    }

    num_prefix[tableID]++;
}

static void read_prefix(char *table_name, int tableID, char *port[]) {
    FILE *fp = fopen(table_name, "r");
    char str[100];
    int  i;

    num_prefix[tableID] = 0;
    uni_port[tableID] = 0;
    while (fgets(str, 100, fp) != NULL) {
        num_prefix[tableID]++;
    }
    rewind(fp);

    PREFIX_TABLE[tableID] = (struct PREFIX *) malloc (num_prefix[tableID] * sizeof(struct PREFIX));
    num_prefix[tableID] = 0;

    for (i = 0; i < 6; i++) {
        fgets(str, 100, fp);
    }

    while (fgets(str, 100, fp) != NULL) {
        set_prefix(str, tableID, port);
    }
    uni_port[tableID]++;

    fclose(fp);
}

static void set_route(char *table_name, int tableID) {
    char port[100][100]; // # of port, port length

    read_prefix(table_name, tableID, port);
}

static void read_route(char *input_file) {
    FILE *fp = fopen(input_file, "r");
    char str[100];

    num_router = 0;
    while (fgets(str, 100, fp) != NULL) {
        num_router++;
    }
    
    rewind(fp);

    PREFIX_TABLE = (struct PREFIX **)malloc(num_router * sizeof(struct PREFIX *));
    num_prefix = (int *)malloc(num_router * sizeof(int));
    uni_port = (int *)malloc(num_router * sizeof(int));

    num_router = 0; int total_route = 0;
    while (fgets(str, 100, fp) != NULL) {
        router_name[num_router] = (char *) malloc (20 * sizeof(char));
        sscanf(str, "%s\n", router_name[num_router]);
        sprintf(str, "route/%s", router_name[num_router]);
        set_route(str, num_router++);

        printf("router %d: %d route, %d port\n", num_router - 1, num_prefix[num_router - 1], uni_port[num_router - 1]); // port 0:drop
        total_route += num_prefix[num_router - 1];
    }
    printf("total_route:%d\n", total_route);
    name_set = 1;

    fclose(fp);
}

void route2all(char *input_file) {
    printf("Reading Routing table...\n");
    read_route(input_file);
    printf("=== Done ===\n");
}

// static int router_n(char *str) {
//     char tok[] = " \n", *s;
//     int  n = 0;

//     s = strtok(str, tok);

//     while(s) {
//         s = strtok(NULL, tok);
//         n++;
//     }

//     return n - 1;
// }