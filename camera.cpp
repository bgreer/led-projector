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

void start_scan (scanner *sc, strip *st)
{
	int ii, ij, count, led;
	float x, y;

	for (led=0; led<st->numpixels; led++)
	{
		// take background
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->grab();
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->retrieve(sc->backframe[ii]);

		// turn on led
		printf("Turning on LED %d\n", led);

		// take a few blank pictures to catch up
		// why do i need to do this?
		// it's dumb
		for (ij=0; ij<10; ij++)
			for (ii=0; ii<sc->numcams; ii++)
				sc->cap[ii]->grab();
		// take foreground image
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->grab();
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->retrieve(sc->foreframe[ii]);

		// turn off led
		printf("Turning off LED %d\n", led);

		// locate led and count successes
		count = 0;
		for (ii=0; ii<sc->numcams; ii++)
			count += find_led(sc, ii, &x, &y);

		if (count > 2)
		{
			printf("Locating LED using optimization method..\n");
		} else if (count == 2) {
			printf("Locating LED using closest point method..\n");
		} else {
			printf("Not enough cameras found LED $d\n", led);
		}

	}

}

void add_camera (scanner *s, VideoCapture *cap)
{
	s->cap[s->numcams] = cap;
	s->numcams++;
}

void grab_frame (scanner *s, int cam, int frame)
{
	if (frame==0) s->cap[cam]->read(s->backframe[cam]);
	if (frame==1) s->cap[cam]->read(s->foreframe[cam]);
}

// find bright point in subtraction
// locations are [-1,1]
int find_led (scanner *s, int cam, float *x, float *y)
{
	int ii, ij;
	float px, py, w, val;
	double max, min;
	uint8_t *ptr;
	Mat sub;

	px = 0.0;
	py = 0.0;
	w = 0.0;

	// create b&w subtraction
	sub.create(s->backframe[cam].cols, s->backframe[cam].rows, CV_8UC1);
	cvtColor(s->foreframe[cam] - s->backframe[cam], sub, CV_BGR2GRAY);

	// find min and max
	minMaxIdx(sub, &min, &max);

	ptr = (uint8_t*) sub.data;

	for (ii=0; ii<sub.rows; ii++)
	{
		for (ij=0; ij<sub.cols; ij++)
		{
			val = ptr[ii*sub.cols+ij];
			if (val < 0.9*max) val = 0.0;
			px += val * ij;
			w += val;
			py += val * ii;
		}
	}
	*x = ((px/w)/sub.cols)*2.-1.;
	*y = ((py/w)/sub.rows)*2.-1.;

	// TODO: return 0 if no LED found
	return 1;
}
