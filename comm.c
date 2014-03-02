#include "header.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <termios.h>
#include <errno.h>

int fd;
struct termios options;
struct serial_struct kernel_serial_settings;

void openComm (char *fname)
{
	int r;
	fd = open(fname, O_RDWR);
	if (fd < 0) printf("unable to open port %s\n", fname);
	if (tcgetattr(fd, &options) < 0) printf("unable to get serial parms\n");
	cfmakeraw(&options);
	if (cfsetspeed(&options, 115200) < 0) printf("error in cfsetspeed\n");
	if (tcsetattr(fd, TCSANOW, &options) < 0) printf("unable to set baud rate\n");
	r = ioctl(fd, TIOCGSERIAL, &kernel_serial_settings);
	if (r >= 0)
	{
		kernel_serial_settings.flags |= ASYNC_LOW_LATENCY;
		r = ioctl(fd, TIOCSSERIAL, &kernel_serial_settings);
		if (r >= 0) printf("set linux low latency mode\n");
	}
}

void openComm_old (char *fname)
{

	fd = open(fname, O_RDWR | O_NOCTTY | O_SYNC);
	
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
	int ii, ebsize, rev;
	uint8_t r, g, b;
	uint8_t *extrabuffer;
	ssize_t ret;

	ebsize = (int)ceil(((float)(5*s->numpixels+1))/1024.)*1024. - (5*s->numpixels+1);
	extrabuffer = (uint8_t*) malloc(ebsize);
	memset(extrabuffer, 0x00, ebsize);
//	printf("padding package by %d bytes\n", ebsize);

	for (rev=0; rev<s->numpixels; rev++)
	{
		ii = NUMPIXELS-rev-1;
		r = s->r[ii] / 2;
		g = s->g[ii] / 2;
		b = s->b[ii] / 2;
		if (r >= 254) r = 253;
		if (g >= 254) g = 253;
		if (b >= 254) b = 253;
		s->sendbuffer[rev*5+0] = 0xff;
		s->sendbuffer[rev*5+1] = rev;
		s->sendbuffer[rev*5+2] = r;
		s->sendbuffer[rev*5+3] = g;
		s->sendbuffer[rev*5+4] = b;
	}
	s->sendbuffer[5*s->numpixels] = 0xfe;
//	printf("sending %d bytes\n", 5*s->numpixels + 1);
	ret = write(fd, s->sendbuffer, 5*s->numpixels + 1);
	ret = write(fd, extrabuffer, ebsize);
	free(extrabuffer);
	waitms(10);

}

void sendShow()
{
	ssize_t ret;
	uint8_t buffer[1];
	buffer[0] = 0xfe;
	ret = write(fd, buffer, 1);
}

void closeComm ()
{
	close(fd);
}
