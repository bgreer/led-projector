#include "header.h"


// single-line packet
// USES: fade
// 
void effect_slp (effect *eff, handle *h, float currtime)
{
	
}


// USES: fade
// attr[0]=theta in deg, gives angle to project at
// attr[1]=proj, method of projection (0=planar, 1=cylindrical, 2=spherical)
// str=filename, not keyframed (obviously)
void effect_image (effect *eff, handle *h, float currtime)
{
	int ii;
	Mat *frames;
	uint8_t r, g, b;
	float x, y, thisx, thisy, thisz, theta;

	// sort out frame data
	frames = (Mat*) eff->ptr;

	theta = h->attr[0]*DEGTORAD;
	printf("%f\n", theta);

	imshow("Source", frames[0]);
	for (ii=0; ii<NUMPIXELS; ii++)
	{
		r = 0;
		g = 0;
		b = 0;
		thisx = eff->str->space_coords[ii][0];
		thisy = eff->str->space_coords[ii][1];
		thisz = eff->str->space_coords[ii][2];
		// make x and y [0,1]
		switch ((int)(h->attr[1]))
		{
			case 0: // planar
				x = ((thisx*cos(theta) + thisy*sin(theta)) + SPHERERAD) / (2.*SPHERERAD);
				y = (thisz + SPHERERAD) / (2.*SPHERERAD);
				break;
			case 1: // cylindrical
				x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
				y = (thisz + SPHERERAD) / (2.*SPHERERAD);
				break;
			case 2: // spherical
				x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
				y = (atan2(thisz,sqrt(thisx*thisx+thisy*thisy))+PI) / TWOPI;
				break;
		}
		interp_pixel(&(frames[0]), x, y, &r, &g, &b);
		eff->pixels[0][ii] = ((float)r) * h->fade;
		eff->pixels[1][ii] = ((float)g) * h->fade;
		eff->pixels[2][ii] = ((float)b) * h->fade;
	}

}


// USE: fade
// color1
// color2
// color3
// attr[0] = rate of fire
// attr[1] = duration
// if colorX != 0,0,0 use it
// use eff->data for persistent storage
// TODO: use gradual turn-on, add third color option
void effect_flicker (effect *eff, handle *h, float currtime)
{
	int ii, ij, ik, numcolors;
	float p, gamma, fac;
	float *cdf, *amp, *type;

	amp = &(eff->data[30]);
	type = &(eff->data[40+NUMPIXELS]);

	// count colors
	numcolors = 1;
	if (h->color2[0] > 0.0 || h->color2[1] > 0.0 || h->color2[2] > 0.0) numcolors++;

	// WORK OUT PROBABILITIES
	cdf = &(eff->data[3]);
	if (eff->data[0] == 0.0)
		eff->data[0] = currtime;
	
	// only go if its been a bit of time
	if (currtime - eff->data[0] > 1./h->attr[0])
	{
		gamma = h->attr[0] * (currtime - eff->data[0]);
		eff->data[0] = currtime; // store current time for next time
		// only compute new cdf if gamma has changed enough?
		if (eff->data[2] != gamma)
		{
			eff->data[2] = gamma;
			// create new lookup table
			for (ii=0; ii<20; ii++)
			{
				cdf[ii] = 0.0;
				for (ij=0; ij<ii; ij++)
				{
					// compute factorial of ij
					fac = 1.0;
					for (ik=1;ik<=ij; ik++)
						fac *= (float)ik;
					cdf[ii] += pow(gamma, ij) / fac;
				}
				cdf[ii] *= exp(-gamma);
			}
		}
	
		// figure out how many to fire off
		p = unit_random();
		// find which entry in cdf is next highest
		ii = 0;
		while (cdf[ii] < p && ii<19) ii++;
//		printf("%f %d %f %f %f %f\n", gamma, ii, p, cdf[0], cdf[1], cdf[2]);
		// need to launch ii pixels
		for (ij=0; ij<ii; ij++)
		{
			// pick a pixel, small chance of overlap?
			p = unit_random() * NUMPIXELS;
			amp[(int)floor(p)] = 1.0;
			type[(int)floor(p)] = floor(unit_random()*numcolors);
		}
	}

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		if (type[ii] == 0.0)
		{
			eff->pixels[0][ii] = h->color1[0] * h->fade * amp[ii];
			eff->pixels[1][ii] = h->color1[1] * h->fade * amp[ii];
			eff->pixels[2][ii] = h->color1[2] * h->fade * amp[ii];
		} else {
			eff->pixels[0][ii] = h->color2[0] * h->fade * amp[ii];
			eff->pixels[1][ii] = h->color2[1] * h->fade * amp[ii];
			eff->pixels[2][ii] = h->color2[2] * h->fade * amp[ii];
		}
	}

	// decay amplitudes
	if (eff->data[1] == 0.0) eff->data[1] = currtime;
	for (ii=0; ii<NUMPIXELS; ii++)
		if (amp[ii] > 0.0)
			amp[ii] -= (currtime - eff->data[1]) / h->attr[1];
	eff->data[1] = currtime;
}


// USES: fade
// color1
// color2
// attr[0] = start led
// attr[1] = end led
// attr[2] = speed
// attr[3] = length scale of variation
void effect_ring (effect *eff, handle *h, float currtime)
{
	int ii, start, end;
	float arg;

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));
	
	start = (int)h->attr[0];
	end = (int) h->attr[1];

	if (start < 0) start = 0;
	if (start >= NUMPIXELS) start = NUMPIXELS-1;
	if (end < 0) end = 0;
	if (end >= NUMPIXELS) end = NUMPIXELS-1;
	for (ii=start; ii<=end; ii++)
	{
		arg = (currtime - eff->starttime)*h->attr[2] + ii*PI/h->attr[3];
		eff->pixels[0][ii] = ((h->color1[0]-h->color2[0])*pow(sin(arg),2) + h->color2[0])*h->fade;
		eff->pixels[1][ii] = ((h->color1[1]-h->color2[1])*pow(sin(arg),2) + h->color2[1])*h->fade;
		eff->pixels[2][ii] = ((h->color1[2]-h->color2[2])*pow(sin(arg),2) + h->color2[2])*h->fade;
	}
}



// USES: fade
// color1
// attr[0] = width in pixels
// attr[1] = gaussian width in pixels
// attr[2] = speed
// attr[3] = origin
// for hard edges, attr[1] >> attr[0]
void effect_packet (effect *eff, handle *h, float currtime)
{
	int ii, start, end;
	float amp, loc;

	// figure out where it should be centered
	loc = h->attr[3] + h->attr[2] * (currtime - eff->starttime);

	// clear pixels first
	memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
	memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));

	// set pixels around loc
	start = loc-(int)(h->attr[0]);
	end = loc+(int)(h->attr[0]);
	if (start < 0) start = 0;
	if (start >= NUMPIXELS) start = NUMPIXELS-1;
	if (end < 0) end = 0;
	if (end >= NUMPIXELS) end = NUMPIXELS-1;
	for (ii=start; ii<=end; ii++)
	{
		amp = exp(-pow((((float)ii)-loc)/h->attr[1],2.));
		eff->pixels[0][ii] = h->color1[0] * h->fade * amp;
		eff->pixels[1][ii] = h->color1[1] * h->fade * amp;
		eff->pixels[2][ii] = h->color1[2] * h->fade * amp;
	}
}

// USES: fade
// attr[0]=speed
// attr[1]=theta in deg, gives angle to project at
// attr[2]=proj, method of projection (0=planar, 1=cylindrical, 2=spherical)
// str=filename, not keyframed (obviously)
//
// probably need to pre-load the video file and load each frame into a MAT
// then pass an array of Mats as eff->ptr;
// load number of frames into data[0]
void effect_video (effect *eff, handle *h, float currtime)
{
	int ii;
	Mat *frames;
	int numframes, currframe;
	uint8_t r, g, b;
	float x, y, thisx, thisy, thisz, theta;

	// sort out frame data
	frames = (Mat*) eff->ptr;
	numframes = (int)(eff->data[0]);
	// figure out current frame
	currframe = (int) ((currtime - eff->starttime) * h->attr[0]);

	theta = h->attr[1]*DEGTORAD;

	if (currframe < numframes)
	{
		imshow("Source", frames[currframe]);
		for (ii=0; ii<NUMPIXELS; ii++)
		{
			r = 0;
			g = 0;
			b = 0;
			thisx = eff->str->space_coords[ii][0];
			thisy = eff->str->space_coords[ii][1];
			thisz = eff->str->space_coords[ii][2];
			// make x and y [0,1]
			switch ((int)(h->attr[2]))
			{
				case 0: // planar
					x = ((thisx*cos(theta) + thisy*sin(theta)) + SPHERERAD) / (2.*SPHERERAD);
					y = (thisz + SPHERERAD) / (2.*SPHERERAD);
					break;
				case 1: // cylindrical
					x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
					y = (thisz + SPHERERAD) / (2.*SPHERERAD);
					break;
				case 2: // spherical
					x = fmod(atan2(thisy,thisx) + theta, TWOPI) / TWOPI;
					y = (atan2(thisz,sqrt(thisx*thisx+thisy*thisy))+PI) / TWOPI;
					break;
			}
			interp_pixel(&(frames[currframe]), x, y, &r, &g, &b);
			eff->pixels[0][ii] = ((float)r) * h->fade;
			eff->pixels[1][ii] = ((float)g) * h->fade;
			eff->pixels[2][ii] = ((float)b) * h->fade;
		}
	} else {
		// clear pixels
		memset(eff->pixels[0], 0, NUMPIXELS * sizeof(float));
		memset(eff->pixels[1], 0, NUMPIXELS * sizeof(float));
		memset(eff->pixels[2], 0, NUMPIXELS * sizeof(float));
	}
}

// NOTE ON LON/LAT
// given an x,y,z for each led, finding lat/lon is
// lon = atan(y,x) (scale and offset appropriately)
// lat = atan(z,r)

// circle of light
// USES: color1, fade
// attr[0]=lon, attr[1]=lat, attr[2]=radius in degrees
void effect_circle (effect *eff, handle *h, float currtime)
{
	int ii;
	float lat, lon, r, thisx, thisy, thisz, arg;
	// first, figure out lat/lon for each pixel
//	printf("%f %f %f %f\n", h->attr[2], h->fade, h->attr[0], h->attr[1]);
	for (ii=0; ii<NUMPIXELS; ii++)
	{
		thisx = eff->str->space_coords[ii][0];
		thisy = eff->str->space_coords[ii][1];
		thisz = eff->str->space_coords[ii][2];
		lat = atan2(thisz, sqrt(thisx*thisx+thisy*thisy));
		lon = atan2(thisy, thisx);
		// figure out great-circle distance to target lat-lon
		arg = sin(lat)*sin(h->attr[1] * DEGTORAD) + cos(lat)*cos(h->attr[1] * DEGTORAD)*cos(lon-(h->attr[0] * DEGTORAD));
		r = acos(arg);
		// compare to target distance
//		printf("%d %f %f %f %f\n", ii, lon, lat, r, arg);
		if (fmod(r, PI) <= h->attr[2]*DEGTORAD)
		{
			eff->pixels[0][ii] = h->color1[0] * h->fade;
			eff->pixels[1][ii] = h->color1[1] * h->fade;
			eff->pixels[2][ii] = h->color1[2] * h->fade;
		} else {
			eff->pixels[0][ii] = 0;
			eff->pixels[1][ii] = 0;
			eff->pixels[2][ii] = 0;
		}
	}
}


// set entire strip to one color
// USES: color1
void effect_solid (effect *eff, handle *h, float currtime)
{
	int ii;

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		eff->pixels[0][ii] = h->color1[0] * h->fade;
		eff->pixels[1][ii] = h->color1[1] * h->fade;
		eff->pixels[2][ii] = h->color1[2] * h->fade;
	}
	
}

// pulse entire strip between two colors
// USES: color1, color2, attr[0]
// attr[0] is cycles per second
void effect_pulse (effect *eff, handle *h, float currtime)
{
	int ii;
	float arg;

	arg = PI * h->attr[0] * (currtime - eff->starttime);

	for (ii=0; ii<NUMPIXELS; ii++)
	{
		eff->pixels[0][ii] = ((h->color1[0]-h->color2[0])*pow(sin(arg),2) + h->color2[0])*h->fade;
		eff->pixels[1][ii] = ((h->color1[1]-h->color2[1])*pow(sin(arg),2) + h->color2[1])*h->fade;
		eff->pixels[2][ii] = ((h->color1[2]-h->color2[2])*pow(sin(arg),2) + h->color2[2])*h->fade;
	}
	
}

float unit_random ()
{
	return ((float)rand())/((float)RAND_MAX);
}

