#include "header.h"


int main (int argc, char *argv[])
{
	Mat imgc, source, res;
	strip s;
	int ii;

	// load and display source image
	source = imread("sparkfun.jpeg", CV_LOAD_IMAGE_UNCHANGED );
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

	print_strip(&s);

	waitKey(0);

	/* close comm */
//	closeComm();

	return 0;
}
