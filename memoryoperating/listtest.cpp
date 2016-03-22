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

//global
list <list_node *> unuseList;
list <list_node *> usingList;
unsigned char* dataMem;
HANDLE hEvent = INVALID_HANDLE_VALUE;
CRITICAL_SECTION seclock;

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
	unuseList.push_back(node);
}

DWORD WINAPI copyFunc(LPVOID  args)
{
	for(int i = 1;unuseList.size();i++)
	{
		list_node * temp = unuseList.front();
		unuseList.pop_front();
		//processing
		memset(temp->pData,i,MAX_DATA_SIZE);
		temp->n        = i;
		temp->width    = i;
		temp->height   = i;
		temp->dataSize = i;
		usingList.push_back(temp);
		SetEvent(hEvent);
		Sleep(1000);
		printf("%d\n",i);
	}
	return 0;
}

int main()
{
	//static int i = 1;
	memInit(5);
	InitializeCriticalSection(&seclock);
	hEvent = CreateEvent(NULL, FALSE, FALSE, LPCSTR("SECEVT"));
	HANDLE hThread = CreateThread(NULL,0,copyFunc, NULL,0,NULL);
	while (1)
	{
		WaitForSingleObject(hEvent, INFINITE);
		list_node * processingNode = usingList.front();
		usingList.pop_front();
		Sleep(1000);
		reuse(processingNode);
	}
	CloseHandle(hEvent);
	CloseHandle(hThread);
	DeleteCriticalSection(&seclock);

}