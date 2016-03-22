#include <cstdio>
#include <Windows.h>
#include "Halcon\include\cpp\HalconCpp.h"

using namespace Halcon;

int main()
{
	//BITMAPFILEHEADER
	Hobject *grayImage = new Hobject;
	HTuple WindowHandle;
	unsigned char *rImage;
	unsigned char *gImage;
	unsigned char *bImage;
	rImage = (unsigned char *)malloc(768*525);
	gImage = new unsigned char[768 * 525];
	bImage = new unsigned char[768 * 525];
	int r,c;
	for(r=0;r<525;r++)
		for(c=0;c<768;c++)
		{
			rImage[r*768+c]=c%255;
			gImage[r*768+c] = (767 - c) % 255;
			bImage[r*768+c]  = r % 255;
		}
	//该函数grayImage有自己的内存块，之后可以释放r，g，b三个指针的内存
	gen_image3(grayImage,"byte",768,525,(long)rImage, (long)gImage, (long)bImage);
	//该函数没有做内存拷贝工作，之后不可以释放r，g，b三个指针的内存
	//gen_image3_extern(&grayImage,"byte",768,525,(long)rImage, (long)gImage, (long)bImage, NULL);
	if(NULL != rImage)
	{
		free(rImage);
		delete[] gImage;
		delete[] bImage;
		//image = NULL;
	}
	write_image(*grayImage,"bmp",0,"testImage.bmp");
	open_window(0,0,840,640,0,"","",&WindowHandle);
	HDevWindowStack::Push(WindowHandle);
	if (HDevWindowStack::IsOpen())
		disp_obj(*grayImage, HDevWindowStack::GetActive());

	FILE *bmpImage = fopen("testImage.bmp", "rb");
	system("pause");
}