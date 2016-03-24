//  *************************************************************
//  Copyright (C) 2000-2009,Seuu Technologies Co., Ltd.
//  
//  File name  : SuperLog.h
//  Description: 性能要比较高，
// 能写char*，string，bin
// 有线程，能主动写
// 三到四秒一次，可配的，
// 写日志可分级别，可调控的。
// 日志文件有多个，轮换写

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

// 日志文件最多可以写多少个
#define   MAX_LOG_FILE_COUNT   3
#define   WRITE_LOG           CSuperLog::WriteLog
#define   LOG_LEVEL_DEBUG     CSuperLog::ENUM_LOG_LEVEL_DEBUG
#define   LOG_LEVEL_RUN       CSuperLog::ENUM_LOG_LEVEL_RUN
#define   LOG_LEVEL_ERROR     CSuperLog::ENUM_LOG_LEVEL_ERROR
#define   WELCOME_LOG_INFOL   _T("\r\nSuper Logger    Version 1.0\r\n")

class CSuperLog
{
private:
    // 日志写入器的状态
    enum enLogStatus
    {
        ENUM_LOG_INIT, 
        ENUM_LOG_RUN, 
        ENUM_LOG_EXITING, 
        ENUM_LOG_EXITED, 
        ENUM_LOG_INVALID,
    };

    static CRITICAL_SECTION m_csWriteLog;         // 写日志的临界变量
    static int              m_iCurLogFileSeq;     // 当前正在写入的文件序列号
	static string           m_strWriteStrInfoL;    // 待写入str
    static int              m_iWriteBinLogLen;    // 当前待写入的bin日志字节长度
	static fstream          m_pFileL;              // 文件指针
    static int              m_iMaxLogFileLen;     // 单个文件最大长度

    static HANDLE           m_hThread;    // 线程句柄
    static unsigned         m_uiThreadID;  // 线程id
    static enLogStatus      m_enStatus;    // 当前log写入器的状态
    static bool             m_bRun;      // 当前log写入器是否结束
    static int              m_iLogLevel; // 当前写日志的级别
    static HANDLE           m_hMapLogFile; //映射到共享内存中
    static LPTSTR           m_psMapAddr;   //映射到共享内存到程序内存的地址

public:

    // 日志级别
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
    static int         GetLogLevelShareMem();              // 取得当前的日志级别
    static int         SetLogLevelShareMem(int iLevel);    // 设置当前的日志级别
    static int         OperaterConfig(BOOL bSave);         // 把当前级别写到配置文件中
    static int         CheckLogLevel();                    // 从共享内存中读配置并设置当前的日志级别

    // 重载不同类型消息的写日志函数
	// debug与非debug版本日志文件书写
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
