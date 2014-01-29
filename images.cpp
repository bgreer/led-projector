#include "header.h"

void map_image (Mat *img, strip *s)
{
	int ii;
	uint8_t r, g, b;

	for (ii=0; ii<s->numpixels; ii++)
	{
		interp_pixel(img, s->img_coords[ii][0], s->img_coords[ii][1], 
				&r, &g, &b);
		s->r[ii] = r;
		s->g[ii] = g;
		s->b[ii] = b;

	}
}

// assume x and y are [0,1]
void interp_pixel (Mat *img, float x, float y, uint8_t *r, uint8_t *g, uint8_t *b)
{
	float xpix, ypix;
	int xint, yint, index;
	uint8_t *ptr;

	ptr = (uint8_t*) img->data;

	xpix = x * img->cols;
	ypix = y * img->rows;

	// nearest-neighbor
	xint = (int)(floor(xpix));
	yint = (int)(floor(ypix));


	index = yint*img->cols + xint;
	*b = ptr[index*3 + 0];
	*g = ptr[index*3 + 1];
	*r = ptr[index*3 + 2];


}

