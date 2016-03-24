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

//��־����
CLog logFile;
defect_notify defectNotify = NULL;
image_data_notify imageDataNotify = NULL;
static HANDLE hThreadForDSF = INVALID_HANDLE_VALUE;
static HANDLE hEvent = INVALID_HANDLE_VALUE;

static bool bWantExit = false;
//phase=1��ʾ����ģ��ͼ��phase=2��ʾ�����ģ��ͼ��
static int defect_phase;
static char strTempletImgDir[MAX_PATH];
static char strNonTempletImgDir[MAX_PATH];
//���ڵ���sj_op.dll��ı���
static HINSTANCE hInstanceDll = NULL;
static sj_notify_register sjNotifyRegister = NULL;
//���ڱ��������������ͼ������
static std::list<yb_image_data_s *> imageDataLst;
static std::list<yb_image_data_s *>::iterator iter;
//ͼ�����ݴ���
static CRITICAL_SECTION imageLock;
static unsigned char * rawMemory;
static unsigned char * dataTransfer;

//������
std::list<char *> imageNames;
std::list<char *>::iterator imageIter;
static std::fstream txtFile;
//����������ģ��
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
////�ڴ��ʼ�������䵽memNode
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

//ͼ�����ݴ�����
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
	//������
	char strTime[MAX_PATH];
	time_t curTime = time(NULL); //��ȡϵͳʱ�䣬��λΪ��;
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
	//д������ַ�ʽ
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
						//��¼ͼ������,��¼��i�μ��
						txtFile<<imageName<<"\n";
						txtFile<<"��"<< k+1 <<"�μ�⣬";
						char bmpFile[MAX_PATH];
						strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
						int len = strnlen_s(bmpFile, MAX_PATH);
						strcat_s(bmpFile, len + strnlen_s(imageName, MAX_PATH) + 1, imageName);
						//����ͼ��
						read_image(&Image,&bmpFile[0]);
						count_seconds(&start);
						get_image_size(Image, &Width, &Height);
						//��ɫ�ռ�任���Ҷ�ͼ����ȡ
						//RGB->gray
						rgb1_to_gray(Image, &GrayImage);
						Image.Reset();
						//��ȡY����sobel����ֵ�����Ǳ�Ե��ȫ��
						sobel_amp(GrayImage, &GrayImage, "y", 3);
						//��ȡrtf��˹�˲��������ڸ�˹������Ϊ��Ҫ�˲���
						Sigma1 = 21.0;
						Sigma2 = 2.5;
						Sigma3 = 1.8;
						Sigma4 = 2.0;
						gen_gauss_filter(&GaussFilter, Sigma1, Sigma2, 0.0, "none", "rft", Width, Height);
						gen_gauss_filter(&GaussFilter1, Sigma3, Sigma4, 0.0, "none", "rft", Width, Height);
						//�����˲���
						sub_image(GaussFilter, GaussFilter1, &GaussFilter, 1, 0);
						GaussFilter1.Reset();
						//��ʵ������Ҷ�任�����˲�
						//optimize_rft_speed (Width, Height, 'standard')
						rft_generic(GrayImage, &ImageFFT, "to_freq", "none", "complex", Width);
						convol_fft(ImageFFT, GaussFilter, &ImageFFT);
						rft_generic(ImageFFT, &ImageTrans, "from_freq", "none", "real", Width);
						GrayImage.Reset();
						ImageFFT.Reset();
						GaussFilter.Reset();
						//�˲�����Ŵ󣨿������ԷŴ�
						count_seconds(&end);
						exetime = end-start;
						scale_image(ImageTrans, &ImageTrans, 1e-007, 0);
						abs_image(ImageTrans, &ImageTrans);
						//�˲�����������
						//��̬ѧѧУgray value
						threshold(ImageTrans, &ConnectedRegions, 3.5, 255);
						connection(ConnectedRegions, &ConnectedRegions);
						//����覴�
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
						// �ͷ�Hobject������ص���ͼ���ڴ��ַ���Ƿ�ȵ��ص�֮�����ͷţ�
						ImageTrans.Reset();
						//��ʼ����覴���Ϣ
						if (defectCnt > 0)
						{
							//д��覴ø���
							txtFile<<"����⵽"<< defectCnt <<"��覴�\n"<<std::endl;
							stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s) * defectCnt);
							for (i = 0; i < defectCnt; i++)
							{
								//д���i��覴ý��
								txtFile<<"��"<< i + 1 <<"��覴ã�"<<"\n";
								// ��ȡÿһ����������꣬����߶ȡ���Ⱥ����ĵ�
								select_obj(SelectedRegions, &ObjSelected, i+1);//����index��1��ʼ����
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
								//覴���Ϣ��¼
								txtFile<<"��"<< stDefectArray[i].defect_width <<"\n";
								txtFile<<"�ߣ�"<< stDefectArray[i].defect_height <<"\n";
								txtFile<<"��ʼ�㣺"<< "��"<<stDefectArray[i].defectX_onPic<<"��"<<stDefectArray[i].defectY_onPic<<"��\n"<<"\n";
							}
							//ģ�ⱨ��Ϊ�ӷ�
							if ((cycleIndex % 375) == 0)
							{
								stDefectArray[i-1].defect_width = 8000;
								stDefectArray[i-1].defect_height = 100;
								stDefectArray[i-1].defectX_onPic = 0;
								stDefectArray[i-1].defectY_onPic = 0;
								//�ӷ�����¼
							}

							if (defectNotify != NULL && imageDataNode != NULL)
							{
								// ��覴�ʱ���ص���
								(defectNotify)(imageDataNode, stDefectArray, defectCnt);
							}
						}
						//û��覴�
						else
						{
							stDefectArray = NULL;
							//û��覴ã������нӷ�
							if ((cycleIndex % 375) == 0)
							{
								//ģ�ⱨ��Ϊ�ӷ�
								stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s));
								stDefectArray[0].time_t = time(NULL);
								stDefectArray[0].defect_type = 255;
								stDefectArray[0].defect_width = 8000;
								stDefectArray[0].defect_height = 100;
								stDefectArray[0].defectX_onPic = 0;
								stDefectArray[0].defectY_onPic = 0;
								//�ӷ��¼
								txtFile<<"û�м�⵽覴ã�ֻ�нӷ�\n"<<"\n";
								txtFile<<"�ӷ�����"<<"\n";
								txtFile<<"��"<< stDefectArray[0].defect_width <<"\n";
								txtFile<<"�ߣ�"<< stDefectArray[0].defect_height <<"\n";
								txtFile<<"��ʼ�㣺"<< "��"<<stDefectArray[0].defectX_onPic<<"��"<<stDefectArray[0].defectY_onPic<<"��\n"<<"\n";
								if (defectNotify != NULL && imageDataNode != NULL)
								{
									(defectNotify)(imageDataNode, stDefectArray, 1);
								}
							}
							else
							{
								//��¼
								//û��覴�û�нӷ�
								txtFile<<"û�м�⵽覴ã�û�нӷ�\n"<<"\n";
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
							txtFile<<"�˳��߳�"<<std::endl;
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
	//���ֱ�Ӵ���ͼ�����ݴ������
	/* while(!bWantExit)
	{
		//�����Ѻ�һ��ȡһ�����,ȡ�������뿪�ٽ���
		WaitForSingleObject(hEvent, INFINITE);
		if (bWantExit)
		{
			break;
		}
		EnterCriticalSection(&imageLock);
		yb_image_data_s *imgDataNode = (yb_image_data_s *)imageDataLst.front();
		imageDataLst.pop_front();
		LeaveCriticalSection(&imageLock);

		//���½���覴ò��Ҵ���
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
				//������Ҫ�������ģʽ��ѡȡ��ͬ�Ĵ���ʽ
				//����ֻ�Ǵ���rgbģʽ��
				WRITE_LOG(_T("defect_search��ʼ��⡣"));
				Width = (HTuple)imgDataNode->width;
				Height = (HTuple)imgDataNode->height;
				offsets = imgDataNode->dataSize * imgDataNode->width;
				//����3��ͼ������
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

				//�ü�ͼ����У��ȥ������ͼ��

				//��ɫ�ռ�任���Ҷ�ͼ����ȡ
				rgb1_to_gray(Image, &GrayImage);
				//Method ��������ʹ�õ�˼·
				//˼·1���ֱ��ͼ��X��Y������ȡ�仯��
				//��ȡY����sobel����ֵ�����Ǳ�Ե��ȫ��
				count_seconds(&start);
				sobel_amp(GrayImage, &ImageY, "y", 3);
				//��ȡrtf��˹�˲��������ڸ�˹������Ϊ��Ҫ�˲���
				Sigma1 = 21.0;
				Sigma2 = 2.5;
				Sigma3 = 1.8;
				Sigma4 = 2.0;
				gen_gauss_filter(&GaussFilter1, Sigma1, Sigma2, 0.0, "none", "rft", Width, Height);
				gen_gauss_filter(&GaussFilter2, Sigma3, Sigma4, 0.0, "none", "rft", Width, Height);
				//�����˲���
				sub_image(GaussFilter1, GaussFilter2, &GaussFilter, 1, 0);
				//��ʵ������Ҷ�任�����˲�
				//optimize_rft_speed (Width, Height, 'standard')
				rft_generic(ImageY, &ImageFFT, "to_freq", "none", "complex", Width);
				convol_fft(ImageFFT, GaussFilter, &ImageConvol);
				rft_generic(ImageConvol, &ImageTrans, "from_freq", "none", "real", Width);
				//�˲�����Ŵ󣨿������ԷŴ�
				scale_image(ImageTrans, &ImageTrans, 1e-007, 0);
				abs_image(ImageTrans, &ImageTrans);
				//�˲�����������
				//��̬ѧѧУgray value
				threshold(ImageTrans, &ConnectedRegions, 3.5, 255);
				connection(ConnectedRegions, &ConnectedRegions);
				//����覴�
				select_gray(ConnectedRegions, ImageTrans, &SelectedRegions, "mean", "and", 7, 255);
				select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 10, 255);
				dilation_circle(SelectedRegions, &SelectedRegions, 3);
				union1(SelectedRegions, &SelectedRegions);
				connection(SelectedRegions, &SelectedRegions);
				fill_up(SelectedRegions, &SelectedRegions);
				select_gray(SelectedRegions, ImageTrans, &SelectedRegions, "max", "and", 18.5, 255);
				//����
				select_gray(ConnectedRegions, ImageTrans, &SelectedRegions1, "max", "and", 12.5, 19);
				//��x�������չ
				dilation_rectangle1(SelectedRegions1, &SelectedRegionsX, 20, 1);
				union1(SelectedRegionsX, &SelectedRegionsX);
				fill_up(SelectedRegionsX, &SelectedRegionsX);
				connection(SelectedRegionsX, &SelectedRegionsX);
				select_shape(SelectedRegionsX, &SelectedRegionsX, "width", "and", 128, 99999);
				erosion_rectangle1(SelectedRegionsX, &SelectedRegionsX, 10, 1);
				//��y�������չ
				dilation_rectangle1(SelectedRegions1, &SelectedRegionsY, 1, 20);
				union1(SelectedRegionsY, &SelectedRegionsY);
				fill_up(SelectedRegionsY, &SelectedRegionsY);
				connection(SelectedRegionsY, &SelectedRegionsY);
				select_shape(SelectedRegionsY, &SelectedRegionsY, "height", "and", 90, 99999);
				erosion_rectangle1(SelectedRegionsY, &SelectedRegionsY, 1, 10);
				//�ϲ�
				union2(SelectedRegionsX, SelectedRegionsY, &SelectedRegions1);
				union2(SelectedRegions, SelectedRegions1, &SelectedRegions);
				union1(SelectedRegions, &SelectedRegions);
				connection(SelectedRegions, &SelectedRegions);
				count_obj(SelectedRegions, &Number);
				count_seconds(&end);
				exetime = end-start;
				defectCnt = Number[0].I();
				// �ͷ�Hobject������ص���ͼ���ڴ��ַ���Ƿ�ȵ��ص�֮�����ͷţ�
				Image.Reset();
				//��ʼ����覴���Ϣ
				if (defectCnt > 0)
				{
					stDefectArray = (yb_defect_s *)malloc(sizeof(yb_defect_s) * defectCnt);
					for (i = 0; i < defectCnt; i++)
					{
						// ��ȡÿһ����������꣬����߶ȡ���Ⱥ����ĵ�
						select_obj(SelectedRegions, &ObjSelected, i+1);//����index��1��ʼ����
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
						WRITE_LOG(_T("覴ü��"),LOG_LEVEL_DEBUG,true);
					}
					//ģ�ⱨ��Ϊ�ӷ�
					if ((cycleIndex % 375) == 0)
					{
						stDefectArray[i-1].defect_width = 8000;
						stDefectArray[i-1].defect_height = 100;
						stDefectArray[i-1].defectX_onPic = 0;
						stDefectArray[i-1].defectY_onPic = 0;
					}

					if (defectNotify != NULL && imgDataNode != NULL)
					{
						// ��覴�ʱ���ص���
						(defectNotify)(imgDataNode, stDefectArray, defectCnt);
					}
				}
				//û��覴�
				else
				{
					stDefectArray = NULL;
					//û��覴ã������нӷ�
					if ((cycleIndex % 375) == 0)
					{
						//ģ�ⱨ��Ϊ�ӷ�
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
						//û��覴�û�нӷ�
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
				WRITE_LOG(_T("defect_search.dll�ص���ɡ�"));
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

		//������ɺ�Ҫ�ж�һ��list���Ƿ�������,����м������ѱ��߳�.
		if (!imageDataLst.empty())
		{
			SetEvent(hEvent);
		}
	} // while(!bWantExit)

	char logTxt[256];
	sprintf_s(logTxt, 256, "覴ü���߳����˳������������л���%d����Ϣû�õ�����.", imageDataLst.size());
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
	//�����Ӿ�����dll
	//hInstanceDll = LoadLibrary(_T("./library/x64/sj_op.dll"));
	//if(hInstanceDll == NULL)
	//{
	//	WRITE_LOG(_T("����sj_op.dllʧ�ܡ�"),LOG_LEVEL_DEBUG,true);
	//}
	//WRITE_LOG(_T("����sj_op.dll�ɹ���"));
	////��ȡע�ắ����ַ
	//sjNotifyRegister = (sj_notify_register)GetProcAddress(hInstanceDll, "yb_sj_notify_register");
	//if(sjNotifyRegister == NULL)
	//{
	//	WRITE_LOG(_T("��ȡ�ص�����ʧ�ܡ�"),LOG_LEVEL_DEBUG,true);
	//}
	//WRITE_LOG(_T("��ȡע�ắ���ɹ���"));
	////ע��ǰ��ʼ��critical session,�ʹ���ͬ���¼�����
	//InitializeCriticalSection(&imageLock);
	//hEvent = CreateEvent(NULL, FALSE, FALSE, LPCSTR("SECEVT"));
	//if (hEvent)
	//{
	//	WRITE_LOG(_T("����ͬ������ʧ��,ԭ����: "),LOG_LEVEL_DEBUG,true);
	//	WRITE_LOG(_T(strerror(GetLastError())),LOG_LEVEL_DEBUG,true);
	//}
	////ע��ص�����
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
	bWantExit = true;//�ر�覴ü���߳�
	if (hEvent != INVALID_HANDLE_VALUE)
	{
		SetEvent(hEvent);
	}
	//�ͷŹؼ���
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

//���¶�����Ҫ�����ĺ����б�
//����������;
DEFECT_SEARCH_API void yb_defect_phase_set(int phase)
{
	defect_phase = phase;
	return;
}
//ͼ�����ݻش�
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
	//demo�汾��ʱ�����ǣ�
	return;
}

DEFECT_SEARCH_API void yb_defect_area_min(int pixel_area)
{
	//demo�汾��ʱ�����ǣ�
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
	//������һ���̣߳����Ŀ���ļ��б仯
	//hThreadForIRDC = CreateThread(NULL, 0, imageRawDateCallback, NULL, 0, NULL);
	//�����ڶ����̣߳�覴ò��ұȶ��߳�
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