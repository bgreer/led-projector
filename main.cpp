#include "header.h"
#include "parse_params.h"
#include <time.h>

/*
	this code has become kind of stringy. fix it up.
*/

int main (int argc, char *argv[])
{
	Mat imgc, source, res, f1, f2, sub, sub_g;
	Mat vidframe;
	Mat *allframes;
	strip s;
	scanner scan;
	VideoCapture cap1(0);
//	VideoCapture cap2(2);
	char buffer[32];
	int ii, ij, index, numframes, pause;
	// for sequencer
	effect eff[MAXEFFECTS];
	handle h[MAXHANDLES];
	int numeffects;
	float currtime, starttime;
	float pixels[3][NUMPIXELS];
	// command-line stuff
	tag t[7];
	int geom_cyl, geom_sph, geom_cam, geom_file;
	char portname[128], fname_seq[128];
	int retval, port_provided;

	// timing
	struct timespec time_curr, time_last;
	float dt;

	srand(time(NULL));

	// collect some cameras from the command-line?
	starttime = 0.0;
	geom_cam = 0;
	geom_cyl = 0;
	geom_sph = 1;
	geom_file = 0;
	port_provided = 0;
	t[0].name = "-cams";
	t[0].type = TAGTYPE_BOOL;
	t[0].data = &geom_cam;
	t[1].name = "-cylinder";
	t[1].type = TAGTYPE_BOOL;
	t[1].data = &geom_cyl;
	t[2].name = "-sphere";
	t[2].type = TAGTYPE_BOOL;
	t[2].data = &geom_sph;
	t[3].name = "-port";
	t[3].type = TAGTYPE_STRING;
	t[3].data = portname;
	t[4].name = "-file"; // read led positions from file
	t[4].type = TAGTYPE_BOOL;
	t[4].data = &geom_file;
	t[5].name = "-seq"; // sequence file
	t[5].type = TAGTYPE_STRING;
	t[5].data = fname_seq;
	t[6].name = "-time";
	t[6].type = TAGTYPE_FLOAT;
	t[6].data = &starttime;

	retval = parse_params(argc, argv, 7, t);

	// connect to arduino
	if (strcmp(portname,"")==0)
	{
		printf("Incorrect number of command-line arguments.\n Include serial port for Arduino comms.\n");
		openComm("/dev/null");
	} else {
		/* open serial port */
		openComm(portname);
	}

	printf("Using geometry: ");

	// create strip geometry
	if (geom_cyl)
	{
		printf("Cylinder\n");
		init_strip_cylinder(&s, 243, 0.0162, 0.078, 0.0185);
	}
	if (geom_sph)
	{
		printf("Sphere\n");
		init_strip_sphere(&s, 232, 0.0162, SPHERERAD, 8.0);
	}
	if (geom_file)
	{
		printf("From File\n");
		read_led_positions(&s, "pos01");
	}



	// load and display source image
	source = imread("colorbars.jpeg", CV_LOAD_IMAGE_UNCHANGED );
	namedWindow( "Source", CV_WINDOW_AUTOSIZE );
	imshow("Source", source);

	imgc.create(source.cols,source.rows,CV_8UC3);
	imgc.setTo(0);

	
	// make image of led locations on source image
	for (ii=0; ii<s.numpixels; ii++)
		circle(imgc, Point(source.rows*s.img_coords[ii][0],source.cols*s.img_coords[ii][1]), 3, Scalar(255,255,255), -1);
	namedWindow( "Image Coords", CV_WINDOW_AUTOSIZE );
	imshow("Image Coords", imgc);


	// projection
	map_image(&source, &s);


	res.create(source.cols,source.rows,CV_8UC3);
	res.setTo(0);
	for (ii=0; ii<s.numpixels; ii++)
		if (s.space_coords[ii][1] >= 0.0)
			circle(res, Point(source.rows*s.img_coords[ii][0],source.cols*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
	namedWindow( "Result", CV_WINDOW_AUTOSIZE );
	imshow("Result", res);


	setPixels(&s);


	//////////////////////////////////////////////////////////////////
	
	printf("Starting Sequencer..\n");
	load_sequence_file(fname_seq, eff, &numeffects, h, &s);

	printf("Preloading Videos and Images..\n");
	// go through each effect, see if handle.preload = 1
	for (ii=0; ii<numeffects; ii++)
	{
		if (h[eff[ii].handleindex].preload)
		{
			if (eff[ii].run==effect_video)
			{
				printf("  Preloading video for handle %d..\n", eff[ii].handleindex);
				// hackety hack
				VideoCapture vid(h[eff[ii].handleindex].file);
				numframes = vid.get(CV_CAP_PROP_FRAME_COUNT);
				eff[ii].data[0] = (float)numframes;
				printf("  Found %d frames in %s\n", numframes, h[eff[ii].handleindex].file);
				allframes = new Mat[numframes];
				printf("  Reading..\n");
				// force sequential read because i dont trust opencv
				for (ij=0; ij<numframes; ij++)
				{
					vid.set(CV_CAP_PROP_POS_FRAMES, ij);
					vid.read(allframes[ij]);
//					GaussianBlur(allframes[ij], allframes[ij], Size(0,0), 5.0);
				}
				eff[ii].ptr = allframes;
			} else {
				printf("  Preloading image for handle %d.. (%s)\n", eff[ii].handleindex, h[eff[ii].handleindex].file);
				allframes = new Mat[1];
				allframes[0] = imread(h[eff[ii].handleindex].file, CV_LOAD_IMAGE_UNCHANGED );
				eff[ii].ptr = allframes;
			}
		}
	}
	printf("Done Preloading.\n");

	currtime = starttime;
	printf("Press Space to Begin at Time=%f.\n", currtime);
	while (1)
	{
			if (char(cvWaitKey(1)) == 32) // 27 is esc
			break;
	}

	pause = 0;
	clock_gettime(CLOCK_MONOTONIC, &time_last);
	while (currtime < 60.0*20.0)
	{
		memset(pixels[0], 0, NUMPIXELS*sizeof(float));
		memset(pixels[1], 0, NUMPIXELS*sizeof(float));
		memset(pixels[2], 0, NUMPIXELS*sizeof(float));
		// look through list of effects
		for (ii=0; ii<numeffects; ii++)
		{
			// is this effect supposed to be running?
			if (eff[ii].starttime <= currtime)
			{
				// update the handle
				update_handle(&(h[eff[ii].handleindex]), currtime);
				// run effect
				eff[ii].run(eff+ii, h+eff[ii].handleindex, currtime);
				// add to primary pixel buffer
				for (ij=0; ij<NUMPIXELS; ij++)
				{
					pixels[0][ij] += eff[ii].pixels[0][ij];
					pixels[1][ij] += eff[ii].pixels[1][ij];
					pixels[2][ij] += eff[ii].pixels[2][ij];
				}
			}
		}
		// process primary pixel buffer
		// gamma correction, bounding
		for (ij=0; ij<NUMPIXELS; ij++)
		{
			s.r[ij] = process_pixelval(pixels[0][ij]);
			s.g[ij] = process_pixelval(pixels[1][ij]);
			s.b[ij] = process_pixelval(pixels[2][ij]);
		}
		// send processed pixel buffer to led driver
		printf("sending pixels at time = %d:%05.2f\n", (int)floor(currtime/60.), fmod(currtime,60.));
//		res.setTo(0);
		for (ii=0; ii<s.numpixels; ii++)
		{
			circle(res, Point(source.rows*s.img_coords[ii][0],source.cols*s.img_coords[ii][1]), 3, Scalar(0,0,0), -1);
			circle(res, Point(source.rows*s.img_coords[ii][0],source.cols*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
		}
		imshow("Result", res);


		setPixels(&s);
		clock_gettime(CLOCK_MONOTONIC, &time_curr);
		dt = (timespec_to_sec(&time_curr) - timespec_to_sec(&time_last));
//		printf("%f\n", 1./dt);
		currtime += dt; // make this real-time?
		clock_gettime(CLOCK_MONOTONIC, &time_last);
		if (char(cvWaitKey(1)) == 32) // 27 is esc
		{
			pause = 1;
			while (pause)
			{
				if ((char(cvWaitKey(1)) == 32)) pause = 0;
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////


//		sendShow();
	// BEGIN MOVIE PROJECTION
/*
	VideoCapture vid("adventure.mp4");
	namedWindow( "vid", CV_WINDOW_AUTOSIZE );
	index = 0;
	while (1)
	{
		vid.read(vidframe);
		if(!vidframe.empty())
		{
			imshow("vid", vidframe);
			if (index % 1 == 0)
			{
				GaussianBlur(vidframe, vidframe, Size(0,0), 5.0);
				map_image(&vidframe, &s);
				setPixels(&s);
				sendShow();
			}
			res.setTo(0);
			for (ii=0; ii<s.numpixels; ii++)
				circle(res, Point(300*s.img_coords[ii][0],300*s.img_coords[ii][1]), 3, Scalar(s.b[ii],s.g[ii],s.r[ii]), -1);
			imshow("Result", res);
			index++;
			waitms(100);
		}
		waitKey(20);
	}
	*/

	// BEGIN 3D SCANNING
/*
	// initialize scanner, cameras
	cap1.set(15,-2);
	scan.numcams = 0;
	add_camera(&scan, &cap1);
	// view object 15.8cm long at distance of 32cm
	// gives 13.87deg for half x-plane
	// or 27.7 for full x range
	scan.camfov[0] = 27.7;
//	add_camera(&scan, &cap2);
//	scan.camfov[1] = 27.7;

	// display camera images live for setup
	namedWindow( "cam1", CV_WINDOW_AUTOSIZE );
//	namedWindow( "cam2", CV_WINDOW_AUTOSIZE );
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

	printf("MEASURE CAMERA LOCATIONS AND ENTER HERE:\n");
	printf("(Camera directions are assumed to be towards common origin)\n");
	for (ii=0; ii<scan.numcams; ii++)
	{
		printf("Current Cam: %d of %d\n", ii+1, scan.numcams);
		memset(buffer,0,32);
		getLine("CamX>", buffer, 32);
		scan.campos[ii][0] = atof(buffer);
		memset(buffer,0,32);
		getLine("CamY>", buffer, 32);
		scan.campos[ii][1] = atof(buffer);
		memset(buffer,0,32);
		getLine("CamZ>", buffer, 32);
		scan.campos[ii][2] = atof(buffer);
	}

	start_scan(&scan, &s);
	// still need to project to image coords
*/
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

// because im lazy:
// http://stackoverflow.com/questions/4023895/how-to-read-string-entered-by-user-in-c
static int getLine (char *prmpt, char *buff, size_t sz) {
	int ch, extra;

	// Get line with buffer overrun protection.
	if (prmpt != NULL) {
		printf ("%s", prmpt);
		fflush (stdout);
	}
	if (fgets (buff, sz, stdin) == NULL)
		return -1;

	// If it was too long, there'll be no newline. In that case, we flush
	// to end of line so that excess doesn't affect the next call.
	if (buff[strlen(buff)-1] != '\n') {
		extra = 0;
		while (((ch = getchar()) != '\n') && (ch != EOF))
			extra = 1;
			return (extra == 1) ? -1 : 0;
		}

	// Otherwise remove newline and give string back to caller.
	buff[strlen(buff)-1] = '\0';
	return 0;
}

uint8_t process_pixelval (float val)
{
	float proc, b, gain;

	// apply gamma correction
	gain = 255.0;
	b = 255.0 / (exp(255./gain) - 1.0);
	proc = b*(exp(val/gain)-1.0);

	// bound
	if (proc > 255.0) proc = 255.0;
	if (proc < 0.0) proc = 0.0;

	return (uint8_t) proc;
}

float timespec_to_sec (struct timespec *t)
{
	return (float)(t->tv_sec) + 1e-9 * (uint32_t)(t->tv_nsec);
}

