// .cppÎÄ¼þ
#include "cdirectorywatch.h"
#include <strsafe.h>

typedef enum
{
	MSG_STARTWATCH = (WM_USER + 0x11),
	MSG_STOPWATCH,
	MSG_EXITTHREAD
};

#define MAX_BUFFER_SIZE	(1024)

typedef struct _tagWATCHPARAMETERS
{
	_tagWATCHPARAMETERS()
	{
		hFile = INVALID_HANDLE_VALUE;
		hEvent = NULL;
		memset(&ol, 0, sizeof(OVERLAPPED));
		pBuffer = NULL;
		dwBufferSize = 0;
		bExit = FALSE;
		pFn_NotifyAction = NULL;
	}
	HANDLE hFile;
	HANDLE hEvent;
	OVERLAPPED ol;
	BYTE* pBuffer;
	DWORD dwBufferSize;
	BOOL bExit;
	PFN_NotifyAction pFn_NotifyAction;
}WATCH_PARAMETERS, *PWATCH_PARAMETERS;

CDirectoryWatch::CDirectoryWatch() : m_hFile(INVALID_HANDLE_VALUE), m_pThread(NULL)
{
	memset(m_szDirectory, 0, sizeof(m_szDirectory));

	m_pThread = AfxBeginThread(ThreadProc, NULL, 0, CREATE_SUSPENDED, 0, NULL);
	if(NULL == m_pThread)
	{
		TRACE("Error Code : %d\n", GetLastError());
		return ;
	}
	m_pThread->m_bAutoDelete = FALSE;
	m_pThread->ResumeThread();
}


CDirectoryWatch::~CDirectoryWatch()
{
	if(INVALID_HANDLE_VALUE != m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	if((NULL != m_pThread) && (NULL != m_pThread->m_hThread))
	{

		m_pThread->PostThreadMessage(MSG_EXITTHREAD, 0, 0);
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pThread;
		m_pThread = NULL;
	}
}

BOOL CDirectoryWatch::StartDirectoryWatch(LPCTSTR lpszDirectory, PFN_NotifyAction pFn_NotifyAction)
{
	if(NULL == m_pThread)
	{
		return FALSE;
	}

	if(NULL == lpszDirectory)
	{
		return FALSE;
	}

	if(NULL == pFn_NotifyAction)
	{
		return FALSE;
	}

	if(!PathFileExists(lpszDirectory))
	{
		TRACE("Error Code : %d\n", GetLastError());
		return FALSE;
	}

	if(!PathIsDirectory(lpszDirectory))
	{
		TRACE("Error Code : %d\n", GetLastError());
		return FALSE;
	}

	if(0 == _tcslen(m_szDirectory))
	{
		StringCchPrintf(m_szDirectory, _countof(m_szDirectory), _T("%s"), lpszDirectory);
	}
	else if(CSTR_EQUAL != CompareStringOrdinal(m_szDirectory, -1, lpszDirectory, -1, TRUE))
	{
		TRACE("Not Change Directory.\n");
		return FALSE;
	}

	if(INVALID_HANDLE_VALUE == m_hFile)
	{
		m_hFile = CreateFile(lpszDirectory, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
		if(INVALID_HANDLE_VALUE == m_hFile)
		{
			TRACE("Error Code : %d\n", GetLastError());
			return FALSE;
		}
	}

	return m_pThread->PostThreadMessage(MSG_STARTWATCH, (WPARAM)m_hFile, (LPARAM)pFn_NotifyAction);
}

BOOL CDirectoryWatch::StopDirectoryWatch()
{
	if(NULL != m_pThread)
	{
		return m_pThread->PostThreadMessage(MSG_STOPWATCH, 0, 0);
	}

	return FALSE;
}

UINT __cdecl CDirectoryWatch::DirectoryWatch(LPVOID lParam)
{
	WATCH_PARAMETERS* pParam = (WATCH_PARAMETERS*)lParam;
	if(NULL == pParam)
	{
		return 0;
	}
	HANDLE& hFile = pParam->hFile;
	BYTE* pBuffer = pParam->pBuffer;
	DWORD dwBufferSize = pParam->dwBufferSize;
	OVERLAPPED& ol = pParam->ol;
	HANDLE& hEvent = pParam->hEvent;
	BOOL& bExit = pParam->bExit;
	PFN_NotifyAction pFn_NotifyAction = pParam->pFn_NotifyAction;
	DWORD dwBytesReturn = 0;
	DWORD dwRet = WAIT_FAILED;
	DWORD dwOffSet = 0;
	TCHAR szFile[MAX_PATH] = {0};
	while(TRUE)
	{
		if(WAIT_OBJECT_0 != WaitForSingleObject(hEvent, INFINITE))
		{
			TRACE("Error Code : %d\n", GetLastError());
			break;
		}

		if(bExit)
		{
			break;
		}
	
		if(!ReadDirectoryChangesW(hFile, pBuffer, dwBufferSize, TRUE, 
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES
			| FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS
			| FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY, &dwBytesReturn, &ol, NULL))
		{
			TRACE("Error Code : %d\n", GetLastError());
			break;
		}
		if(!GetOverlappedResult(hFile, &ol, &dwBytesReturn, TRUE))
		{
			TRACE("Error Code : %d\n", GetLastError());
			break;
		}
		FILE_NOTIFY_INFORMATION* pFileNotify = (FILE_NOTIFY_INFORMATION*)pBuffer;
		
		do 
		{
			if(pFn_NotifyAction && (WAIT_OBJECT_0 == WaitForSingleObject(hEvent, 0)))
			{
				pFn_NotifyAction(pFileNotify->Action, pFileNotify->FileName, (pFileNotify->FileNameLength) / sizeof(WCHAR));
			}

			dwOffSet = pFileNotify->NextEntryOffset;
			pFileNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pFileNotify + dwOffSet);
		} while (dwOffSet);
	}
	TRACE0("DirectoryWatch Thread Exit ... \n");
	return 0;
}

UINT __cdecl CDirectoryWatch::ThreadProc(LPVOID lParam)
{
	WATCH_PARAMETERS* pParam = new WATCH_PARAMETERS;

	if(NULL == pParam)
	{
		goto __CLEANUP__;
	}

	BYTE* pBuffer = new BYTE[MAX_BUFFER_SIZE];
	if(NULL == pBuffer)
	{
		goto __CLEANUP__;
	}
	memset(pBuffer, 0, MAX_BUFFER_SIZE);
	pParam->pBuffer = pBuffer;
	pParam->dwBufferSize = MAX_BUFFER_SIZE;
	HANDLE hWatchEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(NULL == hWatchEvent)
	{
		goto __CLEANUP__;
	}
	pParam->ol.hEvent = hWatchEvent;
	CWinThread* pThread = NULL;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(NULL == hEvent)
	{
		goto __CLEANUP__;
	}
	pParam->hEvent = hEvent;
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0))
	{
		switch(msg.message)
		{
		case MSG_STARTWATCH:
			{
				HANDLE hFile = (HANDLE)(msg.wParam);
				PFN_NotifyAction pFn_NotifyAction = (PFN_NotifyAction)(msg.lParam);
				if((INVALID_HANDLE_VALUE == hFile) && (NULL == pFn_NotifyAction))
				{
					break;
				}
				if(NULL == pThread)
				{
					pParam->hFile = hFile;
					pParam->pFn_NotifyAction = pFn_NotifyAction;
					pThread = AfxBeginThread(DirectoryWatch, (LPVOID)pParam, 0, CREATE_SUSPENDED, NULL);
					if(NULL == pThread)
					{
						goto __CLEANUP__;
					}
					pThread->m_bAutoDelete = FALSE;
					pThread->ResumeThread();
				}				
				SetEvent(hEvent);
			}
			break;

		case MSG_STOPWATCH:
			{
				ResetEvent(hEvent);
			}
			break;

		case MSG_EXITTHREAD:
			{
				SetEvent(hEvent);
				pParam->bExit = FALSE;
				
				if((NULL != pThread) && (NULL != pThread->m_hThread))
				{
					WaitForSingleObject(pThread->m_hThread, INFINITE);
					delete pThread;
					pThread = NULL;
				}
				goto __CLEANUP__;
			}
			
		default:
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

__CLEANUP__:
	if(NULL != hWatchEvent)
	{
		CloseHandle(hWatchEvent);
		hWatchEvent = NULL;
	}
	if(NULL != pBuffer)
	{
		delete[] pBuffer;
		pBuffer = NULL;
	}
	if(NULL != pParam)
	{
		delete pParam;
		pParam = NULL;
	}
	TRACE0("ThreadProc Thread Exit ...\n");
	return 0;
}