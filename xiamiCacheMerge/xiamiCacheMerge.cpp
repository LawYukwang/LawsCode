// xiamiCacheMerge.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include <iomanip>
#include <list>
#include <string>

using namespace std;

list<string> g_FileNameLst;                // 保存文件名链表
CRITICAL_SECTION g_FileLstLock;            // 文件名链表锁
HANDLE g_NewFileEvent;                     // 通告事件
TCHAR* g_DirPath;                          // 监控文件夹
HANDLE g_WatchingThread;                   // 监控线程
bool g_ThreadExit;                         // 线程退出

static DWORD WINAPI FloderMonitor(LPVOID  args);

int _tmain(int argc, _TCHAR* argv[])
{
	//g_DirPath = _T("C:\\Users\\Administrator\\AppData\\Roaming\\XIAMI-MUSIC\\Cache\\");
	//g_NewFileEvent = ::CreateEvent(NULL, FALSE, FALSE, _T("NEW_FILE"));
	//::InitializeCriticalSection(&g_FileLstLock);
	//g_ThreadExit = false;
	//g_WatchingThread = ::CreateThread(NULL, 0, FloderMonitor, NULL, 0, NULL);
	//if (g_WatchingThread != NULL)
	//{
	//	CloseHandle(g_WatchingThread);
	//}
	//while(true)
	//{
	//	::WaitForSingleObject(g_NewFileEvent,INFINITE);
	//	if(g_FileNameLst.size()<=0)
	//	{
	//		continue;
	//	}
	//	::EnterCriticalSection(&g_FileLstLock);
	//	string processFileName = g_FileNameLst.front();
	//	g_FileNameLst.pop_front();
	//	::LeaveCriticalSection(&g_FileLstLock);
	//	cout<<processFileName.c_str()<<endl;
	//	if(g_FileNameLst.size()>0)
	//	{
	//		::SetEvent(g_NewFileEvent);
	//	}
	//}
	//if(g_NewFileEvent != NULL)
	//{
	//	CloseHandle(g_NewFileEvent);
	//}
	//::DeleteCriticalSection(&g_FileLstLock);
	char* g_FilePath = "D:\\QQMSN4LAW\\桌面\\ximi\\f_0013f9";
	FILE* partFile = fopen(g_FilePath, "rb");
	if (partFile == NULL)
	{
		return -1;
	}
	unsigned char* partBin = new unsigned char[1024];
	fread(partBin, 1, 1024, partFile);


	return 0;
}

static DWORD WINAPI FloderMonitor(LPVOID  args)
{

	DWORD cbBytes;
	char file_name[MAX_PATH]; //设置文件名
	char notify[1024];
	int count = 0; //文件数量

	//生成监控目录的句柄
	HANDLE dirHandle = CreateFile(g_DirPath,
		GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(dirHandle == INVALID_HANDLE_VALUE) //若目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
	{
		GetLastError();
	}
	memset(notify,0,strlen(notify));
	FILE_NOTIFY_INFORMATION *pnotify = (FILE_NOTIFY_INFORMATION*)notify; 
	while(!g_ThreadExit)
	{
		//使用ReadDirectoryChangesW监控目录
		if(ReadDirectoryChangesW(dirHandle, &notify, 1024, true,
			//FILE_NOTIFY_CHANGE_FILE_NAME 
			//| FILE_NOTIFY_CHANGE_DIR_NAME 
			//|FILE_NOTIFY_CHANGE_CREATION
			//|FILE_NOTIFY_CHANGE_LAST_WRITE,
			FILE_NOTIFY_CHANGE_SIZE,
			&cbBytes,NULL,NULL))
		{
			//转换文件名为多字节字符串
			if(pnotify->FileName)
			{
				memset(file_name,0,strlen(file_name));
				WideCharToMultiByte(CP_ACP,0,pnotify->FileName,pnotify->FileNameLength/2,file_name,99,NULL,NULL);
			}
			string _fileName = file_name;
			//设置类型过滤器,监听文件创建、更改、删除、重命名等
			switch(pnotify->Action)
			{
			case FILE_ACTION_ADDED:
				break;
			case FILE_ACTION_MODIFIED:
				::EnterCriticalSection(&g_FileLstLock);
				g_FileNameLst.push_back(_fileName);
				::LeaveCriticalSection(&g_FileLstLock);
				::SetEvent(g_NewFileEvent);
				break;
			case FILE_ACTION_REMOVED:
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				break;
			default:
				break;
			}
		}
	}
	CloseHandle(dirHandle);
	return 0;
}
