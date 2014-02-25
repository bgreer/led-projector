#include "header.h"

// print the rgb values of a strip
// so that they can be copy and pasted
// into arduino code
void print_strip (strip *s)
{
	int ii;
	// red
	printf("static uint8_t r[%d] = {\n\t", s->numpixels);
	for (ii=0; ii<s->numpixels; ii++)
	{
		printf("%3d", s->r[ii]);
		if (ii<s->numpixels-1) printf(",");
		if (ii % 10 == 0 ) printf("\n");
	}
	printf("};\n");
	// green
	printf("static uint8_t g[%d] = {\n\t", s->numpixels);
	for (ii=0; ii<s->numpixels; ii++)
	{
		printf("%3d", s->g[ii]);
		if (ii<s->numpixels-1) printf(",");
		if (ii % 10 == 0) printf("\n");
	}
	printf("};\n");
	// blue
	printf("static uint8_t b[%d] = {\n\t", s->numpixels);
	for (ii=0; ii<s->numpixels; ii++)
	{
		printf("%3d", s->b[ii]);
		if (ii<s->numpixels-1) printf(",");
		if (ii % 10 == 0) printf("\n");
	}
	printf("};\n");
}

void print_space_coords (strip *s)
{
	int ii;
	for (ii=0; ii<s->numpixels; ii++)
	{
		printf("%d\t%f\t%f\t%f\n", ii, s->space_coords[ii][0], s->space_coords[ii][1], s->space_coords[ii][2]);
	}
}

void print_img_coords (strip *s)
{
	int ii;
	for (ii=0; ii<s->numpixels; ii++)
	{
		printf("%d\t%f\t%f\n", ii, s->img_coords[ii][0], s->img_coords[ii][1]);
	}
}

// TODO: make this figure out the number of LEDs needed and allocate a strp for it
void read_led_positions (strip *s, char *fname)
{
	FILE *fp;
	int ii;
	float x, y;

	fp = fopen(fname, "r");

	while (fscanf(fp, "%d\t%f\f%f\n", &ii, &x, &y) != EOF)
	{
		s->img_coords[ii][0] = x*0.5 + 0.5;
		s->img_coords[ii][1] = (y*0.5 + 0.5);
	}

	fclose(fp);
}

void init_strip_cylinder (strip *s, int numpixels, float ledspacing, float r, float zstep)
{
	int ii;
	float x, y, z, l;
	float oldx, oldy, oldz;

	s->numpixels = numpixels;
	// allocate memory
	s->space_coords = (float**) malloc(numpixels * sizeof(float*));
	for (ii=0; ii<numpixels; ii++)
		s->space_coords[ii] = (float*) malloc(3 * sizeof(float));
	s->img_coords = (float**) malloc(numpixels * sizeof(float*));
	for (ii=0; ii<numpixels; ii++)
		s->img_coords[ii] = (float*) malloc(2 * sizeof(float));
	s->r = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->g = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->b = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->sendbuffer = (uint8_t*) malloc( (5 * numpixels + 1) * sizeof(uint8_t));

	// place first LED
	x = r;
	y = 0.0;
	z = 0.0;
	s->space_coords[0][0] = x;
	s->space_coords[0][1] = y;
	s->space_coords[0][2] = z;
	oldx = x;
	oldy = y;
	oldz = z;

	// place each next LED until you run out of LEDs
	ii = 1;
	l = 0.0;
	while (ii<numpixels)
	{
		z += 0.001*zstep;
		x = r*cos(z*TWOPI/zstep);
		y = -r*sin(z*TWOPI/zstep);
		l += sqrt((x-oldx)*(x-oldx) + (y-oldy)*(y-oldy) + (z-oldz)*(z-oldz));
		oldx = x;
		oldy = y;
		oldz = z;
		// check distance travelled against led spacing
		if (l >= ledspacing)
		{
			l = 0.0;
			s->space_coords[ii][0] = x;
			s->space_coords[ii][1] = y;
			s->space_coords[ii][2] = z;
			ii++;
		}
	}

	// project back to image plane [0,1],[0,1]
	// method 1: wrap
	for (ii=0; ii<numpixels; ii++)
	{
		s->img_coords[ii][0] = (PI + atan2(s->space_coords[ii][1],s->space_coords[ii][0]))/(TWOPI);
		s->img_coords[ii][1] = s->space_coords[ii][2] / s->space_coords[numpixels-1][2];
//		printf("%d\t%f\t%f\t%f\t%f\t%f\n", ii, s->space_coords[ii][0], s->space_coords[ii][1], s->space_coords[ii][2], s->img_coords[ii][0], s->img_coords[ii][1]);
	}

	// method 2: flat plane
	for (ii=0; ii<numpixels; ii++)
	{
		s->img_coords[ii][0] = (s->space_coords[ii][0] + r) / (2. * r);
		s->img_coords[ii][1] = s->space_coords[ii][2] / s->space_coords[numpixels-1][2];
	}
/*
	// method 3: uhhh
	float angle = 1.4; // in radians
	for (ii=0; ii<numpixels; ii++)
	{
		s->img_coords[ii][0] = (0.8*s->space_coords[ii][0] + r) / (2. * r);
		s->img_coords[ii][1] = (0.8*s->space_coords[ii][1] + r)*cos(angle) / (2. * r)
			+ s->space_coords[ii][2]*sin(angle);
	}
	*/
}

void clear_strip (strip *s)
{
	// sizeof(uint8_t) should be obvious..
	memset(s->r, 0x00, s->numpixels * sizeof(uint8_t));
	memset(s->g, 0x00, s->numpixels * sizeof(uint8_t));
	memset(s->b, 0x00, s->numpixels * sizeof(uint8_t));
}

void free_strip (strip *s)
{
	int ii;
	for (ii=0; ii<s->numpixels; ii++)
	{
		free(s->space_coords[ii]);
		free(s->img_coords[ii]);
	}
	free(s->space_coords);
	free(s->img_coords);
	free(s->r);
	free(s->g);
	free(s->b);
}


void init_strip_sphere (strip *s, int numpixels, float ledspacing, float r, float turns)
{
	int ii, notplaced;
	float x, y, z, l, r1;
	float oldx, oldy, oldz;

	s->numpixels = numpixels;
	// allocate memory
	s->space_coords = (float**) malloc(numpixels * sizeof(float*));
	for (ii=0; ii<numpixels; ii++)
		s->space_coords[ii] = (float*) malloc(3 * sizeof(float));
	s->img_coords = (float**) malloc(numpixels * sizeof(float*));
	for (ii=0; ii<numpixels; ii++)
		s->img_coords[ii] = (float*) malloc(2 * sizeof(float));
	s->r = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->g = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->b = (uint8_t*) malloc(numpixels * sizeof(uint8_t));
	s->sendbuffer = (uint8_t*) malloc( (5 * numpixels + 1) * sizeof(uint8_t));

	notplaced = 0;
	// place first LED
	x = 0.0;
	y = 0.0;
	z = r;
	s->space_coords[0][0] = x;
	s->space_coords[0][1] = y;
	s->space_coords[0][2] = z;
	oldx = x;
	oldy = y;
	oldz = z;

	// place each next LED until you run out of LEDs
	ii = 1;
	l = 0.0;
	while (ii<numpixels)
	{
		z -= 0.0001;
		if (z < -r)
		{
			s->space_coords[ii][0] = 0.0;
			s->space_coords[ii][1] = 0.0;
			s->space_coords[ii][2] = -r;
			notplaced++;
			ii++;
		} else {
			r1 = sqrt(r*r - z*z);
			x = r1*cos(z*TWOPI*turns/(2.*r));
			y = -r1*sin(z*TWOPI*turns/(2.*r));
			l += sqrt((x-oldx)*(x-oldx) + (y-oldy)*(y-oldy) + (z-oldz)*(z-oldz));
			oldx = x;
			oldy = y;
			oldz = z;
			// check distance travelled against led spacing
			if (l >= ledspacing)
			{
				l = 0.0;
				s->space_coords[ii][0] = x;
				s->space_coords[ii][1] = y;
				s->space_coords[ii][2] = z;
				ii++;
			}
		}
	}

	if (notplaced)
		printf("WARNING: %d out of %d LEDs not placed on sphere\n", notplaced, numpixels);

	// project back to image plane [0,1],[0,1]
	// method 1: wrap
	for (ii=0; ii<numpixels; ii++)
	{
		s->img_coords[ii][0] = (PI + atan2(s->space_coords[ii][1],s->space_coords[ii][0]))/(TWOPI);
		s->img_coords[ii][1] = (s->space_coords[ii][2] / r)*0.5 + 0.5;
	}

	// method 2: flat plane
	
	for (ii=0; ii<numpixels; ii++)
	{
		s->img_coords[ii][0] = (s->space_coords[ii][0] + r) / (2. * r);
		s->img_coords[ii][1] = (s->space_coords[ii][2] / r)*0.5 + 0.5;
	}
	
}

