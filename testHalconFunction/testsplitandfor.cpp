#include <cstdio>
#include <Windows.h>
#include "Halcon\include\cpp\HalconCpp.h"
#include <opencv2\core\core.hpp>

using namespace Halcon;

BYTE* transdata;
//cv::Mat bmpMat;

void _bmp2HobjectbyOpenCV(Hobject* Image, int width, int height, BYTE * data)
{
	//cv::Mat bmpMat(width, height, CV_8UC3, data);
	//memcpy(bmpMat.data,data,width*height*3);
	//cv::Mat BMP = cv::Mat(cv::Size2d(width, height),3,(void*)data,0Ui64);
	//cv::Mat rgb[3];
	//gen_image3(Image, "byte",width,height,(long)rgb[2].data,(long)rgb[1].data,(long)rgb[0].data);
}

BYTE* _getRGBData(char *filePath, int & width, int & height, int & dataSize)
{	 
	FILE *fp = NULL; 
	int ret = fopen_s(&fp,filePath,"rb"); 
	if(fp == NULL)    
	{ 
		return false; 
	} 
	BITMAPFILEHEADER fileheader={0}; 
	fread(&fileheader,sizeof(fileheader),1,fp); 
	if(fileheader.bfType != 0x4D42) 
	{ 
		fclose(fp); 
		return false; 
	}

	BITMAPINFOHEADER head; 
	fread(&head,sizeof(BITMAPINFOHEADER),1,fp);  
	long bmpWidth = head.biWidth; 
	long bmpHeight = head.biHeight; 
	WORD biBitCount = head.biBitCount; 
	if(biBitCount != 24)
	{ 
		fclose(fp); 
		return false; 
	}

	int totalSize = (bmpWidth *biBitCount/8+3)/4*4*bmpHeight;
	width = bmpWidth;
	height = bmpHeight;
	dataSize = totalSize;

	BYTE *pBmpBuf = new BYTE[dataSize]; 
	int n = (bmpWidth *biBitCount/8+3)/4*4;
	int offset = totalSize;
	for (int i = 0; i < height; i++)
	{
		offset -= n;
		fread(&pBmpBuf[offset],1,n,fp);
	}
	fclose(fp);
	return pBmpBuf;
}

void _RGB2Hobject(Hobject * Image, int width, int height, BYTE * data)
{
	//	//read_image(&grayImage,"2.jpg");
	//int offsets = height * width;
	////定义3个图像数组
	////BYTE* r_image = transdata;
	//BYTE* r_image = new BYTE[offsets];
	//memset(r_image, '\0', offsets);
	////BYTE* g_image = r_image + offsets;
	//BYTE* g_image = new BYTE[offsets];
	//memset(g_image, '\0', offsets);
	////BYTE* b_image = g_image + offsets;
	//BYTE* b_image = new BYTE[offsets];
	//memset(b_image, '\0', offsets);
	//for (int h = 0;h<height;h++)
	//{
	//	for (int w = 0;w<width;w++)
	//	{
	//		int val = h*width+w;
	//		r_image[val]=data[val*3+2];
	//		g_image[val]=data[val*3+1];
	//		b_image[val]=data[val*3];
	//	}
	//}
	//gen_image3(Image,"byte",width,height,(long)r_image,(long)g_image,(long)b_image);
	//delete[] r_image;
	//delete[] g_image;
	//delete[] b_image;
	gen_image_interleaved (Image, (long long)data, "bgr", (HTuple)width, (HTuple)height, 1,"byte",(HTuple)width, (HTuple)height, 0, 0, -1,0);
}

void RGB2Hobject(Hobject * Image, int width, int height, BYTE * data)
{
	//read_image(&grayImage,"2.jpg");
	int offsets = height * width;
	//定义3个图像数组
	BYTE* r_image = transdata;
	//BYTE* r_image = new BYTE[offsets];
	//memset(r_image, '\0', offsets);
	BYTE* g_image = r_image + offsets;
	//BYTE* g_image = new BYTE[offsets];
	//memset(g_image, '\0', offsets);
	BYTE* b_image = g_image + offsets;
	//BYTE* b_image = new BYTE[offsets];
	//memset(b_image, '\0', offsets);
	int row = 0;
	int val = 0;
	for (int h = 0;h<height;h++)
	{
		row = h * width;
		for (int w = 0;w<width;w++)
		{
			val = row + w;
			r_image[val]=data[val*3+2];
			g_image[val]=data[val*3+1];
			b_image[val]=data[val*3];
		}
	}
	gen_image3(Image,"byte",width,height,(Hlong)r_image,(Hlong)g_image,(Hlong)b_image);
	//delete[] r_image;
	//delete[] g_image;
	//delete[] b_image;
}

void preProcessing(Hobject* &rgbImage, Hobject* &grayImage)
{
	// 图像灰度化
	rgb1_to_gray(*rgbImage, grayImage);
	// 图像去噪：先均值滤波，然后类高斯滤波
	mean_image(*grayImage, grayImage, 3, 3);
	binomial_filter(*grayImage, grayImage, 5, 5);
	return;
}

int main()
{
	transdata = new BYTE[8192*8000*3];
	double start, start1, end, end1;
	int width;
	int height;
	int dataSize;
	Hobject *newImage = new Hobject();
	Hobject *newGray = new Hobject();
	Hobject dImage;
	Hobject dGray;
	HTuple pR, pG, pB, pType, Width, Height;
	char* filename = new char[260];
	sprintf(filename, "defect_%d.bmp", 3);
	BYTE * bmpData = _getRGBData(filename,width,height,dataSize);
	_RGB2Hobject(newImage, width, height, bmpData);
	_RGB2Hobject(&dImage, width, height, bmpData);

	preProcessing(newImage, newGray);
	//preProcessing(&dImage, &dGray);

	HTuple mean, deviation;
	intensity(*newImage, *newImage, &mean, &deviation);
	double test = mean[0].D();
	test = deviation[0].D();

	intensity(dImage, dImage, &mean, &deviation);
	double test1 = mean[0].D();
	test1 = deviation[0].D();
	//for (int i = 0; i < 30; i++)
	//{
	//	char* filename = new char[260];
	//	sprintf(filename, "defect_%d.bmp", i%10+1);
	//	BYTE * bmpData = _getRGBData(filename,width,height,dataSize);
	//	start = GetTickCount();
	//	//if(i == 0  || i > 12)
	//	printf ("add:%X, pointer length:%X\n", bmpData,(long)bmpData);
	//	_RGB2Hobject(Image, width, height, bmpData);
	//	//_bmp2HobjectbyOpenCV(&Image, width, height, bmpData);
	//	end = GetTickCount();
	//	//start1 = GetTickCount();
	//	//_RGB2Hobject(&Image1, width, height, bmpData);
	//	//end1 = GetTickCount();
	//	get_image_pointer3((*Image), &pR, &pG, &pB, &pType, &Width, &Height);
	//	//printf ("add:%X, pointer length:%X\n", bmpData,(long)bmpData);
	//	//printf ("CV  time : %f\n", end1 - start1);
	//	Sleep(1000);
	//}
	//mirror_image(Image, &Image,"row");
	HTuple WindowHandle;
	open_window(0,0,840,680,0,"","",&WindowHandle);
	HDevWindowStack::Push(WindowHandle);
	if (HDevWindowStack::IsOpen())
		disp_obj(*newImage, HDevWindowStack::GetActive());

	system("pause");
	if (HDevWindowStack::IsOpen())
		disp_obj(dImage, HDevWindowStack::GetActive());

	system("pause");
	if (newImage != NULL)
	{
		delete newImage;
	}
}


