#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <tchar.h>
#include <ctime>
#include <string.h>

using namespace std;

//ThreadParameter结构体的定义
/*
*整合参数为一个结构体传给子线程的原因在于创建线程时
*指定的线程入口函数只接受一个LPVOID类型的指针，具体内容可以参考msdn，
*实际上向子线程传递参数还有一种方法，全局变量，
*例如后面程序中的WriteLog就是一个文件输入流对象，我就是在程序首部定义的全局变量。
*/
typedef struct ThreadParameter 
{
	LPTSTR in_directory;//监控的路径
	FILE_NOTIFY_INFORMATION *in_out_notification;//存储监控函数返回信息地址
	DWORD in_MemorySize;//传递存储返回信息的内存的字节数
	DWORD *in_out_BytesReturned;//存储监控函数返回信息的字节数
	DWORD *in_out_version;//返回版本信息
	FILE_NOTIFY_INFORMATION *temp_notification;//备用的一个参数
}ThreadParameter;
DWORD WINAPI WatchChanges(LPVOID lpParameter);
time_t ChangeTime;
int edit_flag;
fstream WriteLog;

int main()
{
	WriteLog.open("wlog.txt", ios::out);
	//传递给WatchChanges函数的参数,这部分代码截自主函数
	
	TCHAR *dir = _T("E:\\Action");
	LPTSTR Directory = dir;
	char notify[1024];
	memset(notify,'\0',1024);
	FILE_NOTIFY_INFORMATION *Notification=(FILE_NOTIFY_INFORMATION *)notify;
	FILE_NOTIFY_INFORMATION *TempNotification=NULL;
	DWORD BytesReturned=0;
	DWORD version=0;

	//整合传给子线程的参数，该结构体的定义参考上面的定义
	ThreadParameter ParameterToThread={Directory,Notification,sizeof(notify),&BytesReturned,&version,TempNotification};

	//创建一个线程专门用于监控文件变化
	HANDLE TrheadWatch=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)WatchChanges,&ParameterToThread,0,NULL);
	WaitForSingleObject(TrheadWatch, INFINITE);  
	CloseHandle(TrheadWatch);
	Sleep(10000);
	return 0;
}

//  函数: WatchChanges(LPVOID lpParameter)
//
//  目的: 监控目录的程序
//
//  注释:主函数创建线程时制定了这个函数的入口
//		 届时该子程序将自动启动执行。
//  备注：因为代码不全，看下面的代码时，主要参考红色的字体部分

DWORD WINAPI WatchChanges(LPVOID lpParameter)//返回版本信息
{
	ThreadParameter *parameter=(ThreadParameter*)lpParameter;
	LPCTSTR WatchDirectory=parameter->in_directory;//


	//创建一个目录句柄
	HANDLE handle_directory=CreateFile(WatchDirectory,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(handle_directory==INVALID_HANDLE_VALUE)
	{
		DWORD ERROR_DIR=GetLastError();
		MessageBox(NULL,TEXT("打开目录错误!"),TEXT("文件监控"),0);
	}


	BOOL watch_state;


	while (TRUE)
	{
		watch_state=ReadDirectoryChangesW(handle_directory,
			(LPVOID)parameter->in_out_notification,
			parameter->in_MemorySize,
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE,
			(LPDWORD)parameter->in_out_BytesReturned,
			NULL,
			NULL);
		//printf("########%d########\n",watch_state);//检测结果为1，表明函数已成功执行
		time(&ChangeTime);//记录修改时间


		if (GetLastError()==ERROR_INVALID_FUNCTION)
		{
			MessageBox(NULL,TEXT("系统不支持!"),TEXT("文件监控"),0);
		}
		else if(watch_state==0)
		{
			MessageBox(NULL,TEXT("监控失败!"),TEXT("文件监控"),0);
		}
		else if (GetLastError()==ERROR_NOTIFY_ENUM_DIR)
		{
			MessageBox(NULL,TEXT("内存溢出!"),TEXT("文件监控"),0);
		}
		else
		{

			//将宽字符类型的FileName变量转换成string，便于写入log文件，否则写不进去正确的文件名
			string file_name;
			DWORD length=WideCharToMultiByte(0,0,parameter->in_out_notification->FileName,-1,NULL,0,NULL,NULL);
			PSTR ps=new CHAR[length];
			if(length>=0)
			{
				WideCharToMultiByte(0,0,parameter->in_out_notification->FileName,-1,ps,length,NULL,NULL);
				file_name=string(ps);
				delete[] ps;
			}

			//这里主要就是检测返回的信息，需要注意FILE_NOTIFY_INFORMATION结构体的定义，以便正确调用返回信息

			if (parameter->in_out_notification->Action==FILE_ACTION_ADDED)
			{
				WriteLog<<ctime(&ChangeTime)<<"新增文件 : "<<file_name.c_str()<<"\n"<<flush;
			}
			if (parameter->in_out_notification->Action==FILE_ACTION_REMOVED)
			{
				WriteLog<<ctime(&ChangeTime)<<"删除文件 : "<<file_name.c_str()<<"\n"<<flush;
			}
			if (parameter->in_out_notification->Action==FILE_ACTION_MODIFIED)
			{
				edit_flag++;
				if(edit_flag==1)
					WriteLog<<ctime(&ChangeTime)<<"修改文件 : "<<file_name.c_str()<<"\n"<<flush;
				else if(edit_flag==2)
				{
					edit_flag=0;
					(*(parameter->in_out_version))--;
				}
				else
					return -1;//break;
			}


			//对于下面两种情况，Action本身也是文件名（可能是old_name也可能是new_name）
			if (parameter->in_out_notification->Action==FILE_ACTION_RENAMED_OLD_NAME)
			{
				WriteLog<<ctime(&ChangeTime)<<"重命名\""<<file_name.c_str()<<"\"文件\n"<<flush;
			}
			if (parameter->in_out_notification->Action==FILE_ACTION_RENAMED_NEW_NAME)
			{
				WriteLog<<ctime(&ChangeTime)<<"重命名\""<<file_name.c_str()<<"\"文件为\""<<parameter->in_out_notification->Action<<"\"\n"<<flush;
			}
			(*(parameter->in_out_version))++;
			memset(parameter->in_out_notification,'\0',1024);
		}
		Sleep(500);
	}
	return 0;
}