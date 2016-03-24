//  *************************************************************
//  Copyright (C) 2000-2009,Seuu Technologies Co., Ltd.
//  
//  File name  : SuperLog.h
//  Description: ����Ҫ�Ƚϸߣ�
// ��дchar*��string��bin
// ���̣߳�������д
// ��������һ�Σ�����ģ�
// д��־�ɷּ��𣬿ɵ��صġ�
// ��־�ļ��ж�����ֻ�д

//  Version    : 1.0
//  Author     : Seuu
//  Created    : 2009-05-04 22:54:18
//  *************************************************************

#include <fstream>
#include <iostream>
#include <string>
#include <tchar.h>
#include <windows.h>
using namespace std; 

const int MAX_BIN_LOG_INFO_LEN = 30*1024+1;
const int MAX_STR_LOG_INFO_LEN = 3*1024+1;
const int MAX_LOG_FILE_LEN     = 5*1024*1024;

// ��־�ļ�������д���ٸ�
#define   MAX_LOG_FILE_COUNT   3
#define   WRITE_LOG           CSuperLog::WriteLog
#define   LOG_LEVEL_DEBUG     CSuperLog::ENUM_LOG_LEVEL_DEBUG
#define   LOG_LEVEL_RUN       CSuperLog::ENUM_LOG_LEVEL_RUN
#define   LOG_LEVEL_ERROR     CSuperLog::ENUM_LOG_LEVEL_ERROR
#define   WELCOME_LOG_INFOL   _T("\r\nSuper Logger    Version 1.0\r\n")

class CSuperLog
{
private:
    // ��־д������״̬
    enum enLogStatus
    {
        ENUM_LOG_INIT, 
        ENUM_LOG_RUN, 
        ENUM_LOG_EXITING, 
        ENUM_LOG_EXITED, 
        ENUM_LOG_INVALID,
    };

    static CRITICAL_SECTION m_csWriteLog;         // д��־���ٽ����
    static int              m_iCurLogFileSeq;     // ��ǰ����д����ļ����к�
	static string           m_strWriteStrInfoL;    // ��д��str
    static int              m_iWriteBinLogLen;    // ��ǰ��д���bin��־�ֽڳ���
	static fstream          m_pFileL;              // �ļ�ָ��
    static int              m_iMaxLogFileLen;     // �����ļ���󳤶�

    static HANDLE           m_hThread;    // �߳̾��
    static unsigned         m_uiThreadID;  // �߳�id
    static enLogStatus      m_enStatus;    // ��ǰlogд������״̬
    static bool             m_bRun;      // ��ǰlogд�����Ƿ����
    static int              m_iLogLevel; // ��ǰд��־�ļ���
    static HANDLE           m_hMapLogFile; //ӳ�䵽�����ڴ���
    static LPTSTR           m_psMapAddr;   //ӳ�䵽�����ڴ浽�����ڴ�ĵ�ַ

public:

    // ��־����
    enum enLogInfoLevel
    {
        ENUM_LOG_LEVEL_DEBUG = 0, 
        ENUM_LOG_LEVEL_RUN, 
        ENUM_LOG_LEVEL_ERROR, 
    };   
    CSuperLog(void);
    ~CSuperLog(void);

	static DWORD WINAPI CSuperLog::LogProcStart( LPVOID  args );
    static enLogStatus OpenLogFile();   
	static int         WriteLogToFileL();  
	static string&     GetCurTimeStrL();
	static int         WriteUnicodeHeadToFileL(fstream& pFileL); 
    static int         GetLogLevelShareMem();              // ȡ�õ�ǰ����־����
    static int         SetLogLevelShareMem(int iLevel);    // ���õ�ǰ����־����
    static int         OperaterConfig(BOOL bSave);         // �ѵ�ǰ����д�������ļ���
    static int         CheckLogLevel();                    // �ӹ����ڴ��ж����ò����õ�ǰ����־����

    // ���ز�ͬ������Ϣ��д��־����
	// debug���debug�汾��־�ļ���д
#ifdef _DEBUG
	static int WriteLog(string &strLog, enLogInfoLevel enLevel = ENUM_LOG_LEVEL_DEBUG, bool bForce = true);
    static int WriteLog(TCHAR* pstrLog,  enLogInfoLevel enLevel = ENUM_LOG_LEVEL_DEBUG, bool bForce = true);
    //static int WriteLog(char* pszLog, enLogInfoLevel enLevel = ENUM_LOG_LEVEL_DEBUG, bool bForce = true);
#else
    static int WriteLog(string &strLog, enLogInfoLevel enLevel = ENUM_LOG_LEVEL_RUN, bool bForce = true);
	static int WriteLog(TCHAR* pstrLog,  enLogInfoLevel enLevel = ENUM_LOG_LEVEL_RUN, bool bForce = true);
    //static int WriteLog(char* pszLog, enLogInfoLevel enLevel = ENUM_LOG_LEVEL_DEBUG, bool bForce = true);
#endif
};
