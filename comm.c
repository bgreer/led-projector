#include "header.h"
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int fd;
struct termios options;

void openComm ()
{

	fd = open("/dev/ttyACM3", O_RDWR | O_NOCTTY | O_SYNC);
	
	if (fd == -1)
	{
		printf("ERROR: Unable to open comm port!\n");
//	} else {
//		fcntl(fd, F_SETFL, 0);
	}

	memset(&options, 0, sizeof(options));
	cfsetispeed(&options, B115200);
	cfsetispeed(&options, B115200);
	options.c_cflag = (options.c_cflag & ~CSIZE) | CS8; // 8-bit chars
	options.c_iflag &= ~IGNBRK; // ignore break chars
	options.c_lflag = 0; // no signaling chars, no echo, no canonical processing
	options.c_oflag = 0; // no remapping, no delays
	options.c_cc[VMIN]  = 0; // reading doesnt block
	options.c_cc[VTIME] = 5; // 0.5s readout time
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	options.c_cflag |= (CLOCAL | CREAD); // ignore modem controls, enable reading
	options.c_cflag &= ~(PARENB | PARODD); // shut off parity
	options.c_cflag |= 0; // parity option
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &options) != 0)
	{
		printf("error %d from tcsetattr", errno);
	}
//	fcntl(fd, F_SETFL, FNDELAY);
}

void setPixels(strip *s)
{
	int ii;
	uint8_t *buffer, r, g, b;

	buffer = (uint8_t*) malloc( 5 * s->numpixels * sizeof(uint8_t));

	for (ii=0; ii<s->numpixels; ii++)
	{
		r = s->r[ii] / 3;
		g = s->g[ii] / 3;
		b = s->b[ii] / 3;
		if (r >= 254) r = 253;
		if (g >= 254) g = 253;
		if (b >= 254) b = 253;
		buffer[ii*5+0] = 0xff;
		buffer[ii*5+1] = (uint8_t)(ii);
		buffer[ii*5+2] = (uint8_t)(r);
		buffer[ii*5+3] = (uint8_t)(g);
		buffer[ii*5+4] = (uint8_t)(b);
	}
	write(fd, buffer, 5*s->numpixels);
}

void sendShow()
{
	uint8_t buffer[1];
	buffer[0] = 0xfe;
	write(fd, buffer, 1);
}

void closeComm ()
{
	close(fd);
}
