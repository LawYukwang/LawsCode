// defect_search.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "defect_search.h"
#include <HalconCpp.h>
#include <list>
#include <tchar.h>
#include <io.h>
#include <fstream>
#include "Log.h"

using namespace Halcon;

//日志对象
CLog logFile;
defect_notify defectNotify = NULL;
image_data_notify imageDataNotify = NULL;
static HANDLE hThreadForDSF = INVALID_HANDLE_VALUE;
static HANDLE hEvent = INVALID_HANDLE_VALUE;

static bool bWantExit = false;
//phase=1表示拍摄模板图像，phase=2表示拍摄非模板图像；
static int defect_phase;
static char strTempletImgDir[MAX_PATH];
static char strNonTempletImgDir[MAX_PATH];
//用于调用sj_op.dll库的变量
static HINSTANCE hInstanceDll = NULL;
static sj_notify_register sjNotifyRegister = NULL;
//用于保存相机传过来的图像数据
static std::list<yb_image_data_s *> imageDataLst;
static std::list<yb_image_data_s *>::iterator iter;
//图像数据处理
static CRITICAL_SECTION imageLock;
static unsigned char * rawMemory;
static unsigned char * dataTransfer;

//测试用
std::list<char *> imageNames;
std::list<char *>::iterator imageIter;
static std::fstream txtFile;
//该数据用于模拟
static yb_image_data_s * imageDataNode;
//
//typedef struct memBlock
//{
//	unsigned char * memPointer;
//	bool isUse;
//	//memBlock * next;
//};
//static memBlock memNode[IMAGE_COUNT];
//
////内存初始化并分配到memNode
//static void initMem()
//{
//	rawMemory = new unsigned char[IMAGE_SIZE_MAX * IMAGE_COUNT];
//	if (rawMemory != NULL)
//	{
//		for (int i = 1; i < IMAGE_COUNT ; i++)
//		{
//			memNode[i].memPointer = rawMemory + i * IMAGE_SIZE_MAX;
//			memNode[i].isUse = false;
//		}
//	}
//}
//
//static unsigned char * newMem()
//{
//	for (int i = 0; i < IMAGE_COUNT; i++)
//	{
//		if(!memNode[i].isUse)
//		{
//			memNode[i].isUse = true;
//			return memNode[i].memPointer;
//		}
//	}
//}
//
//static void deleteMem(unsigned char * blockNode)
//{
//	int num = abs(blockNode - rawMemory)/(IMAGE_SIZE_MAX);
//	if(memNode[num].isUse)
//		memNode[num].isUse = false;
//}

//图像数据处理函数
static void getImageData(int n, void* pData, int width, int height, int DataSize)
{
	yb_image_data_s *imageNode = new yb_image_data_s();
	if (imageNode != NULL)
	{
		imageNode->pData = new UCHAR[DataSize];
		if (imageNode->pData != NULL)
		{
			unsigned char *data = (unsigned char *)pData;
			imageNode->width = width;
			imageNode->height = height;
			imageNode->dataSize = DataSize;
			memcpy((void *)&imageNode->pData[0], (void *)&data[0], DataSize);
			EnterCriticalSection(&imageLock);
			imageDataLst.push_back(imageNode);
			LeaveCriticalSection(&imageLock);
			if (hEvent != INVALID_HANDLE_VALUE)
			{
				SetEvent(hEvent);
			}
		}
	}
}

static DWORD WINAPI defectSearchFunc(LPVOID  args)
{
	unsigned int cycleIndex = 1;
	int defectCnt = 0;
	int i = 0;
	int offsets;
	yb_defect_s * stDefectArray;
	char filePath[MAX_PATH];
	// Local iconic variables 
	Hobject  Image, GrayImage, GaussFilter, GaussFilter1, ImageFFT;
	Hobject  ImageTrans, ConnectedRegions, SelectedRegions, SelectedRegions1;
	Hobject  SelectedRegionsX, SelectedRegionsY, ObjSelected;
	// Local control variables 
	HTuple  Width, Height, Sigma1, Sigma2, Sigma3, Sigma4;
	HTuple  start, end, Number, exetime, Rows, Columns;
	HTuple  MaxRow, MinRow, MaxColumn, MinColumn, MedianRow, MedianColumn; 
	//测试用
	char strTime[MAX_PATH];
	time_t curTime = time(NULL); //获取系统时间，单位为秒;
	tm timeinfo;
	localtime_s(&timeinfo,&curTime);
	sprintf( &strTime[0], "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d.txt",  
           timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,  
             timeinfo.tm_hour, timeinfo.tm_min,timeinfo.tm_sec);
	//strftime(strTime, 256, "%Y%m%d%H%M%S", &timeinfo);
	char fileName[MAX_PATH];
	strcpy_s(&fileName[0],MAX_PATH,strNonTempletImgDir);
	strcat(&fileName[0],&strTime[0]);
	if(!imageNames.empty())
	{
		txtFile.open(&fileName[0], std::fstream::in | std::fstream::out | std::fstream::app);
	}
	else
		return 0;
	//写入的两种方式
	//txtFile<<&strTime[0]<<endl;
	//txtFile.write(&strTime[0],256);
	int k = 0;
	while(!bWantExit)
	{	
		if (bWantExit)
		{
			break;
		}
		if (2 == defect_phase)
		{
			try
			{
				if (!imageNames.empty())
				{
					logFile.WriteLog(CLog::LL_INFORMATION,"defect_image");
					for (imageIter = imageNames.begin(); imageIter != imageNames.end(); imageIter++)
					{
						char * imageName = *imageIter;
						//记录图像名称,记录第i次检测
						txtFile<<imageName<<"\n";
						txtFile<<"第"<< k+1 <<"次检测，";
						char bmpFile[MAX_PATH];
						strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
						int len = strnlen_s(bmpFile, MAX_PATH);
						strcat_s(bmpFile, len + strnlen_s(imageName, MAX_PATH) + 1, imageName);
						//读入图像
						read_image(&Image,&bmpFile[0]);
						count_seconds(&start);
						get_image_size(Image, &Width, &Height);
						//颜色空间变换及灰度图像提取
						//RGB->gray
						rgb1_to_gray(Image, &GrayImage);
						Image.Reset();
						//提取Y方向sobel算子值（考虑边缘补全）
						sobel_amp(GrayImage, &GrayImage, "y", 3);
						//获取rtf高斯滤波器（调节高斯参数，为何要滤波）
						Sigma1 = 21.0;
						Sigma2 = 2.5;
						Sigma3 = 1.8;
						Sigma4 = 2.0;
						gen_gauss_filter(&GaussFilter, Sigma1, Sigma2, 0.0, "none", "rft", Width, Height);
						gen_gauss_filter(&GaussFilter1, Sigma3, Sigma4, 0.0, "none", "rft", Width, Height);
						//带阻滤波器
						sub_image(GaussFilter, GaussFilter1, &GaussFilter, 1, 0);
						GaussFilter1.Reset();
						//作实数傅里叶变换后卷积滤波
						//optimize_rft_speed (Width, Height, 'standard')
						rft_generic(GrayImage, &ImageFFT, "to_freq", "none", "complex", Width);
						convol_fft(ImageFFT, GaussFilter, &ImageFFT);
						rft_generic(ImageFFT, &ImageTrans, "from_freq", "none", "real", Width);
						GrayImage.Reset();
						ImageFFT.Reset();
						GaussFilter.Reset();
						//滤波结果放大（考虑线性放大）
						count_seconds(&end);
						exetime = end-start;
						scale_image(ImageTrans, &ImageTrans, 1e-007, 0);
						abs_image(ImageTrans, &ImageTrans);
						//滤波结果局域调整
						//形态学学校gray value
						threshold(ImageTrans, &ConnectedRegions, 3.5, 255);
						connection(ConnectedRegions, &ConnectedRegions);
						//明显瑕疵
						select_gray(ConnectedRegions, ImageTrans, &SelectedRegions, "mean", "and", 7, 255);
						select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 10, 255);
						dilation_circle(SelectedRegions, &SelectedRegions, 3);
						union1(SelectedRegions, &SelectedRegions);
						connection(SelectedRegions, &SelectedRegions);
						fill_up(SelectedRegions, &SelectedRegions);
						select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 21, 255);
						count_obj(SelectedRegions, &Number);
						count_seconds(&end);
						exetime = end-start;
						defectCnt = Number[0].I();
						// 释放Hobject，如果回调该图像内存地址，是否等到回调之后再释放；
						ImageTrans.Reset();
						//开始设置瑕疵信息
						if (defectCnt > 0)
						{
							//写入瑕疵个数
							txtFile<<"共检测到"<< defectCnt <<"个瑕疵\n"<<std::endl;
							stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s) * defectCnt);
							for (i = 0; i < defectCnt; i++)
							{
								//写入第i个瑕疵结果
								txtFile<<"第"<< i + 1 <<"个瑕疵："<<"\n";
								// 获取每一个区域的坐标，计算高度、宽度和中心点
								select_obj(SelectedRegions, &ObjSelected, i+1);//区域index从1开始计算
								get_region_points(ObjSelected, &Rows, &Columns);
								tuple_max(Rows, &MaxRow);
								tuple_min(Rows, &MinRow);
								tuple_median(Rows, &MedianRow);
								tuple_max(Columns, &MaxColumn);
								tuple_min(Columns, &MinColumn);
								tuple_median(Columns, &MedianColumn);
								stDefectArray[i].time_t = time(NULL);
								stDefectArray[i].defect_type = 255;
								stDefectArray[i].defect_width = MaxColumn[0].I() - MinColumn[0].I() + 1;
								stDefectArray[i].defect_height = MaxRow[0].I() - MinRow[0].I() + 1;
								stDefectArray[i].defectX_onPic = MedianColumn[0].I() - stDefectArray[i].defect_width / 2;
								stDefectArray[i].defectY_onPic = MedianRow[0].I() - stDefectArray[i].defect_height / 2;
								//瑕疵信息记录
								txtFile<<"宽："<< stDefectArray[i].defect_width <<"\n";
								txtFile<<"高："<< stDefectArray[i].defect_height <<"\n";
								txtFile<<"起始点："<< "（"<<stDefectArray[i].defectX_onPic<<"，"<<stDefectArray[i].defectY_onPic<<"）\n"<<"\n";
							}
							//模拟报告为接缝
							if ((cycleIndex % 375) == 0)
							{
								stDefectArray[i-1].defect_width = 8000;
								stDefectArray[i-1].defect_height = 100;
								stDefectArray[i-1].defectX_onPic = 0;
								stDefectArray[i-1].defectY_onPic = 0;
								//接缝结果记录
							}

							if (defectNotify != NULL && imageDataNode != NULL)
							{
								// 有瑕疵时，回调；
								(defectNotify)(imageDataNode, stDefectArray, defectCnt);
							}
						}
						//没有瑕疵
						else
						{
							stDefectArray = NULL;
							//没有瑕疵，但是有接缝
							if ((cycleIndex % 375) == 0)
							{
								//模拟报告为接缝
								stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s));
								stDefectArray[0].time_t = time(NULL);
								stDefectArray[0].defect_type = 255;
								stDefectArray[0].defect_width = 8000;
								stDefectArray[0].defect_height = 100;
								stDefectArray[0].defectX_onPic = 0;
								stDefectArray[0].defectY_onPic = 0;
								//接缝记录
								txtFile<<"没有检测到瑕疵，只有接缝\n"<<"\n";
								txtFile<<"接缝结果："<<"\n";
								txtFile<<"宽："<< stDefectArray[0].defect_width <<"\n";
								txtFile<<"高："<< stDefectArray[0].defect_height <<"\n";
								txtFile<<"起始点："<< "（"<<stDefectArray[0].defectX_onPic<<"，"<<stDefectArray[0].defectY_onPic<<"）\n"<<"\n";
								if (defectNotify != NULL && imageDataNode != NULL)
								{
									(defectNotify)(imageDataNode, stDefectArray, 1);
								}
							}
							else
							{
								//记录
								//没有瑕疵没有接缝
								txtFile<<"没有检测到瑕疵，没有接缝\n"<<"\n";
								if (defectNotify != NULL)
								{
									(defectNotify)(NULL, NULL, 0);
								}
							}
						}

						if (stDefectArray != NULL)
						{
							free(stDefectArray);
						}
						stDefectArray = NULL;
						cycleIndex++;
						if (bWantExit)
						{
							txtFile<<"退出线程"<<std::endl;
							break;
						}
					}//end for;
					k++;
				}//end if;
			}//end try;
			catch (...)
			{
				;
			}
		}
		else if (defect_phase == 1)
		{
			if (imageDataNotify != NULL)
			{
				(imageDataNotify)(imageDataNode);
				delete imageDataNode->pData;
				delete imageDataNode;
				imageDataNode = NULL;
			}
		}
	} //end while(!bWantExit)
	txtFile.close();
//-----------------------------------------------------------------------------------------end test
	//相机直接传入图像数据处理过程
	/* while(!bWantExit)
	{
		//被唤醒后一次取一个结点,取完马上离开临界区
		WaitForSingleObject(hEvent, INFINITE);
		if (bWantExit)
		{
			break;
		}
		EnterCriticalSection(&imageLock);
		yb_image_data_s *imgDataNode = (yb_image_data_s *)imageDataLst.front();
		imageDataLst.pop_front();
		LeaveCriticalSection(&imageLock);

		//以下进行瑕疵查找处理
		if (defect_phase == 2)
		{
			// Local iconic variables 
			Hobject  Image, GrayImage, ImageY, GaussFilter1;
			Hobject  GaussFilter2, GaussFilter, ImageFFT, ImageConvol;
			Hobject  ImageTrans, ConnectedRegions, SelectedRegions, SelectedRegions1;
			Hobject  SelectedRegionsX, SelectedRegionsY, ObjSelected;
			// Local control variables 
			HTuple  Width, Height, Sigma1, Sigma2, Sigma3, Sigma4;
			HTuple  start, end, Number, exetime, Rows, Columns;
			HTuple  MaxRow, MinRow, MaxColumn, MinColumn, MedianRow, MedianColumn; 
			try
			{
				//这里需要根据相机模式，选取不同的处理方式
				//现在只是处理rgb模式的
				WRITE_LOG(_T("defect_search开始检测。"));
				Width = (HTuple)imgDataNode->width;
				Height = (HTuple)imgDataNode->height;
				offsets = imgDataNode->dataSize * imgDataNode->width;
				//定义3个图像数组
				unsigned char* r_image = dataTransfer;
				memset(r_image, '\0', offsets);
				unsigned char* g_image = r_image + offsets;
				memset(g_image, '\0', offsets);
				unsigned char* b_image = g_image + offsets;
				memset(b_image, '\0', offsets);
				for (int i = 0;i<imgDataNode->height;i++)
				{
					for (int j = 0;j<imgDataNode->width;j++)
					{
						int val = i*imgDataNode->width+j;
						r_image[val]=imgDataNode->pData[val*3+2];
						g_image[val]=imgDataNode->pData[val*3+1];
						b_image[val]=imgDataNode->pData[val*3];
					}
				}
				gen_image3(&Image,"byte",imgDataNode->width,imgDataNode->height,(long)r_image,(long)g_image,(long)b_image);

				//裁剪图像（先校正去掉背景图像）

				//颜色空间变换及灰度图像提取
				rgb1_to_gray(Image, &GrayImage);
				//Method 变量控制使用的思路
				//思路1：分别对图像X和Y方向提取变化率
				//提取Y方向sobel算子值（考虑边缘补全）
				count_seconds(&start);
				sobel_amp(GrayImage, &ImageY, "y", 3);
				//获取rtf高斯滤波器（调节高斯参数，为何要滤波）
				Sigma1 = 21.0;
				Sigma2 = 2.5;
				Sigma3 = 1.8;
				Sigma4 = 2.0;
				gen_gauss_filter(&GaussFilter1, Sigma1, Sigma2, 0.0, "none", "rft", Width, Height);
				gen_gauss_filter(&GaussFilter2, Sigma3, Sigma4, 0.0, "none", "rft", Width, Height);
				//带阻滤波器
				sub_image(GaussFilter1, GaussFilter2, &GaussFilter, 1, 0);
				//作实数傅里叶变换后卷积滤波
				//optimize_rft_speed (Width, Height, 'standard')
				rft_generic(ImageY, &ImageFFT, "to_freq", "none", "complex", Width);
				convol_fft(ImageFFT, GaussFilter, &ImageConvol);
				rft_generic(ImageConvol, &ImageTrans, "from_freq", "none", "real", Width);
				//滤波结果放大（考虑线性放大）
				scale_image(ImageTrans, &ImageTrans, 1e-007, 0);
				abs_image(ImageTrans, &ImageTrans);
				//滤波结果局域调整
				//形态学学校gray value
				threshold(ImageTrans, &ConnectedRegions, 3.5, 255);
				connection(ConnectedRegions, &ConnectedRegions);
				//明显瑕疵
				select_gray(ConnectedRegions, ImageTrans, &SelectedRegions, "mean", "and", 7, 255);
				select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 10, 255);
				dilation_circle(SelectedRegions, &SelectedRegions, 3);
				union1(SelectedRegions, &SelectedRegions);
				connection(SelectedRegions, &SelectedRegions);
				fill_up(SelectedRegions, &SelectedRegions);
				select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 18.5, 255);
				//褶皱
				select_gray(ConnectedRegions, ImageTrans, &SelectedRegions1, "max", "and", 12.5, 19);
				//作x方向的扩展
				dilation_rectangle1(SelectedRegions1, &SelectedRegionsX, 20, 1);
				union1(SelectedRegionsX, &SelectedRegionsX);
				fill_up(SelectedRegionsX, &SelectedRegionsX);
				connection(SelectedRegionsX, &SelectedRegionsX);
				select_shape(SelectedRegionsX, &SelectedRegionsX, "width", "and", 128, 99999);
				erosion_rectangle1(SelectedRegionsX, &SelectedRegionsX, 10, 1);
				//作y方向的扩展
				dilation_rectangle1(SelectedRegions1, &SelectedRegionsY, 1, 20);
				union1(SelectedRegionsY, &SelectedRegionsY);
				fill_up(SelectedRegionsY, &SelectedRegionsY);
				connection(SelectedRegionsY, &SelectedRegionsY);
				select_shape(SelectedRegionsY, &SelectedRegionsY, "height", "and", 90, 99999);
				erosion_rectangle1(SelectedRegionsY, &SelectedRegionsY, 1, 10);
				//合并
				union2(SelectedRegionsX, SelectedRegionsY, &SelectedRegions1);
				union2(SelectedRegions, SelectedRegions1, &SelectedRegions);
				union1(SelectedRegions, &SelectedRegions);
				connection(SelectedRegions, &SelectedRegions);
				count_obj(SelectedRegions, &Number);
				count_seconds(&end);
				exetime = end-start;
				defectCnt = Number[0].I();
				// 释放Hobject，如果回调该图像内存地址，是否等到回调之后再释放；
				Image.Reset();
				//开始设置瑕疵信息
				if (defectCnt > 0)
				{
					stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s) * defectCnt);
					for (i = 0; i < defectCnt; i++)
					{
						// 获取每一个区域的坐标，计算高度、宽度和中心点
						select_obj(SelectedRegions, &ObjSelected, i+1);//区域index从1开始计算
						get_region_points(ObjSelected, &Rows, &Columns);
						tuple_max(Rows, &MaxRow);
						tuple_min(Rows, &MinRow);
						tuple_median(Rows, &MedianRow);
						tuple_max(Columns, &MaxColumn);
						tuple_min(Columns, &MinColumn);
						tuple_median(Columns, &MedianColumn);
						stDefectArray[i].time_t = time(NULL);
						stDefectArray[i].defect_type = 255;
						stDefectArray[i].defect_width = MaxColumn[0].I() - MinColumn[0].I() + 1;
						stDefectArray[i].defect_height = MaxRow[0].I() - MinRow[0].I() + 1;
						stDefectArray[i].defectX_onPic = MedianColumn[0].I() - stDefectArray[i].defect_width / 2;
						stDefectArray[i].defectY_onPic = MedianRow[0].I() - stDefectArray[i].defect_height / 2;
						WRITE_LOG(_T("瑕疵检测"),LOG_LEVEL_DEBUG,true);
					}
					//模拟报告为接缝
					if ((cycleIndex % 375) == 0)
					{
						stDefectArray[i-1].defect_width = 8000;
						stDefectArray[i-1].defect_height = 100;
						stDefectArray[i-1].defectX_onPic = 0;
						stDefectArray[i-1].defectY_onPic = 0;
					}

					if (defectNotify != NULL && imgDataNode != NULL)
					{
						// 有瑕疵时，回调；
						(defectNotify)(imgDataNode, stDefectArray, defectCnt);
					}
				}
				//没有瑕疵
				else
				{
					stDefectArray = NULL;
					//没有瑕疵，但是有接缝
					if ((cycleIndex % 375) == 0)
					{
						//模拟报告为接缝
						stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s));
						stDefectArray[0].time_t = time(NULL);
						stDefectArray[0].defect_type = 255;
						stDefectArray[0].defect_width = 8000;
						stDefectArray[0].defect_height = 100;
						stDefectArray[0].defectX_onPic = 0;
						stDefectArray[0].defectY_onPic = 0;

						if (defectNotify != NULL)
						{
							(defectNotify)(imgDataNode, stDefectArray, 1);
						}
					}
					else
					{
						//没有瑕疵没有接缝
						if (defectNotify != NULL)
						{
							(defectNotify)(NULL, NULL, 0);
						}
					}
				}

				if (stDefectArray != NULL)
				{
					free(stDefectArray);
				}
				stDefectArray = NULL;

				cycleIndex++;
				if (bWantExit)
				{
					break;
				}
				WRITE_LOG(_T("defect_search.dll回调完成。"));
			}
			catch (...)
			{
				;
			}
		}
		else if (defect_phase == 1)
		{
			if (imageDataNotify != NULL)
			{
				(imageDataNotify)(imgDataNode);
				delete imgDataNode->pData;
				delete imgDataNode;
				imgDataNode = NULL;
			}
		}

		//处理完成后要判断一下list里是否还有数据,如果有继续唤醒本线程.
		if (!imageDataLst.empty())
		{
			SetEvent(hEvent);
		}
	} // while(!bWantExit)

	char logTxt[256];
	sprintf_s(logTxt, 256, "瑕疵检测线程已退出，待检测队列中还有%d个消息没得到处理.", imageDataLst.size());
	WRITE_LOG(_T(logTxt), LOG_LEVEL_DEBUG, true);

	for (iter = imageDataLst.begin(); iter != imageDataLst.end(); iter++)
	{
		yb_image_data_s *imgDataNode = (*iter);
		delete imgDataNode->pData;
		delete imgDataNode;
		imgDataNode = NULL;
	}
	imageDataLst.clear(); */
	return 0;
}

void module_init(void)
{
	//strTempletImgDir[0] = '\0';
	//strNonTempletImgDir[0] = '\0';
	//hThreadForDSF = INVALID_HANDLE_VALUE;
	logFile.OpenLogFile("log/defectsearch.txt",CLog::LL_INFORMATION);
	//加载视觉操作dll
	//hInstanceDll = LoadLibrary(_T("./library/x64/sj_op.dll"));
	//if(hInstanceDll == NULL)
	//{
	//	WRITE_LOG(_T("加载sj_op.dll失败。"),LOG_LEVEL_DEBUG,true);
	//}
	//WRITE_LOG(_T("加载sj_op.dll成功。"));
	////获取注册函数地址
	//sjNotifyRegister = (sj_notify_register)GetProcAddress(hInstanceDll, "yb_sj_notify_register");
	//if(sjNotifyRegister == NULL)
	//{
	//	WRITE_LOG(_T("读取回调函数失败。"),LOG_LEVEL_DEBUG,true);
	//}
	//WRITE_LOG(_T("读取注册函数成功。"));
	////注册前初始化critical session,和创建同步事件对象
	//InitializeCriticalSection(&imageLock);
	//hEvent = CreateEvent(NULL, FALSE, FALSE, LPCSTR("SECEVT"));
	//if (hEvent)
	//{
	//	WRITE_LOG(_T("创建同步对象失败,原因是: "),LOG_LEVEL_DEBUG,true);
	//	WRITE_LOG(_T(strerror(GetLastError())),LOG_LEVEL_DEBUG,true);
	//}
	////注册回调函数
	//sjNotifyRegister((sj_notify)getImageData);
	dataTransfer = new unsigned char[IMAGE_SIZE_MAX];
	memset(dataTransfer,0,IMAGE_SIZE_MAX);
	if(NULL == dataTransfer)
	{
		logFile.WriteLog(CLog::LL_INFORMATION,"new memory error");
	}
	//
	imageDataNode = new yb_image_data_s();
	imageDataNode->pData = dataTransfer;
	imageDataNode->dataSize = IMAGE_SIZE_MAX;
	imageDataNode->height = 8000;
	imageDataNode->width = 8192;
	logFile.WriteLog(CLog::LL_INFORMATION,"module_init");
}

void module_exit(void)
{
	bWantExit = true;//关闭瑕疵检测线程
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(hEvent);
	}
	//释放关键区
	DeleteCriticalSection(&imageLock);
	//if (dataTransfer)
	//{
	//	delete[] dataTransfer;
	//	dataTransfer = NULL;
	//}
	//if (rawMemory)
	//{
	//	delete[] rawMemory;
	//	rawMemory = NULL;
	//}
	if (hThreadForDSF != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hThreadForDSF);
		hThreadForDSF = INVALID_HANDLE_VALUE;
	}
	if (hInstanceDll != NULL)
	{
		FreeLibrary(hInstanceDll);
		hInstanceDll = NULL;
	}
	logFile.WriteLog(CLog::LL_INFORMATION,"module_exit");
	return;
}

//以下定义需要导出的函数列表
//控制拍照用途
DEFECT_SEARCH_API void yb_defect_phase_set(int phase)
{
	defect_phase = phase;
	return;
}
//图像数据回传
DEFECT_SEARCH_API void yb_defect_image_notify_register(image_data_notify image_notify)
{
	imageDataNotify = image_notify;
	logFile.WriteLog(CLog::LL_INFORMATION,"defect_image_notify_register");
}

DEFECT_SEARCH_API void yb_defect_pic_set(char *pic_templet_path, char *pic_path)
{
	strcpy_s(strTempletImgDir, MAX_PATH, pic_templet_path);
	strcpy_s(strNonTempletImgDir, MAX_PATH, pic_path);
	logFile.WriteLog(CLog::LL_INFORMATION,"defect_pic_set");
	return;
}

DEFECT_SEARCH_API void yb_defect_pic_skip_set(int left_skip, int right_skip, int up_skip, int bottm_skip)
{
	//demo版本暂时不考虑；
	return;
}

DEFECT_SEARCH_API void yb_defect_area_min(int pixel_area)
{
	//demo版本暂时不考虑；
	return;
}

DEFECT_SEARCH_API void yb_defect_notify_register(defect_notify notify)
{
	defectNotify = notify;
	logFile.WriteLog(CLog::LL_INFORMATION,"defect_notify_register");
}

DEFECT_SEARCH_API void yb_defect_start(void)
{
	bWantExit = false;
	//getImageNames();
	//-------------------------------------
	long file;
    struct _finddata_t find;
	char path[MAX_PATH];
	strcpy_s(path, MAX_PATH, strNonTempletImgDir);
	int len = strnlen_s(path, MAX_PATH);
	strcat_s(path, len + 6, "*.bmp");
	if((file=_findfirst(path, &find))!=-1L)
	{
		char *name = new char[MAX_PATH];
		strcpy_s(name, MAX_PATH, find.name);
		imageNames.push_back(name);
		while(_findnext(file, &find)==0)
		{
			char *name = new char[MAX_PATH];
			strcpy_s(name, MAX_PATH, find.name);
			imageNames.push_back(name);
		}
		_findclose(file);
	}
	//-----------------------------------------------
	//InitializeCriticalSection(&imageLock);
	//hEventForSYN = CreateEvent(NULL, FALSE, FALSE, NULL);
	//创建第一个线程：监控目标文件夹变化
	//hThreadForIRDC = CreateThread(NULL, 0, imageRawDateCallback, NULL, 0, NULL);
	//创建第二个线程：瑕疵查找比对线程
	hThreadForDSF = CreateThread(NULL, 0, defectSearchFunc, NULL, 0, NULL);
	logFile.WriteLog(CLog::LL_INFORMATION,"CreateThreadForDSF");
	//CloseHandle(hThreadForIRDC);
	if (hThreadForDSF != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hThreadForDSF);
		hThreadForDSF = INVALID_HANDLE_VALUE;
	}
	logFile.WriteLog(CLog::LL_INFORMATION,"defect_start");
	return;
}

DEFECT_SEARCH_API void yb_defect_stop(void)
{
	return;
}

DEFECT_SEARCH_API int yb_defect_init(void)
{
	return SUCCESS;
}

DEFECT_SEARCH_API void yb_defect_exit(void)
{
	return;
}