/********************************************************************
 * bl - adjust laptop screen backlight                              *
 *                                                                  *
 * usage:                                                           *
 *             bl [ +val[%] | -val[%] | val[%] ]                    *
 *                                                                  *
 * if no argument given, then current brightness level is printed   *
 * +val will increment brightness by that amount                    *
 * -val will decrement brightness by that amount                    *
 * val will set brightness to that amount                           *
 * a percentage of maximum brightness can also be used by using '%' *
 * brightness is enforced within the range 0..max_brightness        *
 *                                                                  *
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

char *sys = "/sys/class/backlight/intel_backlight/";
char *cmd; // av[0]

int readval(char *fname)
{
	int i, fd;
	char buf[10], path[60];

	strcpy(path, sys);
	strcat(path, fname);
	if ((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: can't open %s (%s)\n", cmd, path, strerror(errno));
		exit(1);
	}
	if ((i = read(fd, buf, sizeof(buf)-1)) < 1) {
		fprintf(stderr, "%s: can't read %s (%s)\n", cmd, path, strerror(errno));
		exit(1);
	}
	buf[i] = '\0';
	i = atoi(buf);
	close(fd);
	return i;
}

int badarg(char *s)
{
	// allow only "[+|-]digits[%]"
	if (*s == '+' || *s == '-') ++s;
	do {
		if (!isdigit(*s)) 
			return 1;
		if (*++s == '%') 
			return *++s == '\0'? 0 : 1;
	} while (*s);
	return 0;
}

void main(int ac, char *av[])
{
	int fd, v, inc = 0, actual, max, newval;
	char buf[10], path[60];

	cmd = av[0];
	actual = readval("actual_brightness");
	max = readval("max_brightness");

	if (ac == 1) {
		printf("%d %.0f%%\n", actual, (float) actual / (float) max * 100.0);
		exit(0);
	} else if (ac > 2 || badarg(av[1])) {
		fprintf(stderr, "%s: adjust backlight on laptop screen\n\
usage [ +val[%%] | -val[%%] | val[%%] ]\n", cmd);
		exit(1);
	}

	if (*av[1] == '+' || *av[1] == '-') inc = 1;
	v = atoi(av[1]);

	if (strchr(av[1], '%')) {
		if (inc == 0)
			newval = (int) ((float)v / 100.0 * (float)max);
		else
			newval = actual + (int) ((float)v / 100.0 * (float)max);
	} else 
		newval = inc? actual + v : v;
	

	if (newval < 0)
		newval = 0;
	else if (newval > max)
		newval = max;

	// now set brightness with newval
	
	sprintf(buf, "%d", newval);
	strcpy(path, sys);
	strcat(path, "brightness");
	if ((fd = open(path, O_WRONLY)) < 0) {
		fprintf(stderr, "%s: can't open %s (%s)\n", cmd, path, strerror(errno));
		exit(1);
	}
#define SLOW
#ifdef SLOW // maybe a perceptible ramp to new value
	int change = newval - actual;
	inc = change < 0? -1 : 1;
	for (int i = 0; i < abs(change); ++i) {
		actual += inc;
		sprintf(buf, "%d", actual);
		write(fd, buf, strlen(buf));
		usleep(100);
	}
#else // write new value at once
	if (write(fd, buf, strlen(buf)) != strlen(buf)) {
		fprintf(stderr, "%s: can't set %s with %s (%s)\n", cmd, path, buf, strerror(errno));
		exit(1);
	}
#endif
	close(fd);
	exit(0);
}
