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

//监视文件修改(采用完成端口和ReadDirectoryChangesW同时在一个线程中监视多个目录，并且能够判断文件是否完全复制完毕)

#define STRICT
#define WINVER               0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE           0x0501
#define _RICHEDIT_VER 0x0200
#define _WIN32_DCOM
#include <CTL/CTL_BASE.HPP>
class P2PFileShare
{
	typedef struct
	{
		OVERLAPPED   ov;
		BYTE           buff[1024];
		LPTSTR              path;
		DWORD             flag;
		HANDLE            handle;
	}PATH_OV, *LPPATH_OV;
	typedef struct
	{
		LPTSTR       name;     // 文件名称
		DWORD      time;       // 通知时间
	}FILE_NOTIFY;
public:
	P2PFileShare()
		: mh_IOCP(NULL)
		, mn_OVPtr(0)
		, mp_OVPtr(NULL)
		, mn_Notify(0)
		, mp_Notify(NULL)
	{
	}
	virtual~P2PFileShare()
	{
		Close(TRUE);
	}
private:
	// 创建工作线程
	HRESULT                         _CreateWorkerThread();
	// 工作线程
#ifndef _WIN32_WCE
	static UINT WINAPI _WorkerThreadProc(IN LPVOID pData);
#else
	static DWORD WINAPI   _WorkerThreadProc(IN LPVOID pData);
#endif     // #ifndef _WIN32_WCE
	HRESULT           _WorkerThreadProc();
public:
	HRESULT           Start();
	VRESULT           Close(IN CONST BOOL bWait = FALSE);
public:
	// 监视指定目录
	HRESULT MonitorPath(IN LPCTSTR sFileName);
	// 文件变化通知
	LPTSTR       GetNotify();
private:
	HANDLE                   mh_IOCP;
	MLONG                     mn_OVPtr;
	LPPATH_OV*           mp_OVPtr;
	MLONG                     mn_Notify;
	FILE_NOTIFY* mp_Notify;
public:
	INLINE VRESULT EnterLock() {mo_cs.EnterLock();}
	INLINE VRESULT LeaveLock() {mo_cs.LeaveLock();}
private:
	MTCSObject              mo_cs;
};
// 创建工作线程(根据 CPU 的数量，创建相应数量的工作线程)
HRESULT P2PFileShare::_CreateWorkerThread()
{
	HRESULT hr = E_FAIL;
	HANDLE hThread;
#ifndef _WIN32_WCE
	if((hThread = (HANDLE)_beginthreadex(NULL, 0
		, _WorkerThreadProc
		, (LPVOID)this, 0, NULL)) == 0)
	{
		return _doserrno;
	}
#else
	if((hThread = (HANDLE)::CreateThread(NULL, 0
		, _WorkerThreadProc
		, (LPVOID)this, 0, &NULL)) == 0)
	{
		return ::GetLastError();
	}
#endif
	::CloseHandle(hThread);     // 关闭句柄避免资源泄漏
	hr = S_OK;
	return hr;
}
// 工作线程
#ifndef _WIN32_WCE
UINT P2PFileShare::_WorkerThreadProc(IN LPVOID pData)
#else
DWORD P2PFileShare::_WorkerThreadProc(IN LPVOID pData)
#endif     // #ifndef _WIN32_WCE
{
	((P2PFileShare*)pData)->_WorkerThreadProc();
#ifndef _WIN32_WCE
	_endthreadex(0);
#else
	ExitThread(0);
#endif
	return 0;
}
// 数据处理线程函数
HRESULT P2PFileShare::_WorkerThreadProc()
{
	// 注意: 调用 GetQueuedCompletionStatus 的线程都将被放到完成端口的等待线程队列中
	// 完成操作循环
	BOOL   bSucceed;
	DWORD      dwBytes;
	LPDWORD         pCT;
	PATH_OV* pOV;
	for(;;)
	{
		bSucceed = ::GetQueuedCompletionStatus(mh_IOCP
			, &dwBytes
			, (LPDWORD)&pCT
			, (LPOVERLAPPED*)&pOV
			, INFINITE
			);
		if(bSucceed)
		{
			if(NULL == pOV) break;          // 退出工作线程
			FILE_NOTIFY_INFORMATION * pfiNotifyInfo = (FILE_NOTIFY_INFORMATION*)pOV->buff;
			DWORD dwNextEntryOffset;
			TCHAR sFileName[1024];
			do
			{
				dwNextEntryOffset = pfiNotifyInfo->NextEntryOffset;
				DWORD dwAction = pfiNotifyInfo->Action;
				DWORD dwFileNameLength = pfiNotifyInfo->FileNameLength;
				CPY_W2T(sFileName, pfiNotifyInfo->FileName, dwFileNameLength/sizeof(WCHAR));
				switch(dwAction)
				{
				case FILE_ACTION_REMOVED:         // 文件删除
					{
						LPTSTR sFullName = new TCHAR[LPTSTRLen(pOV->path) + LPTSTRLen(sFileName) + 1];
						if(NULL != sFullName)
						{
							LPTSTRCpy(sFullName, pOV->path);
							LPTSTRCat(sFullName, sFileName);
							LPTSTRPrintf(__T("Del %s"n"), sFullName);
								delete[] sFullName;
						}
					}
					break;
				case FILE_ACTION_ADDED:                     // 文件替换
					{
						// 替换文件时只会触发 FILE_ACTION_ADDED， 因此需要手工触发 FILE_ACTION_MODIFIED
						LPTSTRPrintf(__T("Add %s"n"), sFileName);
					}
				case FILE_ACTION_MODIFIED:         // 文件修改
					{
						// 测试文件是否关闭
						LPTSTR sFullName = new TCHAR[LPTSTRLen(pOV->path) + LPTSTRLen(sFileName) + 1];
						if(NULL != sFullName)
						{
							LPTSTRCpy(sFullName, pOV->path);
							LPTSTRCat(sFullName, sFileName);
							HANDLE hFile = ::CreateFile(sFullName
								, GENERIC_WRITE
								, FILE_SHARE_WRITE
								, NULL
								, OPEN_EXISTING
								, FILE_ATTRIBUTE_NORMAL
								, NULL
								);
							if(INVALID_HANDLE_VALUE == hFile)
							{
								HRESULT hr = ::GetLastError();
								LPTSTRPrintf(__T("Locked %s %d"n"), sFileName, hr);
							}
							else
							{
								::CloseHandle(hFile);
								LPTSTRPrintf(__T("Modify %s"n"), sFileName);
									LONG i;
								EnterLock();
								for(i=0;i<mn_Notify;i++)
								{
									if(LPTSTRCompare(mp_Notify[i].name, sFullName) == 0)
									{
										mp_Notify[i].time = ::GetTickCount();
										break;
									}
								}
								if(i >= mn_Notify)
								{
									FILE_NOTIFY* pNotify = new FILE_NOTIFY[mn_Notify + 1];
									if(NULL != pNotify)
									{
										if(mn_Notify > 0)
										{
											::CopyMemory(pNotify, mp_Notify, sizeof(FILE_NOTIFY)*mn_Notify);
											delete[] mp_Notify;
										}
										pNotify[mn_Notify].name = sFullName; sFullName = NULL;
										pNotify[mn_Notify].time = ::GetTickCount();
										mp_Notify = pNotify;
										++mn_Notify;
									}
								}
								LeaveLock();
							}
							if(NULL != sFullName) delete[] sFullName;
						}
					}
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:                    // 文件改名
					{
						if(dwNextEntryOffset != 0)
						{
							pfiNotifyInfo= (FILE_NOTIFY_INFORMATION*)((BYTE*)pfiNotifyInfo + dwNextEntryOffset);
						}
						dwNextEntryOffset = pfiNotifyInfo->NextEntryOffset;
						DWORD dwAction = pfiNotifyInfo->Action;
						DWORD dwFileNameLength = pfiNotifyInfo->FileNameLength;
						if(dwAction == FILE_ACTION_RENAMED_NEW_NAME)
						{
							TCHAR sNewName[1024];
							CPY_W2T(sNewName, pfiNotifyInfo->FileName, dwFileNameLength/sizeof(WCHAR));
							LPTSTRPrintf(__T("Rename %s -> %s"n"), sFileName, sNewName);
						}
						else
						{
							continue;
						}
					}
					break;
				}
				if(dwNextEntryOffset != 0)
				{
					pfiNotifyInfo= (FILE_NOTIFY_INFORMATION*)((BYTE*)pfiNotifyInfo + dwNextEntryOffset);
				}
			}while (dwNextEntryOffset != 0);
			// 投递目录监视
			::ZeroMemory(pOV->buff, 1024);
			::ReadDirectoryChangesW( pOV->handle
				, pOV->buff
				, 1024
				, FALSE
				, pOV->flag
				, NULL
				, (OVERLAPPED*)pOV
				, NULL
				);
		}
	}
	EnterLock();
	while(mn_Notify > 0)
	{
		--mn_Notify;
		delete[] mp_Notify[mn_Notify].name;
	}
	delete[] mp_Notify;
	mp_Notify = NULL;
	while(mn_OVPtr > 0)
	{
		::InterlockedDecrement(&mn_OVPtr);
		pOV = mp_OVPtr[mn_OVPtr];
		::CloseHandle(pOV->handle);
		delete[] pOV->path;
		delete pOV;
	}
	delete[] mp_OVPtr;
	mp_OVPtr = NULL;
	::CloseHandle(mh_IOCP);
	mh_IOCP = NULL;
	LeaveLock();
	return S_OK;
}
HRESULT P2PFileShare::Start()
{
	HRESULT hr;
	EnterLock();
	USP_ASSERT(mh_IOCP == NULL);
	// 创建完成端口
	mh_IOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if(NULL == mh_IOCP)
	{
		hr = ::GetLastError();
	}
	else
	{
		hr = _CreateWorkerThread();
		if(S_OK != hr)
		{
			::CloseHandle(mh_IOCP);
			mh_IOCP = NULL;
		}
	}
	LeaveLock();
	return hr;
}
VRESULT    P2PFileShare::Close(IN CONST BOOL bWait)
{
	::PostQueuedCompletionStatus(mh_IOCP, 0, NULL, NULL); // 通知工作线程关闭
	EnterLock();
	// 先取消所有的 IO 操作，否则有可能会有内存问题
	PATH_OV* pOV;
	for(LONG i=0;i<mn_OVPtr;i++)
	{
		pOV = mp_OVPtr[i];
		::CancelIo(pOV->handle);
	}
	LeaveLock();
	if(bWait)
	{
		while(mn_OVPtr > 0) ::Sleep(10);
	}
}
// 监视共享目录
HRESULT P2PFileShare::MonitorPath(IN LPCTSTR sPath)
{
	USP_ASSERT(mh_IOCP != NULL);
	if(NULL == mh_IOCP) return E_FAIL;
	PATH_OV*pOV = new PATH_OV;
	if(NULL == pOV) return E_OUTOFMEMORY;
	::ZeroMemory(pOV, sizeof(PATH_OV));
	pOV->path = NEW_T2T(sPath);
	if(NULL == pOV->path)
	{
		delete pOV;
	}
	// 创建目录句柄
	pOV->handle = ::CreateFile(   pOV->path
		, FILE_LIST_DIRECTORY
		, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
		, NULL
		, OPEN_EXISTING
		, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED
		, NULL
		);
	if(INVALID_HANDLE_VALUE == pOV->handle)
	{
		delete[] pOV->path;
		delete pOV;
		return ::GetLastError();
	}
	// 帮定目录句柄
	if(NULL == ::CreateIoCompletionPort(pOV->handle, mh_IOCP, NULL, 0))
	{
		::CloseHandle(pOV->handle);
		delete[] pOV->path;
		delete pOV;
		return ::GetLastError();
	}
	// 提交目录监视
	pOV->flag = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME;
	BOOL bSucceed = ::ReadDirectoryChangesW( pOV->handle
		, pOV->buff
		, 1024
		, FALSE
		, pOV->flag
		, NULL
		, (OVERLAPPED*)pOV
		, NULL
		);
	if(!bSucceed)
	{
		::CloseHandle(pOV->handle);
		delete[] pOV->path;
		delete pOV;
		return ::GetLastError();
	}
	HRESULT hr = S_OK;
	LONG i;
	EnterLock();
	for(i=0;i<mn_OVPtr;i++)
	{
		if(LPTSTRCompare(mp_OVPtr[i]->path, pOV->path) == 0)
		{
			hr = S_FALSE;
			break;
		}
	}
	if(i >= mn_OVPtr)
	{
		LPPATH_OV* pOVPtr = new LPPATH_OV[mn_OVPtr + 1];
		if(NULL == pOVPtr)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			if(mp_OVPtr != NULL)
			{
				::CopyMemory(pOVPtr, mp_OVPtr, sizeof(LPPATH_OV)*mn_OVPtr);
				delete[] mp_OVPtr;
			}
			pOVPtr[mn_OVPtr] = pOV;
			mp_OVPtr = pOVPtr;
			::InterlockedIncrement(&mn_OVPtr);
		}
	}
	LeaveLock();
	if(S_OK != hr)
	{
		::CloseHandle(pOV->handle);
		delete[] pOV->path;
		delete pOV;
		return hr;
	}
	return S_OK;
}
// 文件变化通知
LPTSTR P2PFileShare::GetNotify()
{
	LPTSTR sFileName = NULL;
	DWORD nTime = ::GetTickCount();
	EnterLock();
	for(LONG i=0;i<mn_Notify;i++)
	{
		if(nTime - mp_Notify[i].time >= 1*1000)
		{
			sFileName = mp_Notify[i].name;
			if(mn_Notify - i > 1)
			{
				::CopyMemory(mp_Notify + i, mp_Notify + i + 1, (mn_Notify - i - 1)*sizeof(FILE_NOTIFY));
			}
			--mn_Notify;
			if(mn_Notify == 0)
			{
				delete[] mp_Notify;
				mp_Notify = NULL;
			}
			break;
		}
	}
	LeaveLock();
	return sFileName;
}
int _tmain(IN INT nArgc, IN LPCTSTR* psArgv)
{
	UNREFERENCED_PARAMETER(nArgc);
	UNREFERENCED_PARAMETER(psArgv);
#ifdef UNICODE
	CRTSetLocale();         // 设置本地化开关，保证在 UniCode 下可以输出汉字
#endif // UNICODE
	ConsoleInit();
	HRESULT hr;
	P2PFileShare oShare;
	hr = oShare.Start();
	if(S_OK == hr)
	{
		hr = oShare.MonitorPath(__T("E:""EPServer""bin""incoming"""));
		hr = oShare.MonitorPath(__T("E:""EPServer""bin""incomtmp"""));
		AFSP sInput;
		for(;;)
		{
			LPTSTR sFileName = oShare.GetNotify();
			if(NULL != sFileName)
			{
				// 有一个文件已经完全复制完毕
				LPTSTRPrintf(__T("Hashing %s"n"), sFileName);
					// ...
					delete[] sFileName;
			}
			::Sleep(1000);
			//                   sInput.Attach(ConsoleGetStringNoEcho());
			//                   if(0 == LPTSTRICompare(sInput, __T("exit"))) break;
			//                   else
			//                   if(0 == LPTSTRICompare(sInput, __T("quit"))) break;
		}
		oShare.Close();
	}
	ConsoleTerm();
	return S_OK == hr ? 0 : -1;
}
#include <CTL/CTL_IMPL.HPP>
/*
Monitor File Change Infomation
---Completion Port Module
Monitor Example 1.0  适用于监控一个目录
Author: yaliao [lya_118@163.com]
*/
#include <windows.h>
#include <iostream.h>
#include <conio.h>
#define MAX_BUFFER  4096
//
//自定义结构，即“完成键”(单句柄数据)
//
typedef struct _PER_IO_CONTEXT
{
	OVERLAPPED              ol;
	HANDLE                     hDir;
	CHAR        lpBuffer[MAX_BUFFER];
	_PER_IO_CONTEXT*           pNext;
}PER_IO_CONTEXT, *PPER_IO_CONTEXT;
PPER_IO_CONTEXT g_pIContext;
typedef BOOL (WINAPI *lpReadDirectoryChangesW)( HANDLE,
											   LPVOID,
											   DWORD,
											   BOOL,
											   DWORD,
											   LPDWORD,
											   LPOVERLAPPED,
											   LPOVERLAPPED_COMPLETION_ROUTINE);
lpReadDirectoryChangesW ReadDirectoryChangesW = NULL;
HANDLE g_hIocp;
HANDLE g_hDir;
DWORD WINAPI CompletionRoutine(LPVOID lpParam);
void FileActionRoutine(PFILE_NOTIFY_INFORMATION pfi);
TCHAR szDirectory[] = "c:\\";
void main()
{
	HANDLE hRet;
	HANDLE hThread;
	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == g_hIocp)
	{
		cout <<"CreateIoCompletionPort Failed: " <<GetLastError() <<endl;
		return;
	}
	cout <<"monitor directory is: " <<szDirectory <<endl;
	g_hDir = CreateFile(  szDirectory,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ|
		FILE_SHARE_WRITE|
		FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS|
		FILE_FLAG_OVERLAPPED,
		NULL);
	if (NULL == g_hDir)
	{
		cout <<"CreateFile Failed: " <<GetLastError() <<endl;
		CloseHandle(g_hIocp);
		return;
	}
	HINSTANCE hInst = LoadLibrary("Kernel32.DLL");
	if(!hInst)
	{
		cout <<"LoadLibrary Failed: " <<GetLastError() <<endl;
		CloseHandle(g_hIocp);
		CloseHandle(g_hDir);
		return;
	}
	ReadDirectoryChangesW = (lpReadDirectoryChangesW)GetProcAddress(hInst, "ReadDirectoryChangesW");//获取DLL函数入口
	FreeLibrary(hInst);
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	for (int i=0; i<(int)SysInfo.dwNumberOfProcessors; i++)
	{
		hThread = CreateThread(NULL, 0, CompletionRoutine, NULL, 0, NULL);
		if(NULL == hThread)
		{
			cout <<"CreateThread Failed: " <<GetLastError() <<endl;
			CloseHandle(g_hIocp);
			CloseHandle(g_hDir);
			return;
		}
		CloseHandle(hThread);
	}
	g_pIContext = (PPER_IO_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PER_IO_CONTEXT));
	if(NULL == g_pIContext)
	{
		cout <<"HeapAlloc Failed: " <<GetLastError() <<endl;
		PostQueuedCompletionStatus(g_hIocp, 0, NULL, NULL);
		CloseHandle(g_hIocp);
		CloseHandle(g_hDir);
		return;
	}
	g_pIContext->hDir = g_hDir;
	ZeroMemory(&(g_pIContext->ol), sizeof(OVERLAPPED));
	ZeroMemory(g_pIContext->lpBuffer, MAX_BUFFER);
	g_pIContext->pNext = NULL;
	hRet = CreateIoCompletionPort(g_hDir, g_hIocp, (ULONG_PTR)g_pIContext, 0);
	if(NULL == hRet)
	{
		cout <<"CreateIoCompletionPort Failed: " <<GetLastError() <<endl;
		PostQueuedCompletionStatus(g_hIocp, 0, NULL, NULL);
		CloseHandle(g_hIocp);
		CloseHandle(g_hDir);
		HeapFree(GetProcessHeap(), 0, g_pIContext);
		return;
	}
	DWORD nBytes = 0;
	BOOL  bRet = FALSE;
	bRet = ReadDirectoryChangesW(g_pIContext->hDir,
		g_pIContext->lpBuffer,
		MAX_BUFFER,
		TRUE,
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME |
		FILE_NOTIFY_CHANGE_ATTRIBUTES |
		FILE_NOTIFY_CHANGE_SIZE |
		FILE_NOTIFY_CHANGE_LAST_ACCESS |
		FILE_NOTIFY_CHANGE_CREATION |
		FILE_NOTIFY_CHANGE_SECURITY |
		FILE_NOTIFY_CHANGE_LAST_WRITE,
		&nBytes,
		&(g_pIContext->ol),
		NULL);
	if(!bRet)
	{
		cout <<"ReadDirectoryChangesW Failed: " <<GetLastError() <<endl;
		PostQueuedCompletionStatus(g_hIocp, 0, NULL, NULL);
		CloseHandle(g_hIocp);
		CloseHandle(g_hDir);
		HeapFree(GetProcessHeap(), 0, g_pIContext);
		return;
	}
	cout <<"Enter q to exit: " <<endl;
	while(getch() != 'q');
	PostQueuedCompletionStatus(g_hIocp, 0, NULL, NULL);
	CloseHandle(g_hIocp);
	CloseHandle(g_hDir);
	HeapFree(GetProcessHeap(), 0, g_pIContext);
	cout <<"Main thread will exit " <<endl;
	getch();
}
DWORD WINAPI CompletionRoutine(LPVOID lpParam)
{
	DWORD dwBytes;
	PPER_IO_CONTEXT pIContext = NULL;
	LPOVERLAPPED pOL = NULL;
	PFILE_NOTIFY_INFORMATION pfi = NULL;
	BOOL bRet;
	DWORD cbOffset;
	DWORD nBytes;
	while(true)
	{
		bRet = GetQueuedCompletionStatus(g_hIocp, &dwBytes,(PULONG_PTR )&pIContext, &pOL, INFINITE);
		if(bRet)
		{
			if(NULL == pIContext)
			{
				cout <<"CompletionRoutine thread exit " <<endl;
				return 0;
			}
			pfi = (PFILE_NOTIFY_INFORMATION)pIContext->lpBuffer;
			do
			{
				try
				{
					switch(pfi->Action)
					{
					case FILE_ACTION_ADDED: //The file was added to the directory.
						cout <<"FILE_ACTION_ADDED: ";
						FileActionRoutine(pfi);
						break;
					case FILE_ACTION_REMOVED: //The file was removed from the directory.
						cout <<"FILE_ACTION_REMOVED: ";
						FileActionRoutine(pfi);
						break;
					case FILE_ACTION_MODIFIED: //The file was modified. This can be a change in the time stamp or attributes.
						cout <<"FILE_ACTION_MODIFIED: ";
						FileActionRoutine(pfi);
						break;
					case FILE_ACTION_RENAMED_OLD_NAME: //The file was renamed and this is the old name.
						cout <<"FILE_ACTION_RENAMED_OLD_NAME: ";
						FileActionRoutine(pfi);
						break;
					case FILE_ACTION_RENAMED_NEW_NAME: //The file was renamed and this is the new name.
						cout <<"FILE_ACTION_RENAMED_NEW_NAME: ";
						FileActionRoutine(pfi);
						break;
					default:
						break;
					}
					cbOffset = pfi->NextEntryOffset;//一次消息中包含了多个文件变化的信息吗？
					pfi = (PFILE_NOTIFY_INFORMATION)((LPBYTE) pfi + cbOffset);
				}
				catch(...)
				{
					LPVOID lpMsgBuf;
					FormatMessage( //把错误消息格式化
						FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(), //错误消息
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf, //格式化后的错误消息
						0,
						NULL
						);
					cout <<"CompletionRoutine Exception = " <<lpMsgBuf <<endl;
					LocalFree(lpMsgBuf);
				}
			}while(cbOffset);
		}
		g_pIContext->hDir = g_hDir;
		ZeroMemory(&(g_pIContext->ol), sizeof(OVERLAPPED));
		ZeroMemory(g_pIContext->lpBuffer, MAX_BUFFER);
		g_pIContext->pNext = NULL;
		nBytes = 0;
		bRet = FALSE;
		bRet = ReadDirectoryChangesW(g_pIContext->hDir,
			g_pIContext->lpBuffer,
			MAX_BUFFER,
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_SECURITY |
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			&nBytes,
			&(g_pIContext->ol),
			NULL);
		if(!bRet)
		{
			cout <<"ReadDirectoryChangesW Failed: " <<GetLastError() <<endl;
			return 0;
		}
	}
}
void FileActionRoutine(PFILE_NOTIFY_INFORMATION pfi)
{
	TCHAR szFileName[MAX_PATH];
	memset(szFileName,'\0',sizeof(szFileName));
	WideCharToMultiByte(CP_ACP, //ANSI码
		0,
		pfi->FileName,   //待转换的字符串
		pfi->FileNameLength/2,
		szFileName,     //转换后的字符串
		MAX_PATH,
		NULL,
		NULL);
	cout <<szFileName <<endl;
}
