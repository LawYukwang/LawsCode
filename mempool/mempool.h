//define the data structures here

typedef struct _list_node
{
    void *addr;
    struct _list_node *next_node;
}list_node;

typedef struct _manager_node
{
    list_node *start_node;
    list_node *last_node;
    unsigned int addr_start;
    unsigned int addr_end;
    int node_num;
    int available_num;
}manager_node;

manager_node manager_list[3];

list_node list_8_node[400];
list_node list_16_node[200];
list_node list_32_node[100];

#define assert_param(param)          if(param==NULL) return 0;