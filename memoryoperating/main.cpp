#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

//C�ڴ����
typedef unsigned char UINT8;

#define PAGE_SIZE     256//ÿҳ�ڴ��С
#define PAGE_MAX      1024//ҳ����
#define MEM_FREE      0x01
#define MEM_USED      0x02

UINT8 DATA_PAGE_BASE_ADDRESS[PAGE_SIZE*PAGE_MAX];//DATA_PAGE_BASE_ADDRESSΪ�����ڴ��ַ
UINT8 DataPage_Manage[PAGE_MAX];//���������ڴ��ʹ�����

void InitMemory(void)
{
	for (int i=0; i<PAGE_MAX; i++)
	{
		DataPage_Manage[i] = MEM_FREE;
	}
}

//�ɹ����������ڴ��ַ�����򷵻�NULL
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

//�ɹ�����1�����򷵻�0
UINT8 ReleaseMemory(UINT8 *pAddress)
{
	int ReleaseNum = 0;//�ڴ�ҳ����
	//�жϵ�ַ�Ƿ���Ч
	if (pAddress < DATA_PAGE_BASE_ADDRESS) 
	{
		return 0;
	}
	//���ͷŵ�ַ���ǲ����ڴ��ַ
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

//�ڴ�֮��Ŀ���
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