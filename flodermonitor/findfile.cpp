#include <io.h>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
void getFiles( string path, vector<string>& files );

int main()
{
	char * filePath = "E:\\Action\\testdataset";
	vector<string> files;

	////获取该路径下的所有文件
	getFiles(filePath, files );

	char str[30];
	int size = files.size();
	for (int i = 0;i < size;i++)
	{
		cout<<files[i].c_str()<<endl;
	}
	system("pause");
	return 0;
}

void getFiles( string path, vector<string>& files )
{
	//文件句柄
	long   hFile   =   0;
	//文件信息
	struct _finddata_t fileinfo;
	string p;
	if((hFile = _findfirst(p.assign(path).append("\\*.bmp").c_str(),&fileinfo)) !=  -1)
	{
		do
		{
			//如果是目录,迭代之
			//如果不是,加入列表
			if((fileinfo.attrib &  _A_SUBDIR))
			{
				if(strcmp(fileinfo.name,".") != 0  &&  strcmp(fileinfo.name,"..") != 0)
					getFiles( p.assign(path).append("\\").append(fileinfo.name), files );
			}
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name) );
			}
		}while(_findnext(hFile, &fileinfo)  == 0);
		_findclose(hFile);
	}
}

//int main()
//{
//	long file;
//    struct _finddata_t find;
//	char path[MAX_PATH];
//	strcpy_s(path, MAX_PATH, strNonTempletImgDir);
//	len = strnlen_s(path, MAX_PATH);
//	strcat_s(path, len + 6, "*.bmp");
//	if((file=_findfirst(path, &find))!=-1L)
//	{
//		char bmpFile[MAX_PATH];
//		strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
//		len = strnlen_s(bmpFile, MAX_PATH);
//		strcat_s(bmpFile, len + strnlen_s(find.name, MAX_PATH) + 1, find.name);
//		yb_image_data_s *imgData = new yb_image_data_s;
//		imgData->width = 0;
//		imgData->height = 0;
//		imgData->pData = NULL;
//		imgData->dataSize = 0;
//		imgDataLst.push_back(imgData);
//		while(_findnext(file, &find)==0)
//		{
//			char bmpFile[MAX_PATH];;
//			strcpy_s(bmpFile, MAX_PATH, strNonTempletImgDir);
//			len = strnlen_s(bmpFile, MAX_PATH);
//			strcat_s(bmpFile, len + strnlen_s(find.name, MAX_PATH) + 1, find.name);
//			yb_image_data_s *imgData = new yb_image_data_s;
//			imgData->width = 0;
//			imgData->height = 0;
//			imgData->pData = NULL;
//			imgData->dataSize = 0;
//			imgDataLst.push_back(imgData);
//		}
//		_findclose(file);
//	}
//}