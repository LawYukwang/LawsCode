// .hÎÄ¼þ
#include <Windows.h>
#pragma once

typedef void (*PFN_NotifyAction)(DWORD dwAction, LPWSTR szFile, DWORD dwLength);

class CDirectoryWatch
{
public:
	CDirectoryWatch(void);
	virtual ~CDirectoryWatch(void);

public:
	BOOL StartDirectoryWatch(LPCTSTR lpszDirectory, PFN_NotifyAction pFn_NotifyAction);
	BOOL StopDirectoryWatch(void);

private:
	static UINT __cdecl ThreadProc(LPVOID lParam);
	static UINT __cdecl DirectoryWatch(LPVOID lParam);

private:
	HANDLE m_hFile;
	CWinThread* m_pThread;
	TCHAR m_szDirectory[MAX_PATH];
};