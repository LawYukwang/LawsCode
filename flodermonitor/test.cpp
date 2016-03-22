// ≤‚ ‘¥˙¬Î
#include "cdirectorywatch.h"

void NotifyAction(DWORD dwAction, LPWSTR szFile, DWORD dwLength)
{
	switch(dwAction)
	{
	case FILE_ACTION_ADDED:
		wprintf(L"FILE_ACTION_ADDED: \n\t");
		break;

	case FILE_ACTION_REMOVED:
		wprintf(L"FILE_ACTION_REMOVED: \n\t");
		break;

	case FILE_ACTION_MODIFIED:
		wprintf(L"FILE_ACTION_MODIFIED: \n\t");
		break;

	case FILE_ACTION_RENAMED_OLD_NAME:
		wprintf(L"FILE_ACTION_RENAMED_OLD_NAME: \n\t");
		break;

	case FILE_ACTION_RENAMED_NEW_NAME:
		wprintf(L"FILE_ACTION_RENAMED_NEW_NAME: \n\t");
		break;

	default:
		break;
	}
	WCHAR szPath[MAX_PATH] = {0};
	wmemcpy(szPath, szFile, min(dwLength, MAX_PATH));
	wprintf(L"%s\n", szPath);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	CDirectoryWatch watch;
	wprintf(L"Start Directory Watch ...\n");
	watch.StartDirectoryWatch(_T("F:\\11"), NotifyAction);
	Sleep(30 * 1000);	
	watch.StopDirectoryWatch();
	wprintf(L"Stop Directory Watch ...\n");

	Sleep(10 * 1000);

	wprintf(L"Start Directory Watch ...\n");
	watch.StartDirectoryWatch(_T("F:\\11"), NotifyAction);
	Sleep(30 * 1000);	
	watch.StopDirectoryWatch();
	wprintf(L"Stop Directory Watch ...\n");
	Sleep(30 * 1000);
	wprintf(L"Process Exit ...\n");
	return 0;
}