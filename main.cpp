#include "header.h"


int main (int argc, char *argv[])
{
	Mat imgc, source, res, f1, f2, sub, sub_g;
	strip s;
	int ii;

	// load and display source image
	source = imread("doge.jpeg", CV_LOAD_IMAGE_UNCHANGED );
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
//	openComm();

	map_image(&source, &s);

	res.create(300,300,CV_8UC3);
	res.setTo(0);
	for (ii=0; ii<s.numpixels; ii++)
		circle(res, Point(300*s.img_coords[ii][0],300*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
	namedWindow( "Result", CV_WINDOW_AUTOSIZE );
	imshow("Result", res);

	namedWindow( "cam", CV_WINDOW_AUTOSIZE );
	VideoCapture cap(1);
	cap.set(10,0.5);
	while (1)
	{
		cap.read(f1);
		imshow("cam", f1);
		if (char(cvWaitKey(10)) == 27)
			break;
	}
	sub_g.create(f1.cols, f1.rows, CV_8UC1);
	float x, y;
	while (1)
	{
		cap.read(f2);
		sub = f2-f1;
		cvtColor(sub, sub_g, CV_BGR2GRAY);
		find_led(&sub_g, &x, &y);
		printf("%f\t%f\n", x, y);
		circle(sub, Point(x,y), 20, Scalar(255,100,100), 1);
		imshow("cam", sub);

	//	imshow("cam", 2.*(f2-f1));
		if (char(cvWaitKey(10)) == 27)
			break;
	}

//	print_strip(&s);

	waitKey(0);

	/* close comm */
//	closeComm();

	return 0;
}
