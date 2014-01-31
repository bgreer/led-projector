#include "header.h"
#include <time.h>

int main (int argc, char *argv[])
{
	Mat imgc, source, res, f1, f2, sub, sub_g;
	Mat vidframe;
	strip s;
	scanner scan;
	VideoCapture cap1(0);
	VideoCapture cap2(1);
	int ii, index;

	// load and display source image
	source = imread("colorbars.jpeg", CV_LOAD_IMAGE_UNCHANGED );
	namedWindow( "Source", CV_WINDOW_AUTOSIZE );
	imshow("Source", source);

	imgc.create(300,300,CV_8UC3);
	imgc.setTo(0);

	// create strip
	init_strip_cylinder(&s, 243, 0.0162, 0.078, 0.0185);

	// make image of led locations on source image
	for (ii=0; ii<s.numpixels; ii++)
		circle(imgc, Point(300*s.img_coords[ii][0],300*s.img_coords[ii][1]), 3, Scalar(255,255,255), -1);
	namedWindow( "Image Coords", CV_WINDOW_AUTOSIZE );
	imshow("Image Coords", imgc);

	/* open serial port */
	openComm();

	map_image(&source, &s);


	res.create(300,300,CV_8UC3);
	res.setTo(0);
	for (ii=0; ii<s.numpixels; ii++)
		circle(res, Point(300*s.img_coords[ii][0],300*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
	namedWindow( "Result", CV_WINDOW_AUTOSIZE );
	imshow("Result", res);


	setPixels(&s);
	sendShow();

	waitms(1500);
//	while (1) {}

	VideoCapture vid("nasa.mp4");
	namedWindow( "vid", CV_WINDOW_AUTOSIZE );
	index = 0;
	while (1)
	{
		vid.read(vidframe);
		if(!vidframe.empty())
		{
			imshow("vid", vidframe);
			if (index % 2 == 0)
			{
				GaussianBlur(vidframe, vidframe, Size(0,0), 7.0);
				map_image(&vidframe, &s);
				setPixels(&s);
				sendShow();
			}
			res.setTo(0);
			for (ii=0; ii<s.numpixels; ii++)
				circle(res, Point(300*s.img_coords[ii][0],300*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
			imshow("Result", res);
			index++;
		}
		waitKey(20);
	}

/*
	// initialize scanner, cameras
	scan.numcams = 0;
	add_camera(&scan, &cap1);
	add_camera(&scan, &cap2);

	// display camera images live for setup
	namedWindow( "cam1", CV_WINDOW_AUTOSIZE );
	namedWindow( "cam2", CV_WINDOW_AUTOSIZE );
	printf("PRESS SPACE TO BEGIN SCANNING\n");
	while(1)
	{
		grab_frame(&scan, 0, 0);
		circle(scan.backframe[0], Point(scan.backframe[0].cols/2,scan.backframe[0].rows/2), 
			10, Scalar(255,255,255), 2);
		imshow("cam1", scan.backframe[0]);
		grab_frame(&scan, 1, 0);
		circle(scan.backframe[1], Point(scan.backframe[1].cols/2,scan.backframe[1].rows/2), 
			10, Scalar(255,255,255), 2);
		imshow("cam2", scan.backframe[1]);
		// continue after pressing space
		if (char(cvWaitKey(1)) == 32) // 27 is esc
			break;
	}
	start_scan(&scan, &s);
*/

//	print_strip(&s);

	waitKey(0);


	/* close comm */
	closeComm();

	return 0;
}

void waitms (long ms)
{
	clock_t t;
	t = clock();
	while ((clock()-t)*1000/CLOCKS_PER_SEC < ms) {}
}
