#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mempool.h"

list_node *available[3];
list_node *rear[3];

extern manager_node     manager_list[3];
extern list_node     list_8_node[400];
extern list_node     list_16_node[200];
extern list_node     list_32_node[100];

int count = 0;

typedef list_node* (*init_ptr) (list_node **);

list_node * eight_init(list_node **last)
{
    void *a = malloc(8*400);
    printf("the list8 start addr is %x\n",(int)a);
    int i;
    assert_param(a);
    for(i=0;i<400;i++)
    {
		char * trans = (char *)a + i*8;
		list_8_node[i].addr=(void*)trans;
        list_8_node[i].next_node=NULL;
        //printf("the list %d addr is %x\n",i,(int)(list_8_node[i].addr));
        if(i==399)
            break;
        list_8_node[i].next_node=&list_8_node[i+1];
    }
    *last=&(list_8_node[i]);
    printf("the list8 node %dend addr is %x\n\n",i,(int)(list_8_node[i].addr));
    return (&list_8_node[0]);
}

list_node * sixteen_init(list_node **last)
{
    void *a = malloc(16*400);
    printf("the list16 start addr is %x\n",(int)a);
    int i;
    assert_param(a);
    for(i=0;i<400;i++)
    {
		char * trans = (char *)a + i*16;
        list_16_node[i].addr=(void*)trans;
        list_16_node[i].next_node=NULL;
        //printf("the list %d addr is %x\n",i,(int)(list_16_node[i].addr));
        if(i==199)
            break;
        list_16_node[i].next_node=&list_16_node[i+1];
    }
    *last=&(list_16_node[i]);
    printf("the list16 node %dend addr is %x\n\n",i,(int)(list_16_node[i].addr));
    return (&list_16_node[0]);
}

list_node * thirtytwo_init(list_node **last)
{
    void *a = malloc(32*100);
    printf("the list32 start addr is %x\n",(int)a);
    int i;
    assert_param(a);
    for(i=0;i<100;i++)
    {
		char * trans = (char *)a + i*32;
        list_32_node[i].addr=(void*)trans;
        list_32_node[i].next_node=NULL;
        //printf("the list %d addr is %x\n",i,(int)(list_32_node[i].addr));
        if(i==99)
            break;
        list_32_node[i].next_node=&list_32_node[i+1];
    }
    *last=&(list_32_node[i]);
//    printf("%x++++++++++++++\n",(int)((*last)->addr));
    printf("the list32 node %dend addr is %x\n\n",i,(int)(list_32_node[i].addr));
    return (&list_32_node[0]);
}

init_ptr func_ptr[]=
{
    eight_init,
    sixteen_init,
    thirtytwo_init,
};

int mem_pool_init()
{
    int i;
    for(i=0;i<3;i++)
    {
        available[i]=(*func_ptr[i])(&(rear[i]));
    }
    for(i=0;i<3;i++)
    {
        manager_list[i].start_node=available[i];            
        manager_list[i].last_node=rear[i];
        manager_list[i].addr_start=(unsigned int)(available[i]->addr);
        manager_list[i].addr_end=(unsigned int)(manager_list[i].last_node->addr);
        printf("the manager_list[%d] \tstart addr is%x and end addr is %x\n\n",i,(unsigned int)(unsigned int)(manager_list[i].start_node->addr),(unsigned int)(manager_list[i].last_node->addr));
    }
	return 0;
}


void * mem_alloc(int num)
{
    int i;
    if(num>=1&&num<=8)
        i=0;
    else if(num>=9&&num<=16)
        i=1;
    else if(num>=17&&num<=32)
        i=2;
    else
        return (malloc(num));
    if(manager_list[i].start_node == NULL)
    {
        printf("this is no space in mempool!!");
        return (malloc(num));
    }
    
    list_node *p=manager_list[i].start_node;
    manager_list[i].start_node = manager_list[i].start_node->next_node;
    p->next_node=NULL;
    
    return (p->addr);
}


void mem_free(void *addr)
{
    if(addr==NULL)
        return ;
    int i;
    unsigned int num=(unsigned int)(addr);
    unsigned int start_addr,end_addr;
    for(i=0;i<3;i++)
    {
        start_addr=(unsigned int)available[i]->addr;
        end_addr=(unsigned int)rear[i]->addr;
        if(num>=start_addr&&num<=end_addr)
        break;
    }
//    printf("the free add is %x and the start addr is %x\n",num,start_addr);
    num=(num-start_addr)/(8*(i+1));
    if(num>400)
        return;
//    printf("the free node is %d\tand is nodelist[%d]\n",num,i);
    switch(i)
    {
        case 0:
//            printf("list_8_node[%d] the next is %x\n",num,list_8_node[num].next_node);
            list_8_node[num].next_node=manager_list[i].start_node;
            manager_list[i].start_node=&(list_8_node[num]);
            break;
        case 1:
//            printf("list_16_node[%d] the next is %x\n",num,list_16_node[num].next_node);
            list_16_node[num].next_node=manager_list[i].start_node;
            manager_list[i].start_node=&(list_16_node[num]);
            break;
        case 2:
//            printf("list_32_node[%d] the next is %x\n",num,list_32_node[num].next_node);
            list_32_node[num].next_node=manager_list[i].start_node;
            manager_list[i].start_node=&(list_32_node[num]);
            break;
        default:
            free(addr);
            break;
    }
}

int main()
{
    mem_pool_init();
    int i;
    void *a;
    void *b;
    /*
    for(i=0;i<1000;i++)
    {
        a=mem_alloc(5);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)a);
        i++;        
        b=mem_alloc(5);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)b);
        mem_free(a);
        mem_free(b);
    }

    for(i=0;i<1000;i++)
    {
        a=mem_alloc(18);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)a);
        i++;        
        b=mem_alloc(18);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)b);
        mem_free(a);
        mem_free(b);
    }
    */
    /*
    for(i=0;i<1000;i++)
    {
        a=mem_alloc(10);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)a);
        i++;        
        b=mem_alloc(10);
        printf("+++++++++++++++the %d addr is %x\n",i,(unsigned int)b);
        mem_free(a);
        mem_free(b);
    }
    */
    /*
    printf("the mem alloc is %x\n",(int)a);
    //mem_free(a);
    a=mem_alloc(9);
    //printf("the mem alloc 9 ++++++++++++++++++is %x\n",(int)a);
    mem_free(a);
    a=mem_alloc(18);
    printf("the mem alloc is %x\n",(int)a);
    a=mem_alloc(100);
    free(a);
    */
    clock_t t1,t2,ta,tb;
    t1=clock();
    int t;
    for(t=0;t<0x0ffffffa;t++)
    {
        a=mem_alloc(5);
        mem_free(a);
    }
    t2=clock();
    ta=t2-t1;
    t1=clock();
    for(t=0;t<0x0ffffffa;t++)
    {
        a=malloc(5);
        free(a);
    }
    t2=clock();
    tb=t2-t1;
    printf("the time %ld\t%ld\n",ta,tb);
	system("pause");
}