#include <Windows.h>
#include <iostream>
#include <io.h>
#include <list>
#include "Halcon\include\cpp\HalconCpp.h"

using namespace Halcon;

std::list<Hobject> Images;
std::list<char *> imageNames;
std::list<char *>::iterator iter;

int main()
{
	const char *strNonTempletImgDir = "E:\\Action\\testdataset\\´¿É«\\";
	long file;
    struct _finddata_t find;
	char path[MAX_PATH];
	strcpy_s(path, MAX_PATH, strNonTempletImgDir);
	int len = strnlen_s(path, MAX_PATH);
	strcat_s(path, len + 6, "*.jpg");
	if((file=_findfirst(path, &find))!=-1L)
	{
		//char bmpFile[MAX_PATH];
		//strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
		//len = strnlen_s(bmpFile, MAX_PATH);
		//strcat_s(bmpFile, len + strnlen_s(find.name, MAX_PATH) + 1, find.name);
		imageNames.push_back(find.name);
		//Hobject image;
		//read_image(&image,&bmpFile[0]);
		//Images.push_back(image);
		while(_findnext(file, &find)==0)
		{
			//char bmpFile[MAX_PATH];
			//strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
			//len = strnlen_s(bmpFile, MAX_PATH);
			//strcat_s(bmpFile, len + strnlen_s(find.name, MAX_PATH) + 1, find.name);
			imageNames.push_back(find.name);
			//Hobject image;
			//read_image(&image,&bmpFile[0]);
			//Images.push_back(image);
		}
		_findclose(file);
	}
	for(iter = imageNames.begin(); iter != imageNames.end(); iter++)
	{
		char * imageName = *iter;
		char * imageName1 = imageNames.front();
		std::cout<< imageName << endl;
}