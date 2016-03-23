#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;

int main()
{
	char * floderPath = "E:\\Action\\testdataset\\纯色\\";
	//获取时间作为文件名
	char strTime[256];
	time_t CurTime = time(NULL); //获取系统时间，单位为秒;
	tm timeinfo;
	localtime_s(&timeinfo,&CurTime);
	sprintf( &strTime[0], "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d.txt",  
           timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,  
             timeinfo.tm_hour, timeinfo.tm_min,timeinfo.tm_sec);
	//strftime(strTime, 256, "%Y%m%d%H%M%S", &timeinfo);
	char fileName[256];
	strcpy_s(&fileName[0],256,floderPath);
	strcat(&fileName[0],&strTime[0]);
	fstream txtFile;
	txtFile.open(&fileName[0],fstream::in | fstream::out| fstream::binary | fstream::app);
	txtFile<<&strTime[0]<<endl;
	//txtFile.write(&strTime[0],256);
	txtFile.close();

}