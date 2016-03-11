//  *************************************************************
//  Copyright (C) 2000-2009,Seuu Technologies Co., Ltd.
//  
//  File name  : SuperLog.cpp
//  Description: ��־д����ʵ��
//  Version    : 1.0
//  Author     : Seuu
//  Created    : 2009-05-08 22:32:31
//  *************************************************************

#include "SuperLog.h"
#include <fstream>
#include <istream>
#include <ostream>
#include <ctime>
#include <tchar.h>

using namespace std; 

time_t    g_tmCurTimeL;
string    g_strTimeL;
// �ྲ̬������ʼ��
string    CSuperLog::m_strWriteStrInfoL;
int       CSuperLog::m_iWriteBinLogLen = 0;
int       CSuperLog::m_iCurLogFileSeq = 0;

#ifdef _DEBUG
int       CSuperLog::m_iLogLevel  = CSuperLog::ENUM_LOG_LEVEL_DEBUG;
#else
int       CSuperLog::m_iLogLevel  = CSuperLog::ENUM_LOG_LEVEL_RUN;
#endif

TCHAR*    g_pszLogLevel[]          = {_T("0"), _T("1"), _T("2"), _T("3")};
HANDLE    CSuperLog::m_hMapLogFile = NULL;
LPTSTR    CSuperLog::m_psMapAddr   = NULL;  
char      g_szWriteBinInfo[MAX_BIN_LOG_INFO_LEN];
TCHAR*    g_pszLogFileName[MAX_LOG_FILE_COUNT] = {_T("../log/defectsearchlog.txt"), _T("../log/log2.txt"), _T("../log/log3.txt")};
HANDLE    CSuperLog::m_hThread     = NULL;
unsigned  CSuperLog::m_uiThreadID  = 0;
bool      CSuperLog::m_bRun        = true;
CSuperLog::enLogStatus CSuperLog::m_enStatus   = CSuperLog::ENUM_LOG_INIT;

fstream           CSuperLog::m_pFileL;
//fstream            CSuperLog::m_pFileLL      = NULL;
int                    CSuperLog::m_iMaxLogFileLen = MAX_LOG_FILE_LEN;
CRITICAL_SECTION       CSuperLog::m_csWriteLog;

// �����Ѿ�������ȫ�ֺ���
CSuperLog g_SuperLog;

// string ת���� lpcwstr
LPCWSTR stringToLPCWSTR(string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t *wcstring = (wchar_t *)malloc(sizeof(wchar_t)*(orig.length()-1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);
	std::wstring wstrResult(wcstring);
	if(wcstring){
		free(wcstring); 
	}
	return wstrResult.c_str();
}
// �ڶ��ֽڼ�����£��ú������ã�
#ifdef _UNICODE
string TCHAR2STRIN(TCHAR *STR)
{
	int iLen = WideCharToMultiByte(CP_ACP, 0,STR, -1, NULL, 0, NULL, NULL);
	char* chRtn =new char[iLen*sizeof(char)];
	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
	std::string str(chRtn);
	return str;
}
#endif


DWORD WINAPI CSuperLog::LogProcStart( LPVOID  args )
{
	int nCount = 1;
	//CString strTemp;
	string strTempL;
	WriteLog(_T("��־�߳�����."), ENUM_LOG_LEVEL_DEBUG, true);
	//WriteLog(_T("��־�߳�����."), ENUM_LOG_LEVEL_RUN, true);
	// �߳̿�ʼ
	do 
	{
		Sleep(300);
		if (++nCount % 10 == 0 )
		{
			//WriteLog(strTemp, ENUM_LOG_LEVEL_ERROR, true); // ÿ������дһ����־
			WriteLog(strTempL, ENUM_LOG_LEVEL_ERROR, true); // ÿ������дһ����־
			CheckLogLevel();
		}

		if (nCount % 40 == 0 || m_enStatus == ENUM_LOG_INVALID)
		{
			// ÿ12����һ���ļ�
			if (ENUM_LOG_INVALID ==  OpenLogFile())
			{
				Sleep(3000);
				OpenLogFile();
			}
		}

	} while (m_bRun);

	// ����ʼ�˳�
	WriteLog(_T("Super logger has exited."), ENUM_LOG_LEVEL_RUN, true);
	//if (m_pFile != NULL)
	//{
	//    m_pFile->Close();
	//    delete m_pFile;
	//    m_pFile = NULL;
	//}

	if (m_pFileL.is_open())
	{
		//m_pFileL->Close();

		m_pFileL.close();

		//delete m_pFileL;
		//m_pFileL = NULL;
	}

	m_enStatus = ENUM_LOG_EXITED;
	//_endthreadex( 0 );
	return 0;
} 

CSuperLog::CSuperLog(void)
{
	// ��ʼ���ٽ�������
	InitializeCriticalSection(&m_csWriteLog); 
	// ��һ����־�ļ�
	OpenLogFile();
	// ������Ϣ
	//m_strWriteStrInfo = WELCOME_LOG_INFO;

	m_strWriteStrInfoL = WELCOME_LOG_INFOL;
	WriteLog(_T("Super logger start up."), ENUM_LOG_LEVEL_RUN, true);

	//CString str1 = filePath; 
	//sprintf_s(&filePath[0], MAX_PATH, "%s:%d", __FILE__,__LINE__);
	//string s(&filePath[0],&filePath[strlen(filePath)]); 
	//WriteLog(_T(__FILE__), ENUM_LOG_LEVEL_RUN, true);
	//WriteLog(_T(filePath), ENUM_LOG_LEVEL_RUN, true);
	//char *str = filePath;
	//	WriteLog(str, ENUM_LOG_LEVEL_RUN, true);

	//WriteLogL(_T(__FILE__), ENUM_LOG_LEVEL_RUN, true);
	//"__FILE__"
	// �������ļ��ж�ȡ��־����
	OperaterConfig(FALSE);
	// ͬ���������ڴ���
	SetLogLevelShareMem(m_iLogLevel);
	// Create the Logger thread.
	//m_hThread = (HANDLE)_beginthreadex( NULL, 0, &LogProcStart, NULL, 0, &m_uiThreadID );


	m_hThread = CreateThread(NULL, 0, LogProcStart, NULL, 0, NULL);
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;

}

CSuperLog::~CSuperLog(void)
{
	m_bRun = false;
	m_enStatus = ENUM_LOG_EXITING;

	for (int i = 0; i < 50; i++)
	{
		Sleep(300);
		if (m_enStatus == ENUM_LOG_EXITED)
		{
			break;
		}
	}
	DeleteCriticalSection(&m_csWriteLog);
	if (m_psMapAddr != NULL)
	{
		UnmapViewOfFile(m_psMapAddr);
	}

}

int CSuperLog::OperaterConfig(BOOL bSave)
{
	TCHAR szPath[MAX_PATH];
	if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
	{
		WRITE_LOG(_T("GetModuleFileNameʧ�ܡ�"), LOG_LEVEL_ERROR);
		return -1;
	}

	//CString strDir = szPath;

	UINT len = _tcslen(szPath)*2;
	char *buf = (char *)malloc(len);
	//UINT i = wcstombs(buf,szPath,len);
	//return buf;
	size_t   i;
#ifdef _UNICODE
	errno_t einval= wcstombs_s(&i,  buf, len, szPath, _tcslen(szPath));
#endif

	string strDirL = buf;
	//int nPos = strDir.ReverseFind(_T('\\'));

	int nPosL = strDirL.rfind(('\\'));

	//if (nPos == -1)
	//{
	//    WRITE_LOG(_T("GetModuleFileNameȡ����Ϣ�쳣��"), LOG_LEVEL_ERROR);
	//    return -1;
	//}


	if (nPosL == -1)
	{
		WRITE_LOG(_T("GetModuleFileNameȡ����Ϣ�쳣��"), LOG_LEVEL_ERROR);
		return -1;
	}


	//strDir = strDir.Left(nPos + 1);
	//strDir += _T("SuperLog.ini");


	strDirL = strDirL.substr(nPosL + 1);
	strDirL += ("SuperLog.ini");

#ifdef _UNICODE
		LPCWSTR a = (LPCWSTR)strDirL.c_str();
#else
		LPCSTR a = (LPCSTR)strDirL.c_str();
#endif

	if (bSave)
	{
		//WritePrivateProfileString(
		//    _T("SuperLog"), 
		//    _T("LogLevel"), 
		//    g_pszLogLevel[m_iLogLevel],
		//    strDir);

		WritePrivateProfileString(
			_T("SuperLog"), 
			_T("LogLevel"), 
			g_pszLogLevel[m_iLogLevel],
			a);


		//CString temp;
		//temp.Format(_T("%d"), m_iMaxLogFileLen);
		//WritePrivateProfileString(
		//    _T("SuperLog"), 
		//    _T("MaxLogFileLen"), 
		//    (LPCTSTR)temp,
		//    strDir);

		WritePrivateProfileString(
			_T("SuperLog"), 
			_T("MaxLogFileLen"), 
			_T("%d"),
			a);



	} 
	else
	{

		//m_iLogLevel = GetPrivateProfileInt(
		//          _T("SuperLog"), 
		//          _T("LogLevel"), 
		//          ENUM_LOG_LEVEL_RUN,
		//          strDir);

		m_iLogLevel = GetPrivateProfileInt(
			_T("SuperLog"), 
			_T("LogLevel"), 
			ENUM_LOG_LEVEL_RUN,
			a);

		if (m_iLogLevel > ENUM_LOG_LEVEL_ERROR || m_iLogLevel < ENUM_LOG_LEVEL_DEBUG)
		{
			WriteLog(_T("��־�������÷Ƿ���"),LOG_LEVEL_ERROR);
			m_iLogLevel = ENUM_LOG_LEVEL_RUN;
		}

		//m_iMaxLogFileLen  = GetPrivateProfileInt(
		//    _T("SuperLog"), 
		//    _T("MaxLogFileLen"), 
		//    MAX_LOG_FILE_LEN,
		//    strDir);

		m_iMaxLogFileLen  = GetPrivateProfileInt(
			_T("SuperLog"), 
			_T("MaxLogFileLen"), 
			MAX_LOG_FILE_LEN,
			a);

		if (m_iMaxLogFileLen > 1024*1024*1024 || m_iMaxLogFileLen < MAX_BIN_LOG_INFO_LEN) 
		{
			WriteLog(_T("����ļ��������÷Ƿ���"),LOG_LEVEL_ERROR);
			m_iMaxLogFileLen = MAX_LOG_FILE_LEN;
		}
	}
	return 0;
}

int CSuperLog::CheckLogLevel()
{
	int iLevel = GetLogLevelShareMem();
	if (iLevel != m_iLogLevel)
	{
		SetLogLevelShareMem(iLevel);
		m_iLogLevel = iLevel;
		OperaterConfig(TRUE);
	}

	return 0;
}

int CSuperLog::SetLogLevelShareMem(int iLevel)
{
	if (m_hMapLogFile == NULL || m_psMapAddr == NULL)
	{
		GetLogLevelShareMem();
	}

	if (m_hMapLogFile != NULL && m_psMapAddr != NULL)
	{
		_tcscpy_s(m_psMapAddr, 1024, g_pszLogLevel[iLevel]);                  
		FlushViewOfFile(m_psMapAddr, _tcslen(g_pszLogLevel[iLevel]));
		WriteLog(_T("������־����ɹ���"), ENUM_LOG_LEVEL_RUN);
	}
	return 0;
}

int CSuperLog::GetLogLevelShareMem(void)
{
	//�򿪹�����ļ�����
	if (m_hMapLogFile == NULL)
	{
		m_hMapLogFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("SuperLogShareMem"));
	}
	if (m_hMapLogFile != NULL)
	{
		//��ʾ������ļ����ݡ�
		if (m_psMapAddr == NULL)
		{
			m_psMapAddr = (LPTSTR)MapViewOfFile(m_hMapLogFile, FILE_MAP_ALL_ACCESS,0,0,0);
			WriteLog(m_psMapAddr, ENUM_LOG_LEVEL_DEBUG);
		}
	}
	else
	{
		//���������ļ���
		m_hMapLogFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,1024, _T("SuperLogShareMem"));
		if (m_hMapLogFile != NULL)
		{
			//�������ݵ������ļ��
			m_psMapAddr = (LPTSTR)MapViewOfFile(m_hMapLogFile,FILE_MAP_ALL_ACCESS, 0,0,0);
			if (m_psMapAddr != NULL)
			{
				_tcscpy_s(m_psMapAddr, 1024, g_pszLogLevel[m_iLogLevel]);                  
				FlushViewOfFile(m_psMapAddr, _tcslen(g_pszLogLevel[m_iLogLevel]));
				WriteLog(_T("����Ĭ����־���𵽹����ڴ��гɹ���"), ENUM_LOG_LEVEL_RUN);

			}
		}
		else
		{
			WriteLog(_T("���������ڴ�ʧ�ܡ�"), ENUM_LOG_LEVEL_ERROR);
		}
	}

	if (m_psMapAddr != NULL)
	{
		if (_tcscmp(m_psMapAddr, g_pszLogLevel[ENUM_LOG_LEVEL_RUN]) == 0)
		{
			return ENUM_LOG_LEVEL_RUN;
		}
		else if (_tcscmp(m_psMapAddr, g_pszLogLevel[ENUM_LOG_LEVEL_DEBUG]) == 0)
		{
			return ENUM_LOG_LEVEL_DEBUG;
		}
		else if (_tcscmp(m_psMapAddr, g_pszLogLevel[ENUM_LOG_LEVEL_ERROR]) == 0)
		{
			return ENUM_LOG_LEVEL_ERROR;
		}
		else
		{
			return ENUM_LOG_LEVEL_RUN;
		}    
	}

	return m_iLogLevel;
}


CSuperLog::enLogStatus CSuperLog::OpenLogFile(void)
{
	EnterCriticalSection(&m_csWriteLog); 
	// д������г������Ҫ�ر��ļ�
	if (m_enStatus == ENUM_LOG_INVALID && m_iCurLogFileSeq != 0)
	{
		m_iCurLogFileSeq--;
		//if (m_pFile != NULL)
		//{
		//    m_pFile->Close();
		//    delete m_pFile;
		//    m_pFile = NULL;
		//}
		if(m_pFileL.is_open())
		{
			m_pFileL.close();
		}

	}

	for (int iRunCount = 0; iRunCount < MAX_LOG_FILE_COUNT; iRunCount++)
	{
		//if (m_pFile == NULL)
		//{
		//    m_pFile = new CStdioFile;
		//    if (m_pFile == NULL)
		//    {
		//        LeaveCriticalSection(&m_csWriteLog);
		//        return m_enStatus = ENUM_LOG_INVALID;
		//    }

		/*BOOL bRet = m_pFile->Open(
		g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT],
		CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyNone | CFile::modeNoTruncate);
		*/
		//m_pFileL->open(
		//g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT],
		//fstream::in | fstream::out| fstream::binary | fstream::app);
		//fstream fs;
		//if(fs){
		//	cout<<""<<endl;
		//}
		if(!m_pFileL.is_open())
		{
			m_pFileL.close();

			m_pFileL.open(g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT], fstream::in | fstream::out| fstream::binary | fstream::app);

			//if(m_pFileL.is_open())
			//{
			//	// ���Գɹ�
			//	int ss=1;
			//	m_pFileL.write("abc",3);
			//	m_pFileL.flush();
			//}

			/*   if (bRet)
			{
			WriteUnicodeHeadToFile(m_pFile);
			}*/

			if(m_pFileL.is_open())
			{
				WriteUnicodeHeadToFileL(m_pFileL);

			}

			else
			{
				//delete m_pFile;
				//m_pFile = NULL;
				m_pFileL.close();

				LeaveCriticalSection(&m_csWriteLog);
				return m_enStatus = ENUM_LOG_INVALID;
			}
		}

		// if (m_pFile->GetLength() > MAX_LOG_FILE_LEN)
		streampos ps= m_pFileL.tellg();
		if (ps > MAX_LOG_FILE_LEN)
		{
			m_pFileL.close();
			BOOL bRet = FALSE;
			// ��һ���ļ��������Ǹ��ļ�����д��һ���˵ġ�
			if (m_iCurLogFileSeq >= MAX_LOG_FILE_COUNT) 
			{
				// �����ļ�����д���ˣ���ǿ�ƴӵ�һ���ļ���ʼд��ͬʱ������ļ�
				//bRet = m_pFile->Open(
				//    g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT],
				//    CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyNone); 

				m_pFileL.open(g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT], fstream::in | fstream::out| fstream::binary | fstream::app);
			}
			else
			{
				// �򿪵ڶ����ļ����ټ���Ƿ�������ֵ
				/*bRet = m_pFile->Open(
				g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT],
				CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyNone | CFile::modeNoTruncate);
				*/
				m_pFileL.open(g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT], fstream::in | fstream::out| fstream::binary | fstream::app);

			}

			if (m_pFileL.is_open())
			{
				WriteUnicodeHeadToFileL(m_pFileL);
			}
			else
			{
				//delete m_pFile;
				//m_pFile = NULL;
				m_pFileL.close();
				LeaveCriticalSection(&m_csWriteLog);
				return m_enStatus = ENUM_LOG_INVALID;
			}
		}
		else
		{
			break;
		}
	}

	//m_pFile->SeekToEnd();
	m_pFileL.seekg (0, m_pFileL.end);
	LeaveCriticalSection(&m_csWriteLog);
	return m_enStatus = ENUM_LOG_RUN;
}


//int CSuperLog::WriteLog(CString &strLog,enLogInfoLevel enLevel/* = ENUM_LOG_LEVEL_RUN*/, bool bForce /*= false*/)
//{
//    if (enLevel < m_iLogLevel)
//    {
//        return -1;
//    }
//
//    EnterCriticalSection(&m_csWriteLog); 
//    if (!strLog.IsEmpty())
//    {
//        m_strWriteStrInfo += GetCurTimeStr();
//        // add log level info
//        if (enLevel == ENUM_LOG_LEVEL_ERROR)
//        {
//            m_strWriteStrInfo += _T("Error! ");
//        }
//        m_strWriteStrInfo += strLog;
//        m_strWriteStrInfo += _T("\r\n");
//    }
//
//    if ( bForce
//        || m_strWriteStrInfo.GetLength() > MAX_STR_LOG_INFO_LEN
//        || m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
//    {
//        // write info
//        WriteLogToFile();
//    }
//    LeaveCriticalSection(&m_csWriteLog);
//    return 0;
//}

int CSuperLog::WriteLog(string &strLog,enLogInfoLevel enLevel/* = ENUM_LOG_LEVEL_RUN*/, bool bForce /*= false*/)
{
	if (enLevel < m_iLogLevel)
	{
		return -1;
	}

	EnterCriticalSection(&m_csWriteLog); 
	if (!strLog.empty())
	{
		m_strWriteStrInfoL += GetCurTimeStrL();
		// add log level info
		if (enLevel == ENUM_LOG_LEVEL_ERROR)
		{
			m_strWriteStrInfoL += ("Error! ");
		}
		m_strWriteStrInfoL += strLog;
		m_strWriteStrInfoL += ("\r\n");
	}

	//if ( bForce
	//	|| m_strWriteStrInfo.GetLength() > MAX_STR_LOG_INFO_LEN
	//	|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
	//{
	//	// write info
	//	WriteLogToFile();
	//}


	if ( bForce
		|| m_strWriteStrInfoL.length() > MAX_STR_LOG_INFO_LEN
		|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
	{
		// write info
		WriteLogToFileL();
	}

	LeaveCriticalSection(&m_csWriteLog);
	return 0;
}

int CSuperLog::WriteLog(TCHAR* pstrLog, enLogInfoLevel enLevel /*= ENUM_LOG_LEVEL_RUN*/, bool bForce /*= false*/)
{
	if (pstrLog == NULL || enLevel < m_iLogLevel)
	{
		return -1;
	}

	EnterCriticalSection(&m_csWriteLog); 
	if (_tcslen(pstrLog) != 0)
	{
		//m_strWriteStrInfo += GetCurTimeStr();
		m_strWriteStrInfoL += GetCurTimeStrL();
		if (enLevel == ENUM_LOG_LEVEL_ERROR)
		{
			//m_strWriteStrInfo += _T("Error! ");
			m_strWriteStrInfoL += ("Error! ");
		}
		//m_strWriteStrInfo += pstrLog;
		//m_strWriteStrInfo += _T("\r\n");
		//m_strWriteStrInfoL += pstrLog;
#ifdef _UNICODE
		string str1 = TCHAR2STRIN(pstrLog);
#else
		string str1 = pstrLog;
#endif 
		m_strWriteStrInfoL += str1;
		m_strWriteStrInfoL +=("\r\n");
	}

	//if ( bForce
	//    || m_strWriteStrInfo.GetLength() > MAX_STR_LOG_INFO_LEN
	//    || m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
	if ( bForce
		|| m_strWriteStrInfoL.length() > MAX_STR_LOG_INFO_LEN
		|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
	{
		// write info
		//WriteLogToFile();
		WriteLogToFileL();
	}
	LeaveCriticalSection(&m_csWriteLog);
	return 0;
}

/*int CSuperLog::WriteLog(char* pszLog,enLogInfoLevel enLevel , bool bForce )
{
// ��ʵ��
if (pszLog == NULL || enLevel < m_iLogLevel)
{
return -1;
}


EnterCriticalSection(&m_csWriteLog); 
if (strlen(pszLog) > 0)
{
//m_strWriteStrInfo += GetCurTimeStr();
m_strWriteStrInfoL += GetCurTimeStrL();
if (enLevel == ENUM_LOG_LEVEL_ERROR)
{
//m_strWriteStrInfo += _T("Error! ");
m_strWriteStrInfoL += ("Error! ");
}
//m_strWriteStrInfo += pszLog;
//m_strWriteStrInfo += _T("\r\n");

m_strWriteStrInfoL += pszLog;
m_strWriteStrInfoL += ("\r\n");
}

//if ( bForce	|| m_strWriteStrInfo.GetLength() > MAX_STR_LOG_INFO_LEN
//	|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
if ( bForce	|| m_strWriteStrInfoL.length() > MAX_STR_LOG_INFO_LEN
|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
{
// write info
//WriteLogToFile();
WriteLogToFileL();
}
LeaveCriticalSection(&m_csWriteLog);
return 0;
}*/

/*int CSuperLog::WriteLogL(TCHAR* pstrLog,  enLogInfoLevel enLevel , bool bForce ,int )
{
if (pstrLog == NULL || enLevel < m_iLogLevel)
{
return -1;
}

EnterCriticalSection(&m_csWriteLog); 
if (_tcslen(pstrLog) != 0)
{
//m_strWriteStrInfo += GetCurTimeStr();
m_strWriteStrInfoL += GetCurTimeStrL();

if (enLevel == ENUM_LOG_LEVEL_ERROR)
{
//m_strWriteStrInfo += _T("Error! ");
m_strWriteStrInfoL += ("Error! ");
}
//m_strWriteStrInfo += pstrLog;
//m_strWriteStrInfo += _T("\r\n");

//m_strWriteStrInfoL += pstrLog;

string str1 = TCHAR2STRIN(pstrLog);
m_strWriteStrInfoL += str1;
m_strWriteStrInfoL +=("\r\n");
}

//if ( bForce
//	|| m_strWriteStrInfo.GetLength() > MAX_STR_LOG_INFO_LEN
//	|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)

if ( bForce
|| m_strWriteStrInfoL.length() > MAX_STR_LOG_INFO_LEN
|| m_iWriteBinLogLen > MAX_BIN_LOG_INFO_LEN/10)
{
// write info
WriteLogToFileL();
}
LeaveCriticalSection(&m_csWriteLog);
return 0;
} */

//int CSuperLog::WriteUnicodeHeadToFile(CFile * pFile)
//{
//    if (pFile == NULL)
//    {
//        return -1;
//    }
//    try
//    {
//        if (pFile->GetLength() == 0)
//        {
//            m_pFile->Write("\377\376", 2);
//            if (m_enStatus == ENUM_LOG_RUN)
//            {
//                m_pFile->WriteString(WELCOME_LOG_INFO);
//            }
//            m_pFile->Flush();
//        }
//    }
//    catch (...)
//    {
//        return -1;
//    }
//    return 0;
//}

int CSuperLog::WriteUnicodeHeadToFileL(fstream& pFileL)
{
	if (!pFileL.is_open())
	{
		return -1;
	}
	try
	{
		// ����������������
		//int ch=pFileL.get();
		//if ( (pFileL.eof()))


		m_pFileL.seekg(0,ios::end);
		streampos ps= m_pFileL.tellg();
		if(ps<1)
		{
			//pFileL.write("\377\376", 2);
			if (m_enStatus == ENUM_LOG_RUN)
			{
				char *str = WELCOME_LOG_INFOL;
				int size = strlen(str);
				pFileL.write(str,size);
			}
			pFileL.flush();

		}
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}

//CString& CSuperLog::GetCurTimeStr()
//{
//    g_tmCurTime = CTime::GetCurrentTime();// time(NULL);
//    g_strTime = g_tmCurTime.Format(_T("%Y-%m-%d %H:%M:%S "));
//    return g_strTime;
//}

string& CSuperLog::GetCurTimeStrL()
{
	//g_tmCurTimeL = CTime::GetCurrentTime();// time(NULL);

	//use _CRT_SECURE_NO_WARNINGS:
	g_strTimeL = "";
	g_tmCurTimeL = time(NULL);  

	tm timeinfo;
	localtime_s(&timeinfo,&g_tmCurTimeL);	

	char str[MAX_PATH];
	_itoa_s(timeinfo.tm_year+1900, str, MAX_PATH,10);
	g_strTimeL = g_strTimeL+str+"-";

	_itoa_s(timeinfo.tm_mon+1, str,MAX_PATH, 10);
	g_strTimeL = g_strTimeL+str+"-";

	_itoa_s(timeinfo.tm_mday, str, MAX_PATH,10);
	g_strTimeL = g_strTimeL+str+" ";

	_itoa_s(timeinfo.tm_hour, str,MAX_PATH, 10);
	g_strTimeL = g_strTimeL+str+":";

	_itoa_s(timeinfo.tm_min, str,MAX_PATH, 10);
	g_strTimeL = g_strTimeL+str+":";

	_itoa_s(timeinfo.tm_sec, str, MAX_PATH,10);
	g_strTimeL = g_strTimeL+str+" ";

	//g_strTimeL = g_tmCurTimeL.Format(_T("%Y-%m-%d %H:%M:%S "));
	return g_strTimeL;
}



//int CSuperLog::WriteLogToFile()
//{
//    if (m_pFile == NULL 
//        || (m_iWriteBinLogLen == 0 && m_strWriteStrInfo.IsEmpty()) 
//        || m_enStatus == ENUM_LOG_INIT 
//        || m_enStatus == ENUM_LOG_EXITED
//        || m_enStatus == ENUM_LOG_INVALID
//        )
//    {
//        return 0;
//    }
//
//    try
//    {
//        m_pFile->WriteString(m_strWriteStrInfo); 
//        m_pFile->Flush();	
//		
//    }
//    catch (...)
//    {
//        m_enStatus = ENUM_LOG_INVALID;
//    }
//
//    m_strWriteStrInfo.Empty();
//    return 0;
//}

int CSuperLog::WriteLogToFileL()
{
	//if (m_pFileL.is_open() < 1 
	//	|| (m_iWriteBinLogLen == 0 && m_strWriteStrInfo.IsEmpty()) 
	//	|| m_enStatus == ENUM_LOG_INIT 
	//	|| m_enStatus == ENUM_LOG_EXITED
	//	|| m_enStatus == ENUM_LOG_INVALID
	//	)
	if (!m_pFileL.is_open() 
		|| (m_iWriteBinLogLen == 0 && m_strWriteStrInfoL.empty()) 
		|| m_enStatus == ENUM_LOG_INIT 
		|| m_enStatus == ENUM_LOG_EXITED
		|| m_enStatus == ENUM_LOG_INVALID
		)
	{
		return 0;
	}

	try
	{
		//char *str = WELCOME_LOG_INFOL;
		//int size = strlen(str);

		const char *str = m_strWriteStrInfoL.c_str();
		int size = strlen(str);

		m_pFileL.write(str,size); 
		m_pFileL.flush();	

	}
	catch (...)
	{
		m_enStatus = ENUM_LOG_INVALID;
	}

	//m_strWriteStrInfo.Empty();
	m_strWriteStrInfoL="";
	return 0;
}


//#include <windows.h>
//#include <stdio.h>
//
//class  Class
//{
//public:
//
//	static int  i;
//
//	static DWORD WINAPI ThreadProc( LPVOID lpParameter );
//	int  StartThread();
//
//};
//
//DWORD WINAPI Class::ThreadProc( LPVOID lpParameter )
//{
//	int  j = (int)lpParameter;
//
//	//   i = 2;
//	printf("%d\n", i);
//
//	return 0;
//}
//
//int  Class::StartThread()
//{
//	CreateThread(NULL, 0, ThreadProc, (LPVOID)2, 0, 0);
//	Sleep(1000);
//	return 0;
//}
//int  Class::i;
//int main()
//{
//	Class  c1;
//
//	c1.StartThread();
//
//	return 0;
//}

//#include "stdafx.h"  
//
//#include <iostream>  
//
//#include <iomanip>  
//#include <iosfwd>  
//#include <streambuf>  
//#include <ios>  
//#include <ostream>  
//#include <fstream>  
//
//#include <time.h>  
//#include <vector>  
//
//using namespace std;  
//
//class Test  
//{  
//public:  
//	int v1,v2,v3,v4,v5,v6;  
//public:  
//	Test(int v)  
//	{  
//		v1 = v2 = v3 = v4 = v5 = v6 = v;  
//	}  
//};  
//
//class MyOStream   
//{  
//public:  
//	static const unsigned int BUF_SIZE = 102400;  
//
//	MyOStream(const char* filename) :  
//		out(filename, std::ios_base::binary), current(0)  
//	{  
//	}  
//
//	void write(const char* buffer, unsigned int size)  
//	{  
//		if(current + size > BUF_SIZE) {  
//			this->flush();  
//			out.write(buffer, size);  
//			return;  
//		}  
//
//		if(current+size > BUF_SIZE) {  
//			this->flush();  
//		}  
//
//		memcpy(&this->buffer[current], buffer, size);  
//		current += size;  
//	}  
//
//	void flush()  
//	{  
//		if (current == 0)   
//			return;  
//
//		out.write(buffer, current);  
//		current = 0;  
//	}  
//
//	void close()  
//	{  
//		flush();  
//		out.close();  
//	}  
//
//private:  
//	std::ofstream out;  
//	char buffer[BUF_SIZE];  
//	int current;  
//};  
//
//void write(std::ostream& out, const Test& test)  
//{  
//	out.write((const char*) &test.v1, sizeof(int));  
//	out.write((const char*) &test.v2, sizeof(int));  
//	out.write((const char*) &test.v3, sizeof(int));  
//	out.write((const char*) &test.v4, sizeof(int));  
//	out.write((const char*) &test.v5, sizeof(int));  
//	out.write((const char*) &test.v6, sizeof(int));  
//}  
//
//void write2(ostream& out, const Test& test) {  
//	out.rdbuf()->sputn((const char*) &test.v1, sizeof(int));  
//	out.rdbuf()->sputn((const char*) &test.v2, sizeof(int));  
//	out.rdbuf()->sputn((const char*) &test.v3, sizeof(int));  
//	out.rdbuf()->sputn((const char*) &test.v4, sizeof(int));  
//	out.rdbuf()->sputn((const char*) &test.v5, sizeof(int));  
//	out.rdbuf()->sputn((const char*) &test.v6, sizeof(int));  
//}  
//
//void write4(MyOStream& out, const Test& test) {  
//	out.write((const char*) &test.v1, sizeof(int));  
//	out.write((const char*) &test.v2, sizeof(int));  
//	out.write((const char*) &test.v3, sizeof(int));  
//	out.write((const char*) &test.v4, sizeof(int));  
//	out.write((const char*) &test.v5, sizeof(int));  
//	out.write((const char*) &test.v6, sizeof(int));  
//}  
//
//void outputTime(const char* message, const clock_t& clk1, const clock_t& clk2) {  
//	cout << message << " (ms): "<< (clk2 - clk1)<< endl;  
//}  
//
//void outputTime(const char* message, const clock_t& clk1) {  
//	clock_t clk2 = clock();  
//	outputTime(message, clk1, clk2);  
//}  
//
//int _tmain(int argc, _TCHAR* argv[])  
//{  
//	const int COUNT = 1000000;  
//	clock_t clk = clock();  
//
//	std::vector<Test> data;  
//	for(int i = 0; i < COUNT; i++) {  
//		data.push_back(Test(i));  
//	}  
//
//	outputTime("Create data vector", clk);  
//
//	clk = clock();  
//
//	ofstream out("D:\\test.bin", std::ios_base::binary);  
//	for(int i = 0 ; i < COUNT; i++) {  
//		const Test& test = data[i];  
//		write(out, test);  
//	}  
//	out.close();  
//
//	outputTime("Write all data without buffer", clk);  
//
//	clk = clock();  
//	ofstream out3("f:\\test3.bin", ios_base::binary);  
//	char buffer[102400];  
//	out3.rdbuf()->pubsetbuf(buffer, 102400);  
//	for (int i = 0; i < COUNT; i++) {  
//		const Test& test = data[i];  
//		write2(out3, test);  
//	}  
//	out3.close();  
//
//	outputTime("Write all data with big buffer", clk);  
//
//	clk = clock();  
//
//	MyOStream out4("f:\\test4.bin");  
//	for (int i = 0; i < COUNT; i++) {  
//		const Test& test = data[i];  
//		write4(out4, test);  
//	}  
//	out4.close();  
//
//	outputTime("Write all data with my buffer", clk);  
//}  
//
//
//
//
//class
//	<fstream>
//	std::fstream
//	typedef basic_fstream<char> fstream;
//Input/output file stream class
//	ios_base
//	ios
//	istream
//	ostream
//	iostream
//	fstream
//
//	Input/output stream class to operate on files.
//
//	Objects of this class maintain a filebuf object as their internal stream buffer, which performs input/output operations on the file they are associated with (if any).
//
//	File streams are associated with files either on construction, or by calling member open.
//
//	This is an instantiation of basic_fstream with the following template parameters:
//template parameter	definition	comments
//	charT	char	Aliased as member char_type
//	traits	char_traits<char>	Aliased as member traits_type
//
//	Apart from the internal file stream buffer, objects of this class keep a set of internal fields inherited from ios_base, ios and istream:
//
//field	member functions	description
//	Formatting	format flags	flags
//	setf
//	unsetf	A set of internal flags that affect how certain input/output operations are interpreted or generated.
//	See member type fmtflags.
//	field width	width	Width of the next formatted element to insert.
//	display precision	precision	Decimal precision for the next floating-point value inserted.
//	locale	getloc
//	imbue	The locale object used by the function for formatted input/output operations affected by localization properties.
//	fill character	fill	Character to pad a formatted field up to the field width (width).
//	State	error state	rdstate
//	setstate
//	clear	The current error state of the stream.
//	Individual values may be obtained by calling good, eof, fail and bad.
//	See member type iostate.
//	exception mask	exceptions	The state flags for which a failure exception is thrown.
//	See member type iostate.
//	Other	callback stack	register_callback	Stack of pointers to functions that are called when certain events occur.
//	extensible arrays	iword
//	pword
//	xalloc	Internal arrays to store objects of type long and void*.
//	tied stream	tie	Pointer to output stream that is flushed before each i/o operation on this stream.
//	stream buffer	rdbuf	Pointer to the associated streambuf object, which is charge of all input/output operations.
//	character count	gcount	Count of characters read by last unformatted input operation.
//
//	Member types
//	The class declares the following member types:
//member type	definition
//	char_type	char
//	traits_type	char_traits<char>
//	int_type	int
//	pos_type	streampos
//	off_type	streamoff
//
//	These member types are inherited from its base classes istream, ostream and ios_base:
//event
//	Type to indicate event type (public member type )
//	event_callback
//	Event callback function type (public member type )
//	failure
//	Base class for stream exceptions (public member class )
//	fmtflags
//	Type for stream format flags (public member type )
//	Init
//	Initialize standard stream objects (public member class )
//	iostate
//	Type for stream state flags (public member type )
//	openmode
//	Type for stream opening mode flags (public member type )
//	seekdir
//	Type for stream seeking direction flag (public member type )
//	sentry (istream)
//	Prepare stream for input (public member class )
//	sentry (ostream)
//	Prepare stream for output (public member class )
//
//	Public member functions
//	(constructor)
//	Construct object and optionally open file (public member function )
//	open
//	Open file (public member function )
//	is_open
//	Check if a file is open (public member function )
//	close
//	Close file (public member function )
//	rdbuf
//	Get the associated filebuf object (public member function )
//	operator= 
//	Move assignment (public member function )
//	swap 
//	Swap internals (public member function )
//
//	Public member functions inherited from istream
//	operator>>
//	Extract formatted input (public member function )
//	gcount
//	Get character count (public member function )
//	get
//	Get characters (public member function )
//	getline
//	Get line (public member function )
//	ignore
//	Extract and discard characters (public member function )
//	peek
//	Peek next character (public member function )
//	read
//	Read block of data (public member function )
//	readsome
//	Read data available in buffer (public member function )
//	putback
//	Put character back (public member function )
//	unget
//	Unget character (public member function )
//	tellg
//	Get position in input sequence (public member function )
//	seekg
//	Set position in input sequence (public member function )
//	sync
//	Synchronize input buffer (public member function )
//
//	Public member functions inherited from ostream
//	operator<<
//	Insert formatted output (public member function )
//	put
//	Put character (public member function )
//	write
//	Write block of data (public member function )
//	tellp
//	Get position in output sequence (public member function )
//	seekp
//	Set position in output sequence (public member function )
//	flush
//	Flush output stream buffer (public member function )
//
//	Public member functions inherited from ios
//	good
//	Check whether state of stream is good (public member function )
//	eof
//	Check whether eofbit is set (public member function )
//	fail
//	Check whether either failbit or badbit is set (public member function )
//	bad
//	Check whether badbit is set (public member function )
//	operator!
//	Evaluate stream (not) (public member function )
//	operator bool 
//	Evaluate stream (public member function )
//	rdstate
//	Get error state flags (public member function )
//	setstate
//	Set error state flag (public member function )
//	clear
//	Set error state flags (public member function )
//	copyfmt
//	Copy formatting information (public member function )
//	fill
//	Get/set fill character (public member function )
//	exceptions
//	Get/set exceptions mask (public member function )
//	imbue
//	Imbue locale (public member function )
//	tie
//	Get/set tied stream (public member function )
//	rdbuf
//	Get/set stream buffer (public member function )
//	narrow
//	Narrow character (public member function )
//	widen
//	Widen character (public member function )
//
//	Public member functions inherited from ios_base
//	flags
//	Get/set format flags (public member function )
//	setf
//	Set specific format flags (public member function )
//	unsetf
//	Clear specific format flags (public member function )
//	precision
//	Get/Set floating-point decimal precision (public member function )
//	width
//	Get/set field width (public member function )
//	imbue
//	Imbue locale (public member function )
//	getloc
//	Get current locale (public member function )
//	xalloc
//	Get new index for extensible array [static] (public static member function )
//	iword
//	Get integer element of extensible array (public member function )
//	pword
//	Get pointer element of extensible array (public member function )
//	register_callback
//	Register event callback function (public member function )
//	sync_with_stdio
//	Toggle synchronization with cstdio streams [static] (public static member function )
//
//	Non-member function overloads
//	swap 
//	Swap file streams (function template )
//
//
//	class
//		<string>
//		std::string
//		typedef basic_string<char> string;
//	String class
//		Strings are objects that represent sequences of characters.
//
//		The standard string class provides support for such objects with an interface similar to that of a standard container of bytes, but adding features specifically designed to operate with strings of single-byte characters.
//
//		The string class is an instantiation of the basic_string class template that uses char (i.e., bytes) as its character type, with its default char_traits and allocator types (see basic_string for more info on the template).
//
//		Note that this class handles bytes independently of the encoding used: 
//      If used to handle sequences of multi-byte or variable-length characters (such as UTF-8), all members of this class (such as length or size), 
//      as well as its iterators, will still operate in terms of bytes (not actual encoded characters).
//
//		Member types
//		member type	definition
//		value_type	char
//		traits_type	char_traits<char>
//		allocator_type	allocator<char>
//		reference	char&
//		const_reference	const char&
//		pointer	char*
//		const_pointer	const char*
//		iterator	a random access iterator to char (convertible to const_iterator)
//		const_iterator	a random access iterator to const char
//		reverse_iterator	reverse_iterator<iterator>
//		const_reverse_iterator	reverse_iterator<const_iterator>
//		difference_type	ptrdiff_t
//		size_type	size_t
//
//		Member functions
//		(constructor)
//		Construct string object (public member function )
//		(destructor)
//		String destructor (public member function )
//		operator=
//		String assignment (public member function )
//
//Iterators:
//	begin
//		Return iterator to beginning (public member function )
//		end
//		Return iterator to end (public member function )
//		rbegin
//		Return reverse iterator to reverse beginning (public member function )
//		rend
//		Return reverse iterator to reverse end (public member function )
//		cbegin 
//		Return const_iterator to beginning (public member function )
//		cend 
//		Return const_iterator to end (public member function )
//		crbegin 
//		Return const_reverse_iterator to reverse beginning (public member function )
//		crend 
//		Return const_reverse_iterator to reverse end (public member function )
//
//Capacity:
//	size
//		Return length of string (public member function )
//		length
//		Return length of string (public member function )
//		max_size
//		Return maximum size of string (public member function )
//		resize
//		Resize string (public member function )
//		capacity
//		Return size of allocated storage (public member function )
//		reserve
//		Request a change in capacity (public member function )
//		clear
//		Clear string (public member function )
//		empty
//		Test if string is empty (public member function )
//		shrink_to_fit 
//		Shrink to fit (public member function )
//
//		Element access:
//	operator[]
//	Get character of string (public member function )
//		at
//		Get character in string (public member function )
//		back 
//		Access last character (public member function )
//		front 
//		Access first character (public member function )
//
//Modifiers:
//	operator+=
//		Append to string (public member function )
//		append
//		Append to string (public member function )
//		push_back
//		Append character to string (public member function )
//		assign
//		Assign content to string (public member function )
//		insert
//		Insert into string (public member function )
//		erase
//		Erase characters from string (public member function )
//		replace
//		Replace portion of string (public member function )
//		swap
//		Swap string values (public member function )
//		pop_back 
//		Delete last character (public member function )
//
//		String operations:
//	c_str
//		Get C string equivalent (public member function )
//		data
//		Get string data (public member function )
//		get_allocator
//		Get allocator (public member function )
//		copy
//		Copy sequence of characters from string (public member function )
//		find
//		Find content in string (public member function )
//		rfind
//		Find last occurrence of content in string (public member function )
//		find_first_of
//		Find character in string (public member function )
//		find_last_of
//		Find character in string from the end (public member function )
//		find_first_not_of
//		Find absence of character in string (public member function )
//		find_last_not_of
//		Find non-matching character in string from the end (public member function )
//		substr
//		Generate substring (public member function )
//		compare
//		Compare strings (public member function )
//
//		Member constants
//		npos
//		Maximum value for size_t (public static member constant )
//
//		Non-member function overloads
//		operator+
//		Concatenate strings (function )
//		relational operators
//		Relational operators for string (function )
//		swap
//		Exchanges the values of two strings (function )
//		operator>>
//		Extract string from stream (function )
//		operator<<
//		Insert string into stream (function )
//		getline
//		Get line from stream into string (function )
//
//
//		��׼c++��string�ຯ������
//
//		ע�ⲻ��CString
//		֮��������char*���ַ�����ѡ��C++��׼������е�string�࣬����Ϊ����ǰ�߱Ƚ�������
//      ���ص����ڴ��Ƿ��㹻���ַ������ȵȵȣ�������Ϊһ������֣������ɵĲ�����������������Ǵ���������(������100%)����Ҫ��
//      ���ǿ����� = ���и�ֵ������== ���бȽϣ�+ ���������ǲ��Ǻܼ�?�������Ǿ����԰���������C++�Ļ����������͡�
//
//		���ˣ��������⡭����
//		���ȣ�Ϊ�������ǵĳ�����ʹ��string���ͣ����Ǳ������ͷ�ļ� <string>��
//
//		���£�
//#include <string> //ע�����ﲻ��string.h string.h��C�ַ���ͷ�ļ�
//#include <string>
//		using namespace std;
//
//	1������һ��C++�ַ���
//		����һ���ַ��������ܼ򵥣�
//		string Str;
//	�������Ǿ�������һ���ַ�������������Ȼ��һ���࣬���й��캯�����������������������û�д�����������Ծ�ֱ��ʹ����string��Ĭ�ϵĹ��캯����������������ľ��ǰ�Str��ʼ��Ϊһ�����ַ�����String��Ĺ��캯���������������£�
//		a)      string s;    //����һ�����ַ���s
//	b)      string s(str) //�������캯�� ����str�ĸ���Ʒ
//		c)      string s(str,stridx) //���ַ���str�ڡ�ʼ��λ��stridx���Ĳ��ֵ����ַ����ĳ�ֵ
//		d)      string s(str,stridx,strlen) //���ַ���str�ڡ�ʼ��stridx�ҳ��ȶ���strlen���Ĳ�����Ϊ�ַ����ĳ�ֵ
//		e)      string s(cstr) //��C�ַ�����Ϊs�ĳ�ֵ
//		f)      string s(chars,chars_len) //��C�ַ���ǰchars_len���ַ���Ϊ�ַ���s�ĳ�ֵ��
//		g)      string s(num,c) //����һ���ַ���������num��c�ַ�
//		h)      string s(beg,end) //������beg;end(������end)�ڵ��ַ���Ϊ�ַ���s�ĳ�ֵ
//		i)      s.~string() //���������ַ����ͷ��ڴ�
//		���ܼ򵥣��ҾͲ������ˡ�
//
//		2���ַ�����������
//		������C++�ַ������ص㣬���ȰѸ��ֲ����������г�������ϲ�������к�����������˿������������Լ�ϲ���ĺ������ٵ����濴������ϸ���͡�
//		a) =,assign()     //������ֵ
//		b) swap()     //���������ַ���������
//		c) +=,append(),push_back() //��β������ַ�
//		d) insert() //�����ַ�
//		e) erase() //ɾ���ַ�
//		f) clear() //ɾ��ȫ���ַ�
//		g) replace() //�滻�ַ�
//		h) + //�����ַ���
//		i) ==,!=,<,<=,>,>=,compare()    //�Ƚ��ַ���
//		j) size(),length()    //�����ַ�����
//		k) max_size() //�����ַ��Ŀ���������
//		l) empty()    //�ж��ַ����Ƿ�Ϊ��
//		m) capacity() //�������·���֮ǰ���ַ�����
//		n) reserve() //����һ�����ڴ�������һ���������ַ�
//		o) [ ], at() //��ȡ��һ�ַ�
//		p) >>,getline() //��stream��ȡĳֵ
//		q) <<    //��ıֵд��stream
//		r) copy() //��ĳֵ��ֵΪһ��C_string
//		s) c_str() //��������C_string����
//		t) data() //���������ַ�������ʽ����
//		u) substr() //����ĳ�����ַ���
//		v)���Һ���
//		w)begin() end() //�ṩ����STL�ĵ�����֧��
//		x) rbegin() rend() //���������
//		y) get_allocator() //����������
//
//		������ϸ���ܣ�
//
//		2��1 C++�ַ�����C�ַ�����ת��
//		C ++�ṩ����C++�ַ����õ���Ӧ��C_string�ķ�����ʹ��data()��c_str()��copy()�����У�
//      data()���ַ��������ʽ�����ַ������ݣ����������'/0'��
//      c_str()����һ���ԡ�/0'��β���ַ����飬
//      ��copy()����ַ��������ݸ��ƻ�д����е�c_string�� �ַ������ڡ�C++�ַ���������'/0'��β��
//      �ҵĽ������ڳ�������ʹ��C++�ַ�����ʹ�ã������򲻵��Ѳ�ѡ��c_string������ֻ�Ǽ򵥽��ܣ���ϸ�����ӹ���˭���һ���˽�ʹ���е�ע��������Ը�������(���ҵ��ռ���)������ϸ���͡�
//
//		2��2 ��С����������
//		һ��C++�ַ����������ִ�С��
//      a)���е��ַ�����������size()��length()�����ǵ�Ч��Empty()��������ַ����Ƿ�Ϊ�ա�
//      b)max_size() �����С��ָ��ǰC++�ַ�������ܰ������ַ������ܿ��ܺͻ�����������ƻ����ַ�������λ�������ڴ�Ĵ�С�й�ϵ������һ������²��ù�������Ӧ�ô�С�㹻�����õġ����ǲ����õĻ������׳�length_error�쳣
//      c)capacity()���·����ڴ�֮ǰ string���ܰ���������ַ�����������һ����Ҫָ������reserve()�������������Ϊstring���·����ڴ档���·���Ĵ�С������������� Ĭ�ϲ���Ϊ0����ʱ����string���з�ǿ����������
//
//		���б�Ҫ���ظ�һ��C++�ַ�����C�ַ���ת�������⣬����˻��������������⣬
//      �Լ����ĳ���Ҫ���ñ��˵ĺ�������ʲô�ģ��������ݿ����Ӻ���Connect(char*,char*)����
//      �����˵ĺ����� ���õ���char*��ʽ�ģ�������֪����c_str()��data()���ص��ַ������ɸ��ַ���ӵ�У�
//   	������һ��const char*,Ҫ����Ϊ�����ἰ�ĺ����Ĳ����������뿽����һ��char*,�����ǵ�ԭ�����ܲ�ʹ��C�ַ����Ͳ�ʹ�á�
//  	��ô����ʱ�����ǵĴ���ʽ�ǣ���� �˺����Բ���(Ҳ����char*)�����ݲ��޸ĵĻ������ǿ�������Connect((char*)UserID.c_str(), (char*)PassWD.c_str()),
//      ������ʱ���Ǵ���Σ�յģ���Ϊ����ת������ַ�����ʵ�ǿ����޸ĵģ�����Ȥ�ؿ����Լ���һ�ԣ���
//      ������ǿ�����Ǻ������õ�ʱ�򲻶Բ��������޸ģ�������뿽����һ��char*��ȥ��
//  	��Ȼ�������׵İ취������ʲô�����������һ��char*��ȥ��
//  	ͬʱ����Ҳ��������Ȼʹ��C�ַ������б�̵ĸ����ǣ�˵�����Ǹ���һ���Ҳ��Ϊ����Ҳ�������ǻ������ɿ��ʱ�����ǾͿ�ʼ����ˣ���������д�ĺ������ȽϹ淶���������ǾͲ��ؽ���ǿ��ת���ˡ�
//
//		2��3Ԫ�ش�ȡ
//		���ǿ���ʹ���±������[]�ͺ���at()��Ԫ�ذ������ַ����з��ʡ�����Ӧ��ע����ǲ�����[]������������Ƿ���Ч����Ч����0~str.length()�����������ʧЧ��������δ�������Ϊ����at()���飬���ʹ�� at()��ʱ��������Ч�����׳�out_of_range�쳣��
//
//		��һ�����ⲻ�ò�˵��const string a;�Ĳ�����[]������ֵ��a.length()��Ȼ��Ч���䷵��ֵ��'/0'�������ĸ��������a.length()����������Ч�ġ��������£�
//		const string Cstr(��const string��);
//	string Str(��string��);
//	Str[3];      //ok
//	Str.at(3);    //ok
//	Str[100]; //δ�������Ϊ
//	Str.at(100);    //throw out_of_range
//	Str[Str.length()]    //δ������Ϊ
//	Cstr[Cstr.length()] //���� ��/0'
//	Str.at(Str.length());//throw out_of_range
//	Cstr.at(Cstr.length()) ////throw out_of_range
//		�Ҳ��޳���������������û�ָ�븳ֵ��
//		char& r=s[2];
//	char* p= &s[3];
//	��Ϊһ���������·��䣬r,p����ʧЧ������ķ������ǲ�ʹ�á�
//
//		2��4�ȽϺ���
//		C ++�ַ���֧�ֳ����ıȽϲ�������>,>=,<,<=,==,!=��������֧��string��C-string�ıȽ�(�� str<��hello��)��
//      ��ʹ��>,>=,<,<=��Щ��������ʱ���Ǹ��ݡ���ǰ�ַ����ԡ����ַ����ֵ�˳�������һ�� �Ƚϡ�
//  	�ֵ�����ǰ���ַ�С���Ƚϵ�˳���Ǵ�ǰ���Ƚϣ���������ȵ��ַ��Ͱ����λ���ϵ������ַ��ıȽϽ��ȷ�������ַ����Ĵ�С��ͬʱ��string (��aaaa��) <string(aaaaa)��
//
//		��һ������ǿ��ıȽϺ����ǳ�Ա����compare()����֧�ֶ��������֧��������ֵ�ͳ��ȶ�λ�Ӵ������бȽϡ�������һ����������ʾ�ȽϽ��������ֵ�������£�0-��� ��0-���� <0-С�ڡ��������£�
//		string s(��abcd��);
//	s.compare(��abcd��); //����0
//	s.compare(��dcba��); //����һ��С��0��ֵ
//	s.compare(��ab��); //���ش���0��ֵ
//	s.compare(s); //���
//	s.compare(0,2,s,2,2); //�á�ab���͡�cd�����бȽ� С����
//	s.compare(1,2,��bcx��,2); //�á�bc���͡�bc���Ƚϡ�
//	��ô�������ܹ�ȫ�İɣ�ʲô���������������θ�ڣ��ðɣ��ǵ��ţ������и����Ի��ıȽ��㷨���ȸ�����ʾ��ʹ�õ���STL�ıȽ��㷨��ʲô����STLһ�ϲ�ͨ�����������ްɣ�
//
//		2��5 ��������
//		�����ַ����Ĳ�����ռ�˺ܴ�һ���֡�
//		���Ƚ���ֵ����һ����ֵ������Ȼ��ʹ�ò�����=����ֵ������string(�磺s=ns) ��c_string(�磺s=��gaint��)������һ�ַ����磺s='j'����������ʹ�ó�Ա����assign()�������Ա��������ʹ������Ķ��ַ�����ֵ�����Ǿ���˵���ɣ�
//		s.assign(str); //��˵
//	s.assign(str,1,3);//���str�ǡ�iamangel�� ���ǰѡ�ama�������ַ���
//	s.assign(str,2,string::npos);//���ַ���str������ֵ2��ʼ����β����s
//	s.assign(��gaint��); //��˵
//	s.assign(��nico��,5);//��'n' ��I' ��c' ��o' ��/0'�����ַ���
//	s.assign(5,'x');//�����x�����ַ���
//	���ַ�����յķ�����������s=����;s.clear();s.erase();(��Խ��Խ���þ�����˵���ñ������׶���)��
//		string�ṩ�˺ܶຯ�����ڲ��루insert����ɾ����erase�����滻��replace���������ַ���
//		��˵�����ַ�������˵����������β���ϣ��������� +=��append()��push_back()��
//
//		�������£�
//		s+=str;//�Ӹ��ַ���
//	s+=��my name is jiayp��;//�Ӹ�C�ַ���
//	s+='a';//�Ӹ��ַ�
//	s.append(str);
//	s.append(str,1,3);//�������� ͬǰ��ĺ�������assign�Ľ���
//	s.append(str,2,string::npos)//��������
//		s.append(��my name is jiayp��);
//	s.append(��nico��,5);
//	s.append(5,'x');
//	s.push_back(��a');//�������ֻ�����ӵ����ַ���STL��Ϥ����������ܼ�
//
//		Ҳ������Ҫ��string�м��ĳ��λ�ò����ַ�������ʱ���������insert()���������������Ҫ��ָ��һ������λ�õ���������������ַ�����������������ĺ��档
//		s.insert(0,��my name��);
//	s.insert(1,str);
//	������ʽ��insert()������֧�ִ��뵥���ַ�����ʱ�ĵ����ַ�����д���ַ�����ʽ(���˶���)��
//  ��Ȼ����ö��ģ��ǾͲ��ò�����������һ�λ���Ϊ�˲� �뵥���ַ���
//	insert()�����ṩ�������Բ��뵥���ַ����������غ�����insert(size_type index,size_type num,chart c)
//	��insert(iterator pos,size_type num,chart c)������size_type���޷���������iterator��char*,
//	���ԣ�����ô����insert�����ǲ��еģ�insert(0,1, 'j');
//  ��ʱ���һ��������ת������һ���أ������������ôд��insert((string::size_type)0,1,'j')��
//	�ڶ�����ʽָ ����ʹ�õ����������ַ�����ʽ���ں�����ἰ��˳����һ�£�string�кܶ������ʹ��STL�ĵ������ģ���Ҳ�������ú�STL������
//
//		ɾ������erase()����ʽҲ�кü��֣��淳�������滻����replace()Ҳ�кü�����
//
//		�����ɣ�
//		string s=��il8n��;
//	s.replace(1,2,��nternationalizatio��);//������1��ʼ��2���滻�ɺ����C_string
//	s.erase(13);//������13��ʼ����ȫɾ��
//	s.erase(7,5);//������7��ʼ����ɾ5��
//
//	2��6��ȡ�Ӵ����ַ�������
//		��ȡ�Ӵ��ĺ����ǣ�substr(),��ʽ���£�
//		s.substr();//����s��ȫ������
//	s.substr(11);//������11������Ӵ�
//	s.substr(5,6);//������5��ʼ6���ַ�
//	�������ַ�����������ĺ�����+����˭���������µ�120��
//
//		2��7�����������
//		1��>> ����������ȡһ��string��
//		2��<< ��һ��stringд���������
//		��һ����������getline(),������������ȡһ�����ݣ�ֱ���������з������ļ�β��
//
//		2��8���������
//		���Һ����ܶ࣬����Ҳ��ǿ�󣬰����ˣ�
//		find()
//		rfind()
//		find_first_of()
//		find_last_of()
//		find_first_not_of()
//		find_last_not_of()
//
//		��Щ�������ط��������������ַ������ڵĵ�һ���ַ���������û�ҵ�Ŀ��ͷ���npos�����еĺ����Ĳ���˵�����£�
//		��һ�������Ǳ���Ѱ�Ķ��󡣵ڶ������������п��ޣ�ָ��string�ڵ���Ѱ������������������������п��ޣ�ָ����Ѱ���ַ��������Ƚϼ򵥣�����˵�����Ŀ������������������ϸ�Ľ�𡣵�Ȼ������ǿ���STL��Ѱ�ں�������ἰ��
//
//		�����˵˵npos�ĺ��壬string::npos��������string::size_type,���ԣ�һ����Ҫ��һ��������npos��ȣ��������ֵ������string::size)type���͵ģ����������£����ǿ���ֱ�ӰѺ�����npos���бȽϣ��磺if(s.find(��jia��)== string::npos)����
//
//		string��Ĺ��캯����
//		string(const char *s);    //��c�ַ���s��ʼ��
//	string(int n,char c);     //��n���ַ�c��ʼ��
//	���⣬string�໹֧��Ĭ�Ϲ��캯���͸��ƹ��캯������string s1��string s2="hello"��������ȷ��д�����������string̫�����޷����ʱ���׳�length_error�쳣
//
//		string����ַ�������
//		const char &operator[](int n)const;
//	const char &at(int n)const;
//	char &operator[](int n);
//	char &at(int n);
//	operator[]��at()�����ص�ǰ�ַ����е�n���ַ���λ�ã���at�����ṩ��Χ��飬��Խ��ʱ���׳�out_of_range�쳣���±������[]���ṩ�����ʡ�
//		const char *data()const;//����һ����null��ֹ��c�ַ�����
//	const char *c_str()const;//����һ����null��ֹ��c�ַ���
//	int copy(char *s, int n, int pos = 0) const;//�ѵ�ǰ������pos��ʼ��n���ַ���������sΪ��ʼλ�õ��ַ������У�����ʵ�ʿ�������Ŀ
//
//string����������:
//	int capacity()const;    //���ص�ǰ��������string�в��������ڴ漴�ɴ�ŵ�Ԫ�ظ�����
//	int max_size()const;    //����string�����пɴ�ŵ�����ַ����ĳ���
//	int size()const;        //���ص�ǰ�ַ����Ĵ�С
//	int length()const;       //���ص�ǰ�ַ����ĳ���
//	bool empty()const;        //��ǰ�ַ����Ƿ�Ϊ��
//	void resize(int len,char c);//���ַ�����ǰ��С��Ϊlen�������ַ�c��䲻��Ĳ���
//
//string��������������:
//	string�����������operator>>�������룬ͬ�����������operator<<�������������
//		����getline(istream &in,string &s);���ڴ�������in�ж�ȡ�ַ�����s�У��Ի��з�'\n'�ֿ���
//
//		string�ĸ�ֵ��
//		string &operator=(const string &s);//���ַ���s������ǰ�ַ���
//	string &assign(const char *s);//��c�����ַ���s��ֵ
//	string &assign(const char *s,int n);//��c�ַ���s��ʼ��n���ַ���ֵ
//	string &assign(const string &s);//���ַ���s������ǰ�ַ���
//	string &assign(int n,char c);//��n���ַ�c��ֵ����ǰ�ַ���
//	string &assign(const string &s,int start,int n);//���ַ���s�д�start��ʼ��n���ַ�������ǰ�ַ���
//	string &assign(const_iterator first,const_itertor last);//��first��last������֮��Ĳ��ָ����ַ���
//
//	string�����ӣ�
//		string &operator+=(const string &s);//���ַ���s���ӵ���ǰ�ַ����Ľ�β 
//	string &append(const char *s);            //��c�����ַ���s���ӵ���ǰ�ַ�����β
//	string &append(const char *s,int n);//��c�����ַ���s��ǰn���ַ����ӵ���ǰ�ַ�����β
//	string &append(const string &s);    //ͬoperator+=()
//	string &append(const string &s,int pos,int n);//���ַ���s�д�pos��ʼ��n���ַ����ӵ���ǰ�ַ����Ľ�β
//	string &append(int n,char c);        //�ڵ�ǰ�ַ�����β���n���ַ�c
//	string &append(const_iterator first,const_iterator last);//�ѵ�����first��last֮��Ĳ������ӵ���ǰ�ַ����Ľ�β
//
//	string�ıȽϣ�
//		bool operator==(const string &s1,const string &s2)const;//�Ƚ������ַ����Ƿ����
//	�����">","<",">=","<=","!="�������������ַ����ıȽϣ�
//		int compare(const string &s) const;//�Ƚϵ�ǰ�ַ�����s�Ĵ�С
//	int compare(int pos, int n,const string &s)const;//�Ƚϵ�ǰ�ַ�����pos��ʼ��n���ַ���ɵ��ַ�����s�Ĵ�С
//	int compare(int pos, int n,const string &s,int pos2,int n2)const;//�Ƚϵ�ǰ�ַ�����pos��ʼ��n���ַ���ɵ��ַ�����s��pos2��ʼ��n2���ַ���ɵ��ַ����Ĵ�С
//	int compare(const char *s) const;
//	int compare(int pos, int n,const char *s) const;
//	int compare(int pos, int n,const char *s, int pos2) const;
//	compare������>ʱ����1��<ʱ����-1��==ʱ����0   
//		string���Ӵ���
//		string substr(int pos = 0,int n = npos) const;//����pos��ʼ��n���ַ���ɵ��ַ���
//
//	string�Ľ�����
//		void swap(string &s2);    //������ǰ�ַ�����s2��ֵ
//
//	string��Ĳ��Һ����� 
//		int find(char c, int pos = 0) const;//��pos��ʼ�����ַ�c�ڵ�ǰ�ַ�����λ��
//	int find(const char *s, int pos = 0) const;//��pos��ʼ�����ַ���s�ڵ�ǰ���е�λ��
//	int find(const char *s, int pos, int n) const;//��pos��ʼ�����ַ���s��ǰn���ַ��ڵ�ǰ���е�λ��
//	int find(const string &s, int pos = 0) const;//��pos��ʼ�����ַ���s�ڵ�ǰ���е�λ��
//	//���ҳɹ�ʱ��������λ�ã�ʧ�ܷ���string::npos��ֵ 
//	int rfind(char c, int pos = npos) const;//��pos��ʼ�Ӻ���ǰ�����ַ�c�ڵ�ǰ���е�λ��
//	int rfind(const char *s, int pos = npos) const;
//	int rfind(const char *s, int pos, int n = npos) const;
//	int rfind(const string &s,int pos = npos) const;
//	//��pos��ʼ�Ӻ���ǰ�����ַ���s��ǰn���ַ���ɵ��ַ����ڵ�ǰ���е�λ�ã��ɹ���������λ�ã�ʧ��ʱ����string::npos��ֵ 
//	int find_first_of(char c, int pos = 0) const;//��pos��ʼ�����ַ�c��һ�γ��ֵ�λ��
//	int find_first_of(const char *s, int pos = 0) const;
//	int find_first_of(const char *s, int pos, int n) const;
//	int find_first_of(const string &s,int pos = 0) const;
//	//��pos��ʼ���ҵ�ǰ���е�һ����s��ǰn���ַ���ɵ���������ַ���λ�á�����ʧ�ܷ���string::npos 
//	int find_first_not_of(char c, int pos = 0) const;
//	int find_first_not_of(const char *s, int pos = 0) const;
//	int find_first_not_of(const char *s, int pos,int n) const;
//	int find_first_not_of(const string &s,int pos = 0) const;
//	//�ӵ�ǰ���в��ҵ�һ�����ڴ�s�е��ַ����ֵ�λ�ã�ʧ�ܷ���string::npos 
//	int find_last_of(char c, int pos = npos) const;
//	int find_last_of(const char *s, int pos = npos) const;
//	int find_last_of(const char *s, int pos, int n = npos) const;
//	int find_last_of(const string &s,int pos = npos) const; 
//	int find_last_not_of(char c, int pos = npos) const;
//	int find_last_not_of(const char *s, int pos = npos) const;
//	int find_last_not_of(const char *s, int pos, int n) const;
//	int find_last_not_of(const string &s,int pos = npos) const;
//	//find_last_of��find_last_not_of��find_first_of��find_first_not_of���ƣ�ֻ�����ǴӺ���ǰ����
//
//	string����滻������ 
//		string &replace(int p0, int n0,const char *s);//ɾ����p0��ʼ��n0���ַ���Ȼ����p0�����봮s
//	string &replace(int p0, int n0,const char *s, int n);//ɾ��p0��ʼ��n0���ַ���Ȼ����p0�������ַ���s��ǰn���ַ�
//	string &replace(int p0, int n0,const string &s);//ɾ����p0��ʼ��n0���ַ���Ȼ����p0�����봮s
//	string &replace(int p0, int n0,const string &s, int pos, int n);//ɾ��p0��ʼ��n0���ַ���Ȼ����p0�����봮s�д�pos��ʼ��n���ַ�
//	string &replace(int p0, int n0,int n, char c);//ɾ��p0��ʼ��n0���ַ���Ȼ����p0������n���ַ�c
//	string &replace(iterator first0, iterator last0,const char *s);//��[first0��last0��֮��Ĳ����滻Ϊ�ַ���s
//	string &replace(iterator first0, iterator last0,const char *s, int n);//��[first0��last0��֮��Ĳ����滻Ϊs��ǰn���ַ�
//	string &replace(iterator first0, iterator last0,const string &s);//��[first0��last0��֮��Ĳ����滻Ϊ��s
//	string &replace(iterator first0, iterator last0,int n, char c);//��[first0��last0��֮��Ĳ����滻Ϊn���ַ�c
//	string &replace(iterator first0, iterator last0,const_iterator first, const_iterator last);//��[first0��last0��֮��Ĳ����滻��[first��last��֮����ַ���
//
//	string��Ĳ��뺯���� 
//		string &insert(int p0, const char *s);
//	string &insert(int p0, const char *s, int n);
//	string &insert(int p0,const string &s);
//	string &insert(int p0,const string &s, int pos, int n);
//	//ǰ4��������p0λ�ò����ַ���s��pos��ʼ��ǰn���ַ�
//	string &insert(int p0, int n, char c);//�˺�����p0������n���ַ�c
//	iterator insert(iterator it, char c);//��it�������ַ�c�����ز�����������λ��
//	void insert(iterator it, const_iterator first, const_iterator last);//��it������[first��last��֮����ַ�
//	void insert(iterator it, int n, char c);//��it������n���ַ�c
//
//	string���ɾ������ 
//		iterator erase(iterator first, iterator last);//ɾ��[first��last��֮��������ַ�������ɾ�����������λ��
//	iterator erase(iterator it);//ɾ��itָ����ַ�������ɾ�����������λ��
//	string &erase(int pos = 0, int n = npos);//ɾ��pos��ʼ��n���ַ��������޸ĺ���ַ���
//
//	string��ĵ��������� 
//		string���ṩ����ǰ���������ĵ�����iterator���������ṩ�˷��ʸ����ַ����﷨��������ָ�����������������鷶Χ��
//
//		��string::iterator��string::const_iterator����������������const_iterator������ı���������ݡ����õ����������У�
//		const_iterator begin()const;
//	iterator begin();                //����string����ʼλ��
//	const_iterator end()const;
//	iterator end();                    //����string�����һ���ַ������λ��
//	const_iterator rbegin()const;
//	iterator rbegin();                //����string�����һ���ַ���λ��
//	const_iterator rend()const;
//	iterator rend();                    //����string��һ���ַ�λ�õ�ǰ��
//	rbegin��rend���ڴӺ���ǰ�ĵ������ʣ�ͨ�����õ�����string::reverse_iterator,string::const_reverse_iteratorʵ��
//
//		�ַ��������� 
//		ͨ������ostringstream��istringstream����ʵ�֣�<sstream>ͷ�ļ���
//		���磺
//		string input("hello,this is a test");
//	istringstream is(input);
//	string s1,s2,s3,s4;
//	is>>s1>>s2>>s3>>s4;//s1="hello,this",s2="is",s3="a",s4="test"
//	ostringstream os;
//	os<<s1<<s2<<s3<<s4;
//	cout<<os.str();
//