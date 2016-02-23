#include <Windows.h>
#include <stdio.h>
#include <string>

using namespace std;
void main()
{
	char notify[1024];  
	memset(notify,'\0',1024);  
	FILE_NOTIFY_INFORMATION *in_out_notification=(FILE_NOTIFY_INFORMATION *)notify; 
	DWORD in_MemorySize = 1024;
	DWORD *in_out_BytesReturned = new DWORD;	  
	HANDLE handle_directory=CreateFile("D:\\", FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
	if(handle_directory==INVALID_HANDLE_VALUE)  
	{
		DWORD ERROR_DIR=GetLastError();
		MessageBox("打开文件目录失败!!");
	}
	BOOL watch_state;
	int num = 0;
	string str = "";
	LPOVERLAPPED lpOverlapped = new OVERLAPPED;
	LPDWORD lpNumberOfBytesTransferred = new DWORD;
	while (TRUE)
	{
		watch_state = ReadDirectoryChangesW(handle_directory,(LPVOID)in_out_notification,  
			in_MemorySize,  
			TRUE,  
			FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE,
			(LPDWORD)in_out_BytesReturned,  
			NULL,  
			NULL);
		string file_name;  
		DWORD length=WideCharToMultiByte(0,0,in_out_notification->FileName,-1,NULL,0,NULL,NULL);  
		PSTR ps=new CHAR[length];  
		if(length>=0)  
		{  
			WideCharToMultiByte(0,0,in_out_notification->FileName,-1,ps,length,NULL,NULL);  
			file_name=string(ps);  
			delete[] ps;  
		} 
		if (GetLastError()==ERROR_INVALID_FUNCTION)  
		{  
			MessageBox("系统不支持!");  
		}  
		else if(watch_state==0)  
		{  
			MessageBox("监控失败!");  
		}  
		else if (GetLastError()==ERROR_NOTIFY_ENUM_DIR)  
		{  
			MessageBox("内存溢出!");  
		}
		memset(notify,'\0',1024);
	}
}

void WINAPI CheckAddedFile( LPDIRECTORY_INFO lpdi, PFILE_NOTIFY_INFORMATION lpfni) {
    TCHAR      szFullPathName[MAX_PATH];
    TCHAR      szFileName[MAX_PATH];

    memcpy( szFileName, lpfni->FileName ,lpfni->FileNameLength);
    szFileName[lpfni->FileNameLength/sizeof(TCHAR)]=0;
    lstrcpy( szFullPathName, lpdi->lpszDirName );
    lstrcat( szFullPathName, L"\\" );
    lstrcat( szFullPathName, szFileName );
    wprintf( L"%s\n", szFullPathName );
    wprintf( L"%s added\n",szFileName);//Zz renamed\n",szFileName);
}
/**********************************************************************
   HandleDirectoryChanges()
   Purpose:
      This function receives notification of directory changes and
      calls CheckChangedFile() to display the actual changes. After
      notification and processing, this function calls
      ReadDirectoryChangesW to reestablish the watch.
   Parameters:
      HANDLE hCompPort - Handle for completion port
   Return Value:
      None
   Comments:
********************************************************************/
void WINAPI HandleDirectoryChange( DWORD dwCompletionPort ) {
    DWORD numBytes;
    DWORD cbOffset;
    LPDIRECTORY_INFO di;
    LPOVERLAPPED lpOverlapped;
    PFILE_NOTIFY_INFORMATION fni;
    BOOL r;

    do {
        // Retrieve the directory info for this directory
        // through the completion key
        GetQueuedCompletionStatus( (HANDLE) dwCompletionPort,
                                   &numBytes,
                                   (LPDWORD) &di,
                                   &lpOverlapped,
                                   INFINITE);
        if ( di ) {
            fni = (PFILE_NOTIFY_INFORMATION)di->lpBuffer;
            do {
                cbOffset = fni->NextEntryOffset;

//              if( fni->Action == FILE_ACTION_MODIFIED )
//                  CheckChangedFile( di, fni );
                if( fni->Action == FILE_ACTION_ADDED) //Zz FILE_ACTION_RENAMED_NEW_NAME)
                    CheckAddedFile( di, fni );

                fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE) fni + cbOffset);

            } while( cbOffset );

            // Reissue the watch command
            r=ReadDirectoryChangesW( di->hDir,
                                   di->lpBuffer,
                                   MAX_BUFFER,
                                   FALSE,
                                   FILE_NOTIFY_CHANGE_FILE_NAME,//|FILE_NOTIFY_CHANGE_LAST_WRITE,
                                   &di->dwBufLength,
                                   &di->Overlapped,
                                   NULL);
            if (0==r) {
                wprintf( L"ReadDirectoryChangesW error! GetLastError()==%d\n",GetLastError());
                no_loop=1;break;//
            }
        } else {
            no_loop=1;
        }
    } while( di );
}

