#include <stdio.h>
#include "cv.h"
#include "highgui.h"
#include "blobs.h"


Cblobs blobs;

int main( void )
{
	IplImage* img=cvLoadImage("obraz1.png");
	if(!img)	return 0;

	IplImage* hsv=cvCreateImage( cvGetSize(img), 8, 3 ); //obrazek dla przestrzeni HSV
	IplImage* binary=cvCreateImage( cvGetSize(img), 8, 1 );//tu bedzie maska na podstawie koloru
	cvCvtColor(img,hsv,CV_BGR2HSV); //konwersja na HSV - z tego bedzie robiona maska kolorow

	//szybki sposob dostepu do pikseli obrazu
	uchar* hsv_ptr = (uchar *)hsv->imageData;
	uchar* binary_ptr = (uchar *)binary->imageData;
	int hsv_step = hsv->widthStep;
	int binary_step = binary->widthStep;

	for (int i = 0; i < hsv->height; i++)
		for (int j = 0; j < hsv->width; j++) {
			//odczyt poszczegolnych skladowych
			int h=hsv_ptr[i*hsv_step+j*3+0];
			int s=hsv_ptr[i*hsv_step+j*3+1];
			int v=hsv_ptr[i*hsv_step+j*3+2];

			//utworzenie maski binarnej dla koloru czerwonego

			if(v>=40 && (h>160 || h<20) && s>150) //jasnosc >40, kolor czerwony, nasycenie >150
				binary_ptr[i*binary_step+j] = 255;
			else
				binary_ptr[i*binary_step+j] = 0;
		}


		cvReleaseImage(&hsv); //obraz hsv juz nie jest nam porzebny!

		cvNamedWindow( "Binarny", 1);
		cvShowImage("Binarny",binary);

		blobs.BlobAnalysis(binary, 0, 0, binary->width, binary->height, 0, 10);

		blobs.BlobExclude(BLOBCIRCULARITY,1.1,5);
		blobs.BlobExclude(BLOBCIRCULARITY,.1,.9);

		printf("Znaleziono %d blobow",blobs.BlobCount);
		blobs.PrintRegionDataArray(1);


		//zaznaczenie wykrytych blobow
		for(int i=1;i<=blobs.BlobCount;i++)
		{
			cvRectangle(img,cvPoint(blobs.RegionData[i][BLOBMINX],blobs.RegionData[i][BLOBMINY]),cvPoint(blobs.RegionData[i][BLOBMAXX],blobs.RegionData[i][BLOBMAXY]),CV_RGB(255,0,0));
			cvEllipse(img,cvPoint(blobs.RegionData[i][BLOBSUMX],blobs.RegionData[i][BLOBSUMY]),cvSize(5,5),360, 0, 360, CV_RGB(255,255,255));
		}

		cvNamedWindow( "Podglad", 1 );
		cvShowImage("Podglad",img);

		cvWaitKey(0);
		cvDestroyAllWindows();
		cvReleaseImage(&binary);
		cvReleaseImage(&img);

		return 0;
}