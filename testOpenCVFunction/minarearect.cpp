#include <vector>
#include <opencv2\opencv.hpp>

int otsu(cv::Mat image)
{
	int width = image.cols;
	int height = image.rows;
	int x = 0, y = 0;
	int pixelCount[256];
	float pixelPro[256];
	int i, j, pixelSum = width * height, threshold = 0;
	//uchar* data = (uchar*)image->imageData;
	uchar* data = (uchar*)image.data;

	//��ʼ��
	for (i = 0; i < 256; i++)
	{
		pixelCount[i] = 0;
		pixelPro[i] = 0;
	}

	//ͳ�ƻҶȼ���ÿ������������ͼ���еĸ���
	for (i = y; i < height; i++)
	{
		for (j = x; j <width; j++)
		{
			//pixelCount[data[i * image->widthStep + j]]++;
			pixelCount[data[i * image.step + j]]++;
		}
	}


	//����ÿ������������ͼ���еı���
	for (i = 0; i < 256; i++)
	{
		pixelPro[i] = (float)(pixelCount[i]) / (float)(pixelSum);
	}

	//����ostu�㷨,�õ�ǰ���ͱ����ķָ�
	//�����Ҷȼ�[0,255],������������ĻҶ�ֵ,Ϊ�����ֵ
	float w0, w1, u0tmp, u1tmp, u0, u1, u, deltaTmp, deltaMax = 0;
	for (i = 0; i < 256; i++)
	{
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;

		for (j = 0; j < 256; j++)
		{
			if (j <= i) //��������
			{
				//��iΪ��ֵ���࣬��һ���ܵĸ���
				w0 += pixelPro[j];
				u0tmp += j * pixelPro[j];
			}
			else       //ǰ������
			{
				//��iΪ��ֵ���࣬�ڶ����ܵĸ���
				w1 += pixelPro[j];
				u1tmp += j * pixelPro[j];
			}
		}

		u0 = u0tmp / w0;		//��һ���ƽ���Ҷ�
		u1 = u1tmp / w1;		//�ڶ����ƽ���Ҷ�
		u = u0tmp + u1tmp;		//����ͼ���ƽ���Ҷ�
		//������䷽��
		deltaTmp = w0 * (u0 - u)*(u0 - u) + w1 * (u1 - u)*(u1 - u);
		//�������䷽���Լ���Ӧ����ֵ
		if (deltaTmp > deltaMax)
		{
			deltaMax = deltaTmp;
			threshold = i;
		}
	}
	//���������ֵ;
	return threshold;
}

int main(int argc, char* argv[])
{
	cv::Mat srcImage = cv::imread("002.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	if (srcImage.data == NULL)
		return 0;
	cv::namedWindow("src", 0);
	cv::resizeWindow("src", 512, 512);
	cv::imshow("src", srcImage);

	cv::Mat biImage = cv::Mat(srcImage.size(), CV_8UC1);
	//���������ֵ
	int threshold = otsu(srcImage)*0.5;
	//��ͼ���ֵ��
	cv::threshold(srcImage, biImage, threshold, 255, CV_THRESH_BINARY);
	cv::Mat reImage;
	cv::transpose(srcImage, reImage);
	cv::resize(srcImage, reImage, cv::Size(512,512)); 

	int k = 1;
	cv::namedWindow("binary", 0);
	cv::resizeWindow("binary", 512, 512);
	cv::imshow("binary",biImage);
	//canny��ȡ��Ե
	cv::Mat edgeImage = cv::Mat(srcImage.size(), CV_8UC1);
	cv::Canny(biImage,edgeImage, 10, 87);
	cv::namedWindow("edge", 0);
	cv::resizeWindow("edge", 512, 512);
	cv::imshow("edge", edgeImage);
	//����֧��in-place����	
	cv::dilate(edgeImage, edgeImage, NULL, cv::Point(-1,-1), 3);
	cv::namedWindow("filter", 0);
	cv::resizeWindow("filter", 512, 512);
	cv::imshow("filter", edgeImage);
	//��ȡ�����ͨ��
	cv::Mat pContourImg = cv::Mat(edgeImage.size(), CV_8UC3);
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Point> contmax;
	int mode = CV_RETR_TREE;
	cv::cvtColor(edgeImage, pContourImg, CV_GRAY2BGR);
	cv::Mat sege = edgeImage.clone();
	std::vector<cv::Vec4i> h;
	cv::findContours(sege, contours, h, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	cv::drawContours(pContourImg, contours, -1, cv::Scalar(255), 1);
	int area, maxArea = 1;//��������ֵ����10Pixel
	for (int i = 0; i < contours.size(); i++)
	{
		area = fabs(cv::contourArea(contours[i], false));
		printf("area == %lf\n", area);
		if (area > maxArea)
		{
			contmax = contours[i];
			maxArea = area;
		}
	}
	//����ROI����
	cv::namedWindow("contour", 0);
	cv::resizeWindow("contour", 512, 512);
	cv::imshow("contour", pContourImg);
	//��С��Ӿ���
	cv::RotatedRect rect = cv::minAreaRect(contours[68]);
	cv::Point2f rectPoints[4];
	rect.points(rectPoints);
	for (int i = 0; i < 4; i++)
		cv::line(pContourImg, rectPoints[i], rectPoints[(i+1)%4], cv::Scalar(255,255,255));
	//int npts = 4;
	//CvPoint rect_pts[4], *pt = rect_pts;
	//for (int rp = 0; rp<4; rp++)
	//rect_pts[rp] = cvPointFrom32f(rect_pts0[rp]);
	//����Box
	//cvPolyLine(pContourImg, &pt, &npts, 1, 1, CV_RGB(255, 0, 0), 2);
	//cv::rectangle (pContourImg,rect, cv::Scalar(255), 1, 8, 0);
	//cv::polylines(pContourImg, &(points), &npts, 4, 1, cv::Scalar(255), 1, 8, 0);
	cv::namedWindow("box", 0);
	cv::resizeWindow("box", 512, 512);
	cv::imshow("box", pContourImg);
	cvWaitKey(0);
	cv::destroyAllWindows();
	return 0;
}
