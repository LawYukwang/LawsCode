#include <iomanip>
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <string.h>

using namespace std;
#define MAX_PATH 1024

void fileWatcher();

int main()
{
	fileWatcher();
	return 0;
}

void fileWatcher()
{
	DWORD cbBytes;
	char file_name[MAX_PATH]; //设置文件名
	char file_name2[MAX_PATH]; //设置文件重命名后的名字
	char notify[1024];
	int count = 0; //文件数量。可能同时拷贝、删除多个文件，可以进行更友好的提示。
	TCHAR *dir = _T("E:\\Action");
	HANDLE dirHandle = CreateFile(dir,
		GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(dirHandle == INVALID_HANDLE_VALUE) //若网络重定向或目标文件系统不支持该操作，函数失败，同时调用GetLastError()返回ERROR_INVALID_FUNCTION
	{
		cout<<"error"+GetLastError()<<endl;
	}
	memset(notify,0,strlen(notify));
	FILE_NOTIFY_INFORMATION *pnotify = (FILE_NOTIFY_INFORMATION*)notify; 
	cout<<"start...."<<endl;   
	while(true)
	{
		if(ReadDirectoryChangesW(dirHandle,
			&notify,
			1024,
			true,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
			//| FILE_NOTIFY_CHANGE_CREATION
			//| FILE_NOTIFY_CHANGE_LAST_WRITE
			| FILE_NOTIFY_CHANGE_SIZE,
			&cbBytes,
			NULL,
			NULL))
		{
			//转换文件名为多字节字符串
			if(pnotify->FileName)
			{
				memset(file_name,0,strlen(file_name));
				WideCharToMultiByte(CP_ACP,0,pnotify->FileName,pnotify->FileNameLength/2,file_name,99,NULL,NULL);
			}
			//获取重命名的文件名
			if(pnotify->NextEntryOffset !=0 && (pnotify->FileNameLength > 0 && pnotify->FileNameLength < MAX_PATH)) 
			{ 
				PFILE_NOTIFY_INFORMATION p = (PFILE_NOTIFY_INFORMATION)((char*)pnotify+pnotify->NextEntryOffset); 
				memset(file_name2,0,sizeof(file_name2) ); 
				WideCharToMultiByte(CP_ACP,0,p->FileName,p->FileNameLength/2,file_name2,99,NULL,NULL ); 
			} 
			//设置类型过滤器,监听文件创建、更改、删除、重命名等
			switch(pnotify->Action)
			{
			case FILE_ACTION_ADDED:
				count++;
				cout<<count<<setw(5)<<"file add:"<<setw(5)<<file_name<<endl;
				break;
			case FILE_ACTION_MODIFIED:
				cout<<"file modified:"<<setw(5)<<file_name<<endl;
				break;
			case FILE_ACTION_REMOVED:
				count++;
				cout<<count<<setw(5)<<"file removed:"<<setw(5)<<file_name<<endl;
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				cout<<"file renamed:"<<setw(5)<<file_name<<"->"<<file_name2<<endl;
				break;
			default:
				cout<<"unknow command!"<<endl;
			}
		}
	}
	CloseHandle(dirHandle);
}

//注意事项（步骤）：
//1、通过CreateFile获取要监控的目录句柄。
//2、通过ReadDirectoryChangesW来监测到文件系统的变化，还能够返回详细的文件变动的信息，并且能够选择
//是使用同步方式检测还是异步方式监测。
//3、通过Action设置类型过滤器，根据过滤器的设置，ReadDirectoryChangesW函数可以监控文件名改变、文件
//属性改变、文件大小改变、文件内容被改写、文件被删除等多种类型的变化。