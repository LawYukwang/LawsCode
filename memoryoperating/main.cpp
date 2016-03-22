#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

//C内存管理
typedef unsigned char UINT8;

#define PAGE_SIZE     256//每页内存大小
#define PAGE_MAX      1024//页总数
#define MEM_FREE      0x01
#define MEM_USED      0x02

UINT8 DATA_PAGE_BASE_ADDRESS[PAGE_SIZE*PAGE_MAX];//DATA_PAGE_BASE_ADDRESS为操作内存基址
UINT8 DataPage_Manage[PAGE_MAX];//用来管理内存的使用情况

void InitMemory(void)
{
	for (int i=0; i<PAGE_MAX; i++)
	{
		DataPage_Manage[i] = MEM_FREE;
	}
}

//成功返回申请内存基址，否则返回NULL
void *GetMemory(void)
{
	for (int i=0; i<PAGE_MAX; i++)
	{
		if (DataPage_Manage[i] == MEM_FREE) 
		{
			DataPage_Manage[i] = MEM_USED;
			return ((UINT8 *)(DATA_PAGE_BASE_ADDRESS + i * PAGE_SIZE)); 
		} 
	}
	return NULL;
}

//成功返回1，否则返回0
UINT8 ReleaseMemory(UINT8 *pAddress)
{
	int ReleaseNum = 0;//内存页索引
	//判断地址是否有效
	if (pAddress < DATA_PAGE_BASE_ADDRESS) 
	{
		return 0;
	}
	//若释放地址不是操作内存基址
	if ((pAddress - DATA_PAGE_BASE_ADDRESS) != 0)
	{
		ReleaseNum = (pAddress - DATA_PAGE_BASE_ADDRESS) / PAGE_SIZE;
	}
	if (ReleaseNum > PAGE_MAX) 
	{
		return 0;
	}
	DataPage_Manage[ReleaseNum] = MEM_FREE;
	return 1;
}
//int main(void)
//{
//	char *pData[PAGE_MAX+1] = {NULL};
//	int i = 0;
//
//
//	InitMemory();
//	for (i=0; i<PAGE_MAX+1; i++)
//	{
//		pData[i] = (char *)GetMemory();
//		if (pData[i] != NULL)
//		{
//			printf("GET : succeed Page %d\n", i);
//		}
//		else
//		{
//			printf("GET : Failed\n");
//		}
//	}
//
//
//	getchar();
//
//
//	for (i=0; i<PAGE_MAX+1; i++)
//	{
//		if (1 == ReleaseMemory((UINT8 *)pData[i]))
//		{
//			printf("RELEASE : Success Page %d\n", i);
//		}
//		else
//		{
//			printf("RELEASE : Fail Page %d\n", i);
//		}
//	}
//
//	getchar();
//
//
//	for (i=0; i<PAGE_MAX+1; i++)
//	{
//		pData[i] = (char *)GetMemory();
//		if (pData[i] != NULL)
//		{
//			printf("GET : succeed Page %d\n", i);
//		}
//		else
//		{
//			printf("GET : Failed\n");
//		}
//	}
//
//
//	return 0;
//}

int main()
{
	int size;
	size = 8192 * 4000 * 3;
	int * a = (int *)malloc(size * 10);
	memset(a, 0, size * 10);
	int * b = (int *)calloc(10, size);
	free(a);
	free(b);
}

//内存之间的拷贝
void* memmove(void* dest, const void* src, size_t count) 
{
	assert((src != NULL)&&(dest !=NULL));
	char*tmp, *s;
	if(dest <=src)
	{         
		tmp = (char*) dest;         
		s = (char*) src;          
		while(count--)         
			*tmp++ = *s++;       
	}       
	else 
	{
		tmp = (char*) dest +count;
		s = (char*) src +count;
		while(count--)             
			*--tmp = *--s;             
	}
	return dest;
} 