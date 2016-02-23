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
	char file_name[MAX_PATH]; //�����ļ���
	char file_name2[MAX_PATH]; //�����ļ��������������
	char notify[1024];
	int count = 0; //�ļ�����������ͬʱ������ɾ������ļ������Խ��и��Ѻõ���ʾ��
	TCHAR *dir = _T("E:\\Action");
	HANDLE dirHandle = CreateFile(dir,
		GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);
	if(dirHandle == INVALID_HANDLE_VALUE) //�������ض����Ŀ���ļ�ϵͳ��֧�ָò���������ʧ�ܣ�ͬʱ����GetLastError()����ERROR_INVALID_FUNCTION
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
			//ת���ļ���Ϊ���ֽ��ַ���
			if(pnotify->FileName)
			{
				memset(file_name,0,strlen(file_name));
				WideCharToMultiByte(CP_ACP,0,pnotify->FileName,pnotify->FileNameLength/2,file_name,99,NULL,NULL);
			}
			//��ȡ���������ļ���
			if(pnotify->NextEntryOffset !=0 && (pnotify->FileNameLength > 0 && pnotify->FileNameLength < MAX_PATH)) 
			{ 
				PFILE_NOTIFY_INFORMATION p = (PFILE_NOTIFY_INFORMATION)((char*)pnotify+pnotify->NextEntryOffset); 
				memset(file_name2,0,sizeof(file_name2) ); 
				WideCharToMultiByte(CP_ACP,0,p->FileName,p->FileNameLength/2,file_name2,99,NULL,NULL ); 
			} 
			//�������͹�����,�����ļ����������ġ�ɾ������������
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

//ע��������裩��
//1��ͨ��CreateFile��ȡҪ��ص�Ŀ¼�����
//2��ͨ��ReadDirectoryChangesW����⵽�ļ�ϵͳ�ı仯�����ܹ�������ϸ���ļ��䶯����Ϣ�������ܹ�ѡ��
//��ʹ��ͬ����ʽ��⻹���첽��ʽ��⡣
//3��ͨ��Action�������͹����������ݹ����������ã�ReadDirectoryChangesW�������Լ���ļ����ı䡢�ļ�
//���Ըı䡢�ļ���С�ı䡢�ļ����ݱ���д���ļ���ɾ���ȶ������͵ı仯��