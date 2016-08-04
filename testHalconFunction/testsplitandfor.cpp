#include <cstdio>
#include <Windows.h>
#include "Halcon\include\cpp\HalconCpp.h"
//#include <opencv2\opencv.hpp>
#include <opencv2\core\core.hpp>
//#include <opencv2\core\mat.hpp>

using namespace Halcon;

BYTE * transdata;
cv::Mat bmpMat;

void _bmp2HobjectbyOpenCV(Hobject * Image, int width, int height, BYTE * data)
{
	//cv::Mat bmpMat(width, height, CV_8UC3, data);
	memcpy(bmpMat.data,data,width*height*3);
	cv::Mat rgb[3];
	cv::split(bmpMat,rgb);
	gen_image3(Image, "byte",width,height,(long)rgb[2].data,(long)rgb[1].data,(long)rgb[0].data);
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
	gen_image_interleaved (Image, (long)data, "bgr", (HTuple)width, (HTuple)height, 1,"byte",(HTuple)width, (HTuple)height, 0, 0, -1,0);
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
	gen_image3(Image,"byte",width,height,(long)r_image,(long)g_image,(long)b_image);
	//delete[] r_image;
	//delete[] g_image;
	//delete[] b_image;
}

int main()
{
	transdata = new BYTE[8192*8000*3];
	bmpMat = cv::Mat(8192,4000,CV_8UC3,cv::Scalar(255,255,255));
	double start, start1, end, end1;
	int width;
	int height;
	int dataSize;
	Hobject Image,Image1;
	HTuple pR, pG, pB, pType, Width, Height;
	for (int i = 0; i < 30; i++)
	{
		char* filename = new char[260];
		sprintf(filename, "defect_%d.bmp", i%10+1);
		BYTE * bmpData = _getRGBData(filename,width,height,dataSize);
		//start = GetTickCount();
		//if(i<15 || i> 25)
		//	_RGB2Hobject(&Image, width, height, bmpData);
		//end = GetTickCount();
		start1 = GetTickCount();
		RGB2Hobject(&Image1, width, height, bmpData);
		end1 = GetTickCount();
		get_image_pointer3 (Image1, &pR, &pG, &pB, &pType, &Width, &Height);
		printf ("for time : %f\n", end - start);
		//printf ("CV  time : %f\n", end1 - start1);
		Sleep(1000);
	}
	//mirror_image(Image, &Image,"row");
	HTuple WindowHandle;
	open_window(0,0,840,680,0,"","",&WindowHandle);
	HDevWindowStack::Push(WindowHandle);
	if (HDevWindowStack::IsOpen())
		disp_obj(Image, HDevWindowStack::GetActive());

	system("pause");
}


