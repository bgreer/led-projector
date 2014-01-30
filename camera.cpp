#include "header.h"


void display_webcam()
{
	VideoCapture cap(0);
	Mat frame;

	if (!cap.isOpened())
		printf("could not open video\n");

	namedWindow("MyVideo",CV_WINDOW_AUTOSIZE);

	while (1)
	{
		cap.read(frame);
		imshow("MyVideo", frame);
		if (char(cvWaitKey(10)) == 27)
			break;
	}
}


void grab_frame(Mat *frame)
{
	printf("grabbing frame\n");
}

void find_led (Mat *img, float *x, float *y)
{
	int ii, ij;
	float px, py, wx, wy, val;
	double max, min;
	uint8_t *ptr;

	px = 0.0;
	py = 0.0;
	wx = 0.0;
	wy = 0.0;

	minMaxIdx(*img, &min, &max);

	ptr = (uint8_t*) img->data;

	for (ii=0; ii<img->rows; ii++)
	{
		for (ij=0; ij<img->cols; ij++)
		{
			val = ptr[ii*img->cols+ij];
			if (val < 0.9*max) val = 0.0;
			px += val * ij;
			wx += val;
			py += val * ii;
		}
	}
	*x = px/wx;
	*y = py/wx;
}
