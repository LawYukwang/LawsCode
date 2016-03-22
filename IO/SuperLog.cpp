//  *************************************************************
//  Copyright (C) 2000-2009,Seuu Technologies Co., Ltd.
//  
//  File name  : SuperLog.cpp
//  Description: 日志写入器实现
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
// 类静态变量初始化
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

// 这里已经构造了全局函数
CSuperLog g_SuperLog;

// string 转换成 lpcwstr
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
// 在多字节集情况下，该函数不用；
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
	WriteLog(_T("日志线程启动."), ENUM_LOG_LEVEL_DEBUG, true);
	//WriteLog(_T("日志线程启动."), ENUM_LOG_LEVEL_RUN, true);
	// 线程开始
	do 
	{
		Sleep(300);
		if (++nCount % 10 == 0 )
		{
			//WriteLog(strTemp, ENUM_LOG_LEVEL_ERROR, true); // 每隔三秒写一次日志
			WriteLog(strTempL, ENUM_LOG_LEVEL_ERROR, true); // 每隔三秒写一次日志
			CheckLogLevel();
		}

		if (nCount % 40 == 0 || m_enStatus == ENUM_LOG_INVALID)
		{
			// 每12秒检查一次文件
			if (ENUM_LOG_INVALID ==  OpenLogFile())
			{
				Sleep(3000);
				OpenLogFile();
			}
		}

	} while (m_bRun);

	// 程序开始退出
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
	// 初始化临界区变量
	InitializeCriticalSection(&m_csWriteLog); 
	// 打开一个日志文件
	OpenLogFile();
	// 启动信息
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
	// 从配置文件中读取日志级别
	OperaterConfig(FALSE);
	// 同步到共离内存中
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
		WRITE_LOG(_T("GetModuleFileName失败。"), LOG_LEVEL_ERROR);
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
	//    WRITE_LOG(_T("GetModuleFileName取得信息异常。"), LOG_LEVEL_ERROR);
	//    return -1;
	//}


	if (nPosL == -1)
	{
		WRITE_LOG(_T("GetModuleFileName取得信息异常。"), LOG_LEVEL_ERROR);
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
			WriteLog(_T("日志级别配置非法。"),LOG_LEVEL_ERROR);
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
			WriteLog(_T("最大文件长度配置非法。"),LOG_LEVEL_ERROR);
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
		WriteLog(_T("设置日志级别成功。"), ENUM_LOG_LEVEL_RUN);
	}
	return 0;
}

int CSuperLog::GetLogLevelShareMem(void)
{
	//打开共享的文件对象。
	if (m_hMapLogFile == NULL)
	{
		m_hMapLogFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _T("SuperLogShareMem"));
	}
	if (m_hMapLogFile != NULL)
	{
		//显示共享的文件数据。
		if (m_psMapAddr == NULL)
		{
			m_psMapAddr = (LPTSTR)MapViewOfFile(m_hMapLogFile, FILE_MAP_ALL_ACCESS,0,0,0);
			WriteLog(m_psMapAddr, ENUM_LOG_LEVEL_DEBUG);
		}
	}
	else
	{
		//创建共享文件。
		m_hMapLogFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,1024, _T("SuperLogShareMem"));
		if (m_hMapLogFile != NULL)
		{
			//拷贝数据到共享文件里。
			m_psMapAddr = (LPTSTR)MapViewOfFile(m_hMapLogFile,FILE_MAP_ALL_ACCESS, 0,0,0);
			if (m_psMapAddr != NULL)
			{
				_tcscpy_s(m_psMapAddr, 1024, g_pszLogLevel[m_iLogLevel]);                  
				FlushViewOfFile(m_psMapAddr, _tcslen(g_pszLogLevel[m_iLogLevel]));
				WriteLog(_T("设置默认日志级别到共享内存中成功。"), ENUM_LOG_LEVEL_RUN);

			}
		}
		else
		{
			WriteLog(_T("创建共享内存失败。"), ENUM_LOG_LEVEL_ERROR);
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
	// 写入过程中出错后需要关闭文件
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
			//	// 测试成功
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
			// 上一个文件是最大的那个文件或是写过一遍了的。
			if (m_iCurLogFileSeq >= MAX_LOG_FILE_COUNT) 
			{
				// 所有文件都是写满了，则强制从第一个文件开始写，同时先清空文件
				//bRet = m_pFile->Open(
				//    g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT],
				//    CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyNone); 

				m_pFileL.open(g_pszLogFileName[(m_iCurLogFileSeq++)%MAX_LOG_FILE_COUNT], fstream::in | fstream::out| fstream::binary | fstream::app);
			}
			else
			{
				// 打开第二个文件，再检查是否过了最大值
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
// 待实现
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
		// 这个操作是有问题的
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
//		标准c++中string类函数介绍
//
//		注意不是CString
//		之所以抛弃char*的字符串而选用C++标准程序库中的string类，是因为他和前者比较起来，
//      不必担心内存是否足够、字符串长度等等，而且作为一个类出现，他集成的操作函数足以完成我们大多数情况下(甚至是100%)的需要。
//      我们可以用 = 进行赋值操作，== 进行比较，+ 做串联（是不是很简单?）。我们尽可以把它看成是C++的基本数据类型。
//
//		好了，进入正题………
//		首先，为了在我们的程序中使用string类型，我们必须包含头文件 <string>。
//
//		如下：
//#include <string> //注意这里不是string.h string.h是C字符串头文件
//#include <string>
//		using namespace std;
//
//	1．声明一个C++字符串
//		声明一个字符串变量很简单：
//		string Str;
//	这样我们就声明了一个字符串变量，但既然是一个类，就有构造函数和析构函数。上面的声明没有传入参数，所以就直接使用了string的默认的构造函数，这个函数所作的就是把Str初始化为一个空字符串。String类的构造函数和析构函数如下：
//		a)      string s;    //生成一个空字符串s
//	b)      string s(str) //拷贝构造函数 生成str的复制品
//		c)      string s(str,stridx) //将字符串str内“始于位置stridx”的部分当作字符串的初值
//		d)      string s(str,stridx,strlen) //将字符串str内“始于stridx且长度顶多strlen”的部分作为字符串的初值
//		e)      string s(cstr) //将C字符串作为s的初值
//		f)      string s(chars,chars_len) //将C字符串前chars_len个字符作为字符串s的初值。
//		g)      string s(num,c) //生成一个字符串，包含num个c字符
//		h)      string s(beg,end) //以区间beg;end(不包含end)内的字符作为字符串s的初值
//		i)      s.~string() //销毁所有字符，释放内存
//		都很简单，我就不解释了。
//
//		2．字符串操作函数
//		这里是C++字符串的重点，我先把各种操作函数罗列出来，不喜欢把所有函数都看完的人可以在这里找自己喜欢的函数，再到后面看他的详细解释。
//		a) =,assign()     //赋以新值
//		b) swap()     //交换两个字符串的内容
//		c) +=,append(),push_back() //在尾部添加字符
//		d) insert() //插入字符
//		e) erase() //删除字符
//		f) clear() //删除全部字符
//		g) replace() //替换字符
//		h) + //串联字符串
//		i) ==,!=,<,<=,>,>=,compare()    //比较字符串
//		j) size(),length()    //返回字符数量
//		k) max_size() //返回字符的可能最大个数
//		l) empty()    //判断字符串是否为空
//		m) capacity() //返回重新分配之前的字符容量
//		n) reserve() //保留一定量内存以容纳一定数量的字符
//		o) [ ], at() //存取单一字符
//		p) >>,getline() //从stream读取某值
//		q) <<    //将谋值写入stream
//		r) copy() //将某值赋值为一个C_string
//		s) c_str() //将内容以C_string返回
//		t) data() //将内容以字符数组形式返回
//		u) substr() //返回某个子字符串
//		v)查找函数
//		w)begin() end() //提供类似STL的迭代器支持
//		x) rbegin() rend() //逆向迭代器
//		y) get_allocator() //返回配置器
//
//		下面详细介绍：
//
//		2．1 C++字符串和C字符串的转换
//		C ++提供的由C++字符串得到对应的C_string的方法是使用data()、c_str()和copy()，其中，
//      data()以字符数组的形式返回字符串内容，但并不添加'/0'。
//      c_str()返回一个以‘/0'结尾的字符数组，
//      而copy()则把字符串的内容复制或写入既有的c_string或 字符数组内。C++字符串并不以'/0'结尾。
//      我的建议是在程序中能使用C++字符串就使用，除非万不得已不选用c_string。由于只是简单介绍，详细介绍掠过，谁想进一步了解使用中的注意事项可以给我留言(到我的收件箱)。我详细解释。
//
//		2．2 大小和容量函数
//		一个C++字符串存在三种大小：
//      a)现有的字符数，函数是size()和length()，他们等效。Empty()用来检查字符串是否为空。
//      b)max_size() 这个大小是指当前C++字符串最多能包含的字符数，很可能和机器本身的限制或者字符串所在位置连续内存的大小有关系。我们一般情况下不用关心他，应该大小足够我们用的。但是不够用的话，会抛出length_error异常
//      c)capacity()重新分配内存之前 string所能包含的最大字符数。这里另一个需要指出的是reserve()函数，这个函数为string重新分配内存。重新分配的大小由其参数决定， 默认参数为0，这时候会对string进行非强制性缩减。
//
//		还有必要再重复一下C++字符串和C字符串转换的问题，许多人会遇到这样的问题，
//      自己做的程序要调用别人的函数、类什么的（比如数据库连接函数Connect(char*,char*)），
//      但别人的函数参 数用的是char*形式的，而我们知道，c_str()、data()返回的字符数组由该字符串拥有，
//   	所以是一种const char*,要想作为上面提及的函数的参数，还必须拷贝到一个char*,而我们的原则是能不使用C字符串就不使用。
//  	那么，这时候我们的处理方式是：如果 此函数对参数(也就是char*)的内容不修改的话，我们可以这样Connect((char*)UserID.c_str(), (char*)PassWD.c_str()),
//      但是这时候是存在危险的，因为这样转换后的字符串其实是可以修改的（有兴趣地可以自己试一试），
//      所以我强调除非函数调用的时候不对参数进行修改，否则必须拷贝到一个char*上去。
//  	当然，更稳妥的办法是无论什么情况都拷贝到一个char*上去。
//  	同时我们也祈祷现在仍然使用C字符串进行编程的高手们（说他们是高手一点儿也不为过，也许在我们还穿开裆裤的时候他们就开始编程了，哈哈…）写的函数都比较规范，那样我们就不必进行强制转换了。
//
//		2．3元素存取
//		我们可以使用下标操作符[]和函数at()对元素包含的字符进行访问。但是应该注意的是操作符[]并不检查索引是否有效（有效索引0~str.length()），如果索引失效，会引起未定义的行为。而at()会检查，如果使用 at()的时候索引无效，会抛出out_of_range异常。
//
//		有一个例外不得不说，const string a;的操作符[]对索引值是a.length()仍然有效，其返回值是'/0'。其他的各种情况，a.length()索引都是无效的。举例如下：
//		const string Cstr(“const string”);
//	string Str(“string”);
//	Str[3];      //ok
//	Str.at(3);    //ok
//	Str[100]; //未定义的行为
//	Str.at(100);    //throw out_of_range
//	Str[Str.length()]    //未定义行为
//	Cstr[Cstr.length()] //返回 ‘/0'
//	Str.at(Str.length());//throw out_of_range
//	Cstr.at(Cstr.length()) ////throw out_of_range
//		我不赞成类似于下面的引用或指针赋值：
//		char& r=s[2];
//	char* p= &s[3];
//	因为一旦发生重新分配，r,p立即失效。避免的方法就是不使用。
//
//		2．4比较函数
//		C ++字符串支持常见的比较操作符（>,>=,<,<=,==,!=），甚至支持string与C-string的比较(如 str<”hello”)。
//      在使用>,>=,<,<=这些操作符的时候是根据“当前字符特性”将字符按字典顺序进行逐一得 比较。
//  	字典排序靠前的字符小，比较的顺序是从前向后比较，遇到不相等的字符就按这个位置上的两个字符的比较结果确定两个字符串的大小。同时，string (“aaaa”) <string(aaaaa)。
//
//		另一个功能强大的比较函数是成员函数compare()。他支持多参数处理，支持用索引值和长度定位子串来进行比较。他返回一个整数来表示比较结果，返回值意义如下：0-相等 〉0-大于 <0-小于。举例如下：
//		string s(“abcd”);
//	s.compare(“abcd”); //返回0
//	s.compare(“dcba”); //返回一个小于0的值
//	s.compare(“ab”); //返回大于0的值
//	s.compare(s); //相等
//	s.compare(0,2,s,2,2); //用”ab”和”cd”进行比较 小于零
//	s.compare(1,2,”bcx”,2); //用”bc”和”bc”比较。
//	怎么样？功能够全的吧！什么？还不能满足你的胃口？好吧，那等着，后面有更个性化的比较算法。先给个提示，使用的是STL的比较算法。什么？对STL一窍不通？靠，你重修吧！
//
//		2．5 更改内容
//		这在字符串的操作中占了很大一部分。
//		首先讲赋值，第一个赋值方法当然是使用操作符=，新值可以是string(如：s=ns) 、c_string(如：s=”gaint”)甚至单一字符（如：s='j'）。还可以使用成员函数assign()，这个成员函数可以使你更灵活的对字符串赋值。还是举例说明吧：
//		s.assign(str); //不说
//	s.assign(str,1,3);//如果str是”iamangel” 就是把”ama”赋给字符串
//	s.assign(str,2,string::npos);//把字符串str从索引值2开始到结尾赋给s
//	s.assign(“gaint”); //不说
//	s.assign(“nico”,5);//把'n' ‘I' ‘c' ‘o' ‘/0'赋给字符串
//	s.assign(5,'x');//把五个x赋给字符串
//	把字符串清空的方法有三个：s=””;s.clear();s.erase();(我越来越觉得举例比说话让别人容易懂！)。
//		string提供了很多函数用于插入（insert）、删除（erase）、替换（replace）、增加字符。
//		先说增加字符（这里说的增加是在尾巴上），函数有 +=、append()、push_back()。
//
//		举例如下：
//		s+=str;//加个字符串
//	s+=”my name is jiayp”;//加个C字符串
//	s+='a';//加个字符
//	s.append(str);
//	s.append(str,1,3);//不解释了 同前面的函数参数assign的解释
//	s.append(str,2,string::npos)//不解释了
//		s.append(“my name is jiayp”);
//	s.append(“nico”,5);
//	s.append(5,'x');
//	s.push_back(‘a');//这个函数只能增加单个字符对STL熟悉的理解起来很简单
//
//		也许你需要在string中间的某个位置插入字符串，这时候你可以用insert()函数，这个函数需要你指定一个安插位置的索引，被插入的字符串将放在这个索引的后面。
//		s.insert(0,”my name”);
//	s.insert(1,str);
//	这种形式的insert()函数不支持传入单个字符，这时的单个字符必须写成字符串形式(让人恶心)。
//  既然你觉得恶心，那就不得不继续读下面一段话：为了插 入单个字符，
//	insert()函数提供了两个对插入单个字符操作的重载函数：insert(size_type index,size_type num,chart c)
//	和insert(iterator pos,size_type num,chart c)。其中size_type是无符号整数，iterator是char*,
//	所以，你这么调用insert函数是不行的：insert(0,1, 'j');
//  这时候第一个参数将转换成哪一个呢？所以你必须这么写：insert((string::size_type)0,1,'j')！
//	第二种形式指 出了使用迭代器安插字符的形式，在后面会提及。顺便提一下，string有很多操作是使用STL的迭代器的，他也尽量做得和STL靠近。
//
//		删除函数erase()的形式也有好几种（真烦！），替换函数replace()也有好几个。
//
//		举例吧：
//		string s=”il8n”;
//	s.replace(1,2,”nternationalizatio”);//从索引1开始的2个替换成后面的C_string
//	s.erase(13);//从索引13开始往后全删除
//	s.erase(7,5);//从索引7开始往后删5个
//
//	2．6提取子串和字符串连接
//		题取子串的函数是：substr(),形式如下：
//		s.substr();//返回s的全部内容
//	s.substr(11);//从索引11往后的子串
//	s.substr(5,6);//从索引5开始6个字符
//	把两个字符串结合起来的函数是+。（谁不明白请致电120）
//
//		2．7输入输出操作
//		1．>> 从输入流读取一个string。
//		2．<< 把一个string写入输出流。
//		另一个函数就是getline(),他从输入流读取一行内容，直到遇到分行符或到了文件尾。
//
//		2．8搜索与查找
//		查找函数很多，功能也很强大，包括了：
//		find()
//		rfind()
//		find_first_of()
//		find_last_of()
//		find_first_not_of()
//		find_last_not_of()
//
//		这些函数返回符合搜索条件的字符区间内的第一个字符的索引，没找到目标就返回npos。所有的函数的参数说明如下：
//		第一个参数是被搜寻的对象。第二个参数（可有可无）指出string内的搜寻起点索引，第三个参数（可有可无）指出搜寻的字符个数。比较简单，不多说不理解的可以向我提出，我再仔细的解答。当然，更加强大的STL搜寻在后面会有提及。
//
//		最后再说说npos的含义，string::npos的类型是string::size_type,所以，一旦需要把一个索引与npos相比，这个索引值必须是string::size)type类型的，更多的情况下，我们可以直接把函数和npos进行比较（如：if(s.find(“jia”)== string::npos)）。
//
//		string类的构造函数：
//		string(const char *s);    //用c字符串s初始化
//	string(int n,char c);     //用n个字符c初始化
//	此外，string类还支持默认构造函数和复制构造函数，如string s1；string s2="hello"；都是正确的写法。当构造的string太长而无法表达时会抛出length_error异常
//
//		string类的字符操作：
//		const char &operator[](int n)const;
//	const char &at(int n)const;
//	char &operator[](int n);
//	char &at(int n);
//	operator[]和at()均返回当前字符串中第n个字符的位置，但at函数提供范围检查，当越界时会抛出out_of_range异常，下标运算符[]不提供检查访问。
//		const char *data()const;//返回一个非null终止的c字符数组
//	const char *c_str()const;//返回一个以null终止的c字符串
//	int copy(char *s, int n, int pos = 0) const;//把当前串中以pos开始的n个字符拷贝到以s为起始位置的字符数组中，返回实际拷贝的数目
//
//string的特性描述:
//	int capacity()const;    //返回当前容量（即string中不必增加内存即可存放的元素个数）
//	int max_size()const;    //返回string对象中可存放的最大字符串的长度
//	int size()const;        //返回当前字符串的大小
//	int length()const;       //返回当前字符串的长度
//	bool empty()const;        //当前字符串是否为空
//	void resize(int len,char c);//把字符串当前大小置为len，并用字符c填充不足的部分
//
//string类的输入输出操作:
//	string类重载运算符operator>>用于输入，同样重载运算符operator<<用于输出操作。
//		函数getline(istream &in,string &s);用于从输入流in中读取字符串到s中，以换行符'\n'分开。
//
//		string的赋值：
//		string &operator=(const string &s);//把字符串s赋给当前字符串
//	string &assign(const char *s);//用c类型字符串s赋值
//	string &assign(const char *s,int n);//用c字符串s开始的n个字符赋值
//	string &assign(const string &s);//把字符串s赋给当前字符串
//	string &assign(int n,char c);//用n个字符c赋值给当前字符串
//	string &assign(const string &s,int start,int n);//把字符串s中从start开始的n个字符赋给当前字符串
//	string &assign(const_iterator first,const_itertor last);//把first和last迭代器之间的部分赋给字符串
//
//	string的连接：
//		string &operator+=(const string &s);//把字符串s连接到当前字符串的结尾 
//	string &append(const char *s);            //把c类型字符串s连接到当前字符串结尾
//	string &append(const char *s,int n);//把c类型字符串s的前n个字符连接到当前字符串结尾
//	string &append(const string &s);    //同operator+=()
//	string &append(const string &s,int pos,int n);//把字符串s中从pos开始的n个字符连接到当前字符串的结尾
//	string &append(int n,char c);        //在当前字符串结尾添加n个字符c
//	string &append(const_iterator first,const_iterator last);//把迭代器first和last之间的部分连接到当前字符串的结尾
//
//	string的比较：
//		bool operator==(const string &s1,const string &s2)const;//比较两个字符串是否相等
//	运算符">","<",">=","<=","!="均被重载用于字符串的比较；
//		int compare(const string &s) const;//比较当前字符串和s的大小
//	int compare(int pos, int n,const string &s)const;//比较当前字符串从pos开始的n个字符组成的字符串与s的大小
//	int compare(int pos, int n,const string &s,int pos2,int n2)const;//比较当前字符串从pos开始的n个字符组成的字符串与s中pos2开始的n2个字符组成的字符串的大小
//	int compare(const char *s) const;
//	int compare(int pos, int n,const char *s) const;
//	int compare(int pos, int n,const char *s, int pos2) const;
//	compare函数在>时返回1，<时返回-1，==时返回0   
//		string的子串：
//		string substr(int pos = 0,int n = npos) const;//返回pos开始的n个字符组成的字符串
//
//	string的交换：
//		void swap(string &s2);    //交换当前字符串与s2的值
//
//	string类的查找函数： 
//		int find(char c, int pos = 0) const;//从pos开始查找字符c在当前字符串的位置
//	int find(const char *s, int pos = 0) const;//从pos开始查找字符串s在当前串中的位置
//	int find(const char *s, int pos, int n) const;//从pos开始查找字符串s中前n个字符在当前串中的位置
//	int find(const string &s, int pos = 0) const;//从pos开始查找字符串s在当前串中的位置
//	//查找成功时返回所在位置，失败返回string::npos的值 
//	int rfind(char c, int pos = npos) const;//从pos开始从后向前查找字符c在当前串中的位置
//	int rfind(const char *s, int pos = npos) const;
//	int rfind(const char *s, int pos, int n = npos) const;
//	int rfind(const string &s,int pos = npos) const;
//	//从pos开始从后向前查找字符串s中前n个字符组成的字符串在当前串中的位置，成功返回所在位置，失败时返回string::npos的值 
//	int find_first_of(char c, int pos = 0) const;//从pos开始查找字符c第一次出现的位置
//	int find_first_of(const char *s, int pos = 0) const;
//	int find_first_of(const char *s, int pos, int n) const;
//	int find_first_of(const string &s,int pos = 0) const;
//	//从pos开始查找当前串中第一个在s的前n个字符组成的数组里的字符的位置。查找失败返回string::npos 
//	int find_first_not_of(char c, int pos = 0) const;
//	int find_first_not_of(const char *s, int pos = 0) const;
//	int find_first_not_of(const char *s, int pos,int n) const;
//	int find_first_not_of(const string &s,int pos = 0) const;
//	//从当前串中查找第一个不在串s中的字符出现的位置，失败返回string::npos 
//	int find_last_of(char c, int pos = npos) const;
//	int find_last_of(const char *s, int pos = npos) const;
//	int find_last_of(const char *s, int pos, int n = npos) const;
//	int find_last_of(const string &s,int pos = npos) const; 
//	int find_last_not_of(char c, int pos = npos) const;
//	int find_last_not_of(const char *s, int pos = npos) const;
//	int find_last_not_of(const char *s, int pos, int n) const;
//	int find_last_not_of(const string &s,int pos = npos) const;
//	//find_last_of和find_last_not_of与find_first_of和find_first_not_of相似，只不过是从后向前查找
//
//	string类的替换函数： 
//		string &replace(int p0, int n0,const char *s);//删除从p0开始的n0个字符，然后在p0处插入串s
//	string &replace(int p0, int n0,const char *s, int n);//删除p0开始的n0个字符，然后在p0处插入字符串s的前n个字符
//	string &replace(int p0, int n0,const string &s);//删除从p0开始的n0个字符，然后在p0处插入串s
//	string &replace(int p0, int n0,const string &s, int pos, int n);//删除p0开始的n0个字符，然后在p0处插入串s中从pos开始的n个字符
//	string &replace(int p0, int n0,int n, char c);//删除p0开始的n0个字符，然后在p0处插入n个字符c
//	string &replace(iterator first0, iterator last0,const char *s);//把[first0，last0）之间的部分替换为字符串s
//	string &replace(iterator first0, iterator last0,const char *s, int n);//把[first0，last0）之间的部分替换为s的前n个字符
//	string &replace(iterator first0, iterator last0,const string &s);//把[first0，last0）之间的部分替换为串s
//	string &replace(iterator first0, iterator last0,int n, char c);//把[first0，last0）之间的部分替换为n个字符c
//	string &replace(iterator first0, iterator last0,const_iterator first, const_iterator last);//把[first0，last0）之间的部分替换成[first，last）之间的字符串
//
//	string类的插入函数： 
//		string &insert(int p0, const char *s);
//	string &insert(int p0, const char *s, int n);
//	string &insert(int p0,const string &s);
//	string &insert(int p0,const string &s, int pos, int n);
//	//前4个函数在p0位置插入字符串s中pos开始的前n个字符
//	string &insert(int p0, int n, char c);//此函数在p0处插入n个字符c
//	iterator insert(iterator it, char c);//在it处插入字符c，返回插入后迭代器的位置
//	void insert(iterator it, const_iterator first, const_iterator last);//在it处插入[first，last）之间的字符
//	void insert(iterator it, int n, char c);//在it处插入n个字符c
//
//	string类的删除函数 
//		iterator erase(iterator first, iterator last);//删除[first，last）之间的所有字符，返回删除后迭代器的位置
//	iterator erase(iterator it);//删除it指向的字符，返回删除后迭代器的位置
//	string &erase(int pos = 0, int n = npos);//删除pos开始的n个字符，返回修改后的字符串
//
//	string类的迭代器处理： 
//		string类提供了向前和向后遍历的迭代器iterator，迭代器提供了访问各个字符的语法，类似于指针操作，迭代器不检查范围。
//
//		用string::iterator或string::const_iterator声明迭代器变量，const_iterator不允许改变迭代的内容。常用迭代器函数有：
//		const_iterator begin()const;
//	iterator begin();                //返回string的起始位置
//	const_iterator end()const;
//	iterator end();                    //返回string的最后一个字符后面的位置
//	const_iterator rbegin()const;
//	iterator rbegin();                //返回string的最后一个字符的位置
//	const_iterator rend()const;
//	iterator rend();                    //返回string第一个字符位置的前面
//	rbegin和rend用于从后向前的迭代访问，通过设置迭代器string::reverse_iterator,string::const_reverse_iterator实现
//
//		字符串流处理： 
//		通过定义ostringstream和istringstream变量实现，<sstream>头文件中
//		例如：
//		string input("hello,this is a test");
//	istringstream is(input);
//	string s1,s2,s3,s4;
//	is>>s1>>s2>>s3>>s4;//s1="hello,this",s2="is",s3="a",s4="test"
//	ostringstream os;
//	os<<s1<<s2<<s3<<s4;
//	cout<<os.str();
//