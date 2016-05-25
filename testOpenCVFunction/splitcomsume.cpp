//#include <opencv2\core\core.hpp>
#include <opencv.hpp>
#include <iostream>
#include <ctime>
#include <Windows.h>

int main()
{
	cv::Mat image = cv::imread("3.bmp");
	cv::imshow("Ô­Ê¼Í¼Ïñ",image);
	cv::Mat rgb[3];
	clock_t start1, end1;
	double start2, end2;
	start1 = clock();
	start2 = GetTickCount();
	double start = cv::getTickCount();
	cv::split(image,rgb);

	double end = cv::getTickCount();
	end2 = GetTickCount();
	end1 = clock();
	double time = (end - start)/cv::getTickFrequency();
	std::cout<<time<<"\n"<<(end1-start1)<<"\n"<<(end2-start2)<<std::endl;
	cv::imshow("ºì",rgb[0]);
	cv::imshow("ÂÌ",rgb[1]);
	cv::imshow("À¶",rgb[2]);
	cv::waitKey(0);
}