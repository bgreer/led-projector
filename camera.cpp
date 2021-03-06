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
	float xs[MAXCAMS], ys[MAXCAMS];
	// for two-point method
	float dist, ang;
	float a1[3], a2[3], n1[3], n2[3], nt[3];
	float M[2][2], b[2], t[2];
	float p[3], nn, coef;

	// clear all pixels
	clear_strip(st);
	setPixels(st);
	sendShow();

	for (led=0; led<st->numpixels; led++)
	{
		// take background
		for (ij=0; ij<3; ij++)
			for (ii=0; ii<sc->numcams; ii++)
				sc->cap[ii]->grab();
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->grab();
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->retrieve(sc->backframe[ii]);


//		imshow("back", sc->backframe[0]);

		// turn on led
		clear_strip(st);
		st->r[led] = 50;
		st->g[led] = 50;
		st->b[led] = 50;
		setPixels(st);
		sendShow();

		// take a few blank pictures to catch up
		// why do i need to do this?
		// it's dumb
		for (ij=0; ij<3; ij++)
			for (ii=0; ii<sc->numcams; ii++)
				sc->cap[ii]->grab();
		// take foreground image
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->grab();
		for (ii=0; ii<sc->numcams; ii++)
			sc->cap[ii]->retrieve(sc->foreframe[ii]);

//		imshow("fore", sc->foreframe[0]);

		// turn off led
		clear_strip(st);
		setPixels(st);
		sendShow();

		// locate led and count successes
		count = 0;
		for (ii=0; ii<sc->numcams; ii++)
			count += find_led(sc, ii, &(xs[ii]), &(ys[ii]));

		Mat temp = sc->foreframe[0] - sc->backframe[0];
		if (count) circle(temp, Point((xs[0]*0.5+0.5)*temp.cols, (ys[0]*0.5+0.5)*temp.rows), 10, Scalar(255,255,255),2);
		imshow("cam1", temp);
//		char str[256];
//		sprintf(str, "frame%04d.jpg", led);
	//	printf("%s\n", str);
//		imwrite( str, temp );
	//	while(1) {
		if (char(cvWaitKey(1)) == 32) break;
	//	}

		if (count > 2)
		{
			printf("Locating LED using optimization method..\n");
		} else if (count == 2) {
			printf("Locating LED using closest point method..\n");
			/* each led location defines a line \vec{x} = \vec{a} + t * \vec{n}
				where \vec{a} is the location of the camera
				and \vec{n} is the camera-to-led unit vector

				with two cameras, we just want to know the point closest to each
				this involves solving for t, the parameterized 'distance' along
				each line that gives us the closest approach position
				Once t has been found for each line, solve for \vec{a} on each
				and take the average
			*/

			// for simplicity, load up a1[3], a2[3], n1[3], n2[3]
			memcpy(a1, sc->campos[0], 3*sizeof(float));
			memcpy(a2, sc->campos[1], 3*sizeof(float));
			// assume cameras are pointed at origin.
			// also assume image y-axis is global z-axis (cam is not rotated)
			// start by defining normal relative to boring camera
			// that is, a camera at [1,0,0] pointed at [0,0,0]
			// led is in y-z plane
			// FIRST CAMERA
			// TODO: fix dist to consider camera aspect ratio. camfov is x-fov only
			dist = DEGTORAD * sc->camfov[0] * sqrt(xs[0]*xs[0] + ys[0]*ys[0]);
			ang = atan2(-ys[0], xs[0]);
			printf("led pos %f %f is at %f rad from axis, %f ang azim\n", xs[0], ys[0], dist, ang);
			n1[0] = -1.0*cos(dist);
			n1[1] = (1.0*sin(dist)) * cos(ang);
			n1[2] = (1.0*sin(dist)) * sin(ang);
			// then rotate up/down based on camera zpos
			dist = sqrt(sc->campos[0][0]*sc->campos[0][0] + sc->campos[0][1]*sc->campos[0][1]);
			ang = -atan2(sc->campos[0][2], dist);
			printf("cam 0 dist in xy is %f, ang is %f rad\n", dist, ang);
			nt[0] = cos(ang)*n1[0] + sin(ang)*n1[2];
			nt[1] = n1[1];
			nt[2] = -sin(ang)*n1[0] + cos(ang)*n1[2];
			// then rotate around z based on cam xpos,ypos
			ang = atan2(sc->campos[0][1], sc->campos[0][0]);
//			printf("cam 0 is %f rad around z axis\n", ang);
			n1[0] = cos(ang)*nt[0] + sin(ang)*nt[1];
			n1[1] = -sin(ang)*nt[0] + cos(ang)*nt[1];
			n1[2] = nt[2];
			printf("cam 0 normal vec is %f, %f, %f\n", n1[0], n1[1], n1[2]);
			// NEXT CAMERA
			dist = DEGTORAD * sc->camfov[1] * sqrt(xs[1]*xs[1] + ys[1]*ys[1]);
			ang = atan2(-ys[1], xs[1]);
			printf("led pos %f %f is at %f rad from axis, %f ang azim\n", xs[1], ys[1], dist, ang);
			n2[0] = -1.0*cos(dist);
			n2[1] = (1.0*sin(dist)) * cos(ang);
			n2[2] = (1.0*sin(dist)) * sin(ang);
			// then rotate up/down based on camera zpos
			dist = sqrt(sc->campos[1][0]*sc->campos[1][0] + sc->campos[1][1]*sc->campos[1][1]);
			ang = -atan2(sc->campos[1][2], dist);
			printf("cam 1 dist in xy is %f, ang is %f rad\n", dist, ang);
			nt[0] = cos(ang)*n2[0] + sin(ang)*n2[2];
			nt[1] = n2[1];
			nt[2] = -sin(ang)*n2[0] + cos(ang)*n2[2];
			// then rotate around z based on cam xpos,ypos
			ang = atan2(sc->campos[1][1], sc->campos[1][0]);
//			printf("cam 1 is %f rad around z axis\n", ang);
			n2[0] = cos(ang)*nt[0] + sin(ang)*nt[1];
			n2[1] = -sin(ang)*nt[0] + cos(ang)*nt[1];
			n2[2] = nt[2];
			printf("cam 1 normal vec is %f, %f, %f\n", n2[0], n2[1], n2[2]);

			// camera positions and led normals have been defined
			// \vec{t} = M^{-1} \vec{b}
			nn = n1[0]*n2[0] + n1[1]*n2[1] + n1[2]*n2[2];
			coef = 1.0/(nn*nn - 1.0);
			t[0] = coef * ((n1[0]-n2[0]*nn)*(a1[0]-a2[0]) + (n1[1]-n2[1]*nn)*(a1[1]-a2[1]) + (n1[2]-n2[2]*nn)*(a1[2]-a2[2]));
			t[1] = coef * ((n1[0]*nn-n2[0])*(a1[0]-a2[0]) + (n1[1]*nn-n2[1])*(a1[1]-a2[1]) + (n1[2]*nn-n2[2])*(a1[2]-a2[2]));
			// SOLVE FOR POINT
			p[0] = 0.5 * (a1[0] + a2[0] + t[0]*n1[0] + t[1]*n2[0]); //
			p[1] = 0.5 * (a1[1] + a2[1] + t[0]*n1[1] + t[1]*n2[1]); //
			p[2] = 0.5 * (a1[2] + a2[2] + t[0]*n1[2] + t[1]*n2[2]); //
			// CHECK POINT
			// ?
			// SET STRIP LED COORDS
			printf("TWOPT\t%f\t%f\t%f\t%f\t%f\n", p[0], p[1], p[2], t[0], t[1]);
			memcpy(st->space_coords[led], p, 3*sizeof(float));
		} else {
//			printf("Not enough cameras found LED %d\n", led);
			if (count) printf("%d\t%f\t%f\n", led, xs[0], ys[0]);
			else printf("LED %d not found\n", led);
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
	if (max > 110.)
		return 1;
	else
		return 0;
}
