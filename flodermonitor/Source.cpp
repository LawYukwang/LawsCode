#include <io.h>
#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>

using namespace std;

void getFiles( string path, vector<string>& files );
BOOL IsRoot(LPCTSTR lpszPath);
void FindInAll(LPCTSTR lpszPath);
void MonitorFolderChange(wstring file_path);


int main()
{
	char * filePath = "D:/QQMSN4LAW/diary"; 
	vector<string> files;  

	////��ȡ��·���µ������ļ�  
	getFiles(filePath, files );  

	char str[30];  
	int size = files.size();  
	for (int i = 0;i < size;i++)  
	{  
		cout<<files[i].c_str()<<endl;  
	}  
	FindInAll(filePath); 
}

void getFiles( string path, vector<string>& files )  
{  
	//�ļ����  
	long   hFile   =   0;  
	//�ļ���Ϣ  
	struct _finddata_t fileinfo;  
	string p;  
	if((hFile = _findfirst(p.assign(path).append("\\*").c_str(),&fileinfo)) != -1)  
	{  
		do  
		{  
			//�����Ŀ¼,����֮  
			//�������,�����б�  
			if((fileinfo.attrib & _A_SUBDIR))  
			{  
				if(strcmp(fileinfo.name,".") != 0  &&  strcmp(fileinfo.name,"..") != 0)  
					getFiles( p.assign(path).append("\\").append(fileinfo.name), files );  
			}  
			else  
			{  
				files.push_back(p.assign(path).append("\\").append(fileinfo.name) );  
			}  
		}while(_findnext(hFile, &fileinfo)  == 0);  
		_findclose(hFile);  
	}  
} 

//����win api
BOOL IsRoot(LPCTSTR lpszPath)
{
    TCHAR szRoot[4];
    wsprintf(szRoot, "%c:\\", lpszPath[0]);
    return (lstrcmp(szRoot, lpszPath) == 0);
}

void FindInAll(LPCTSTR lpszPath)
{
    TCHAR szFind[MAX_PATH];
    lstrcpy(szFind, lpszPath);
    if (!IsRoot(szFind))
    lstrcat(szFind, "\\");
    lstrcat(szFind, "*.*"); // �������ļ�
    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFile(szFind, &wfd);
    if (hFind == INVALID_HANDLE_VALUE) // ���û���ҵ������ʧ��
    return;
  
    do
    {
        if (wfd.cFileName[0] == '.')
            continue; // ����������Ŀ¼
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            TCHAR szFile[MAX_PATH];
            if (IsRoot(lpszPath))
                wsprintf(szFile, "%s%s", lpszPath, wfd.cFileName);
            else
                wsprintf(szFile, "%s\\%s", lpszPath, wfd.cFileName);
            FindInAll(szFile); // ����ҵ�����Ŀ¼��������Ŀ¼���еݹ�
        }
        else
        {
            TCHAR szFile[MAX_PATH];
            if (IsRoot(lpszPath))
                wsprintf(szFile, "%s%s", lpszPath, wfd.cFileName);
            else
                wsprintf(szFile, "%s\\%s", lpszPath, wfd.cFileName);
            printf("%s\n",szFile);
            // ���ļ����в���
        }
    } while (FindNextFile(hFind, &wfd));
    FindClose(hFind); // �رղ��Ҿ��
}

void MonitorFolderChange(wstring file_path)  
{  
    const int buf_size = 1024;
    TCHAR buf[buf_size];

    DWORD buffChangeSize;
    HANDLE hDir;
	// ���ض�file_path�Ŀ��ƾ��
	hDir = CreateFile(file_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING,     
        FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        DWORD dwErrorCode;
        dwErrorCode = GetLastError();
        CloseHandle(hDir);
        exit(0);
    }
    while(true)
    {
         //���û�м�����ʲô�仯����ֱ�ӷ��أ������˳�����������Ҫʵʱ�������Կ���һ���߳������ԡ�  
        if(ReadDirectoryChangesW(
			hDir, 
			&buf, 
			buf_size, 
			FALSE, 
			FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES, 
			&buffChangeSize, 
			NULL,
			NULL))    
        {
			//�������任��Ĳ���
            FILE_NOTIFY_INFORMATION * pfiNotifyInfo = (FILE_NOTIFY_INFORMATION*)buf;    
            DWORD dwNextEntryOffset;
			dwNextEntryOffset = pfiNotifyInfo->NextEntryOffset;    
			DWORD dwAction = pfiNotifyInfo->Action;     
			DWORD dwFileNameLength = pfiNotifyInfo->FileNameLength;    
			wstring file_name =pfiNotifyInfo->FileName;  
			int n =file_name.find_last_of('.');  
			if(n>=0)  
			{  
				file_name = file_name.substr(0,n+4);  
				wstring changepath = file_path;  
				changepath.append(L"\\").append(file_name);  
				//char *char_path =ConvertUnicodeToUtf8(changepath.c_str());  
				cout<<"�޸ĵ��ļ�·�� path:"<<changepath.c_str()<<endl;  
				//free(char_path);  
			}   
		}    
        else    
        {    
            printf("Moniter Failed!\n");    
        }       
    }    
        
    CloseHandle(hDir);   
}  