#include <iostream>
#include <list>
#include <Windows.h>

using namespace std;

//
#define MAX_DATA_SIZE    80000000
#define NODE_COUNT       10

#pragma pack(push, 1)
typedef struct 
{
	int n;
	int width;
	int height;
	int dataSize;
	unsigned char* pData;
} list_node;
#pragma pack(pop)

//mem struct
typedef struct memBlock
{
	unsigned char * memPointer;
	bool isUse;
	memBlock * next;
}memBlockNode;

//mem head
memBlockNode memNode[NODE_COUNT];
memBlockNode * memHead;

void memInit()
{
	unsigned char * memStart = new unsigned char[MAX_DATA_SIZE * NODE_COUNT];

	for (int i = 1; i < NODE_COUNT ; i++)
	{
		memNode[i].memPointer = memStart + i * MAX_DATA_SIZE;
		memNode[i].isUse = false;
		if (i == NODE_COUNT - 1)
		{
			memNode[i].next       = NULL;
			break;
		}
		memNode[i].next = &memNode[i+1];
	}
	memHead = &memNode[0];
}

unsigned char * newMem()
{
	if(!memHead->isUse)
		return memHead->memPointer;
	else
		return NULL;
}

void deleteMem()
{}
//global
list <list_node *> unuseList;
list <list_node *> usingList;
unsigned char* dataMem;
HANDLE hEvent = INVALID_HANDLE_VALUE;
CRITICAL_SECTION seclock;
CRITICAL_SECTION unseclock;

void memInit(int num)
{
	dataMem = new unsigned char[MAX_DATA_SIZE * num];
	for(int i = 0; i < num; i++)
	{
		list_node * temp = new list_node();
		temp->pData    = new unsigned char[MAX_DATA_SIZE];
		temp->n        = 0;
		temp->width    = 0;
		temp->height   = 0;
		temp->dataSize = 0;
		unuseList.push_back(temp);
	}
}

void reuse(list_node * node)
{
	memset(node->pData,'\0', MAX_DATA_SIZE);
	node->n        = 0;
	node->width    = 0;
	node->height   = 0;
	node->dataSize = 0;
	EnterCriticalSection(&unseclock);
	unuseList.push_back(node);
	LeaveCriticalSection(&unseclock);

}

DWORD WINAPI copyFunc(LPVOID  args)
{
	
	for(int i = 1;i > 0;i++)
	{	
		EnterCriticalSection(&unseclock);
		printf("size:%d\n",unuseList.size());
		if(!unuseList.size())
		{
			//LeaveCriticalSection(&unseclock);
			return 0;
		}
		list_node * temp = unuseList.front();
		unuseList.pop_front();
		LeaveCriticalSection(&unseclock);

		//processing
		memset(temp->pData,i,MAX_DATA_SIZE);
		temp->n        = i;
		temp->width    = i;
		temp->height   = i;
		temp->dataSize = i;
		EnterCriticalSection(&seclock);
		usingList.push_back(temp);
		LeaveCriticalSection(&seclock);
		printf("%d\n",i);
		SetEvent(hEvent);
		Sleep(1000);	
	}
	return 0;
}

int main()
{
	//static int i = 1;
	memInit(5);
	InitializeCriticalSection(&seclock);
	InitializeCriticalSection(&unseclock);
	hEvent = CreateEvent(NULL, FALSE, FALSE, LPCSTR("SECEVT"));
	HANDLE hThread = CreateThread(NULL,0,copyFunc, NULL,0,NULL);
	while (1)
	{
		WaitForSingleObject(hEvent, INFINITE);
		EnterCriticalSection(&seclock);
		list_node * processingNode = usingList.front();
		usingList.pop_front();
		LeaveCriticalSection(&seclock);
		Sleep(1200);
		printf("processed:%d\n",processingNode->n);
		reuse(processingNode);
		if(usingList.size())
			SetEvent(hEvent);
	}
	CloseHandle(hEvent);
	CloseHandle(hThread);
	DeleteCriticalSection(&seclock);
	DeleteCriticalSection(&unseclock);
}