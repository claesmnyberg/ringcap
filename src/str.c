/*
 * str.c - Commonly used string routines
 *
 *  Copyright (c) 2004 Claes M. Nyberg <pocpon@fuzzpoint.com>
 *  All rights reserved, all wrongs reversed.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: str.c,v 1.2 2005-05-26 04:38:36 cmn Exp $
 */

#ifdef WINDOWS
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/time.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h> 
#include <sys/types.h> 


/*
 * Join strings in strv together with str as separator.
 * Returns a string which has to be free'd using free
 * on success, NULL otherwise.
 */
char *
str_join(const char *str, char * const *strv)
{
    char *js = NULL;
    size_t n;
    size_t i;
    size_t j;

    if ((str == NULL) || (strv == NULL) || (strv[0] == NULL))
        return(NULL);

    j = strlen(str);

    /* Count bytes needed */
    for (n=0, i=0; strv[i] != NULL; i++)
        n += strlen(strv[i]) + j;

    if (n == 0)
        return(NULL);

    /* Just pad with a few bytes */
    n += 24;

	if ( (js = calloc(n, 1)) == NULL) {
		perror("calloc");
		return(NULL);
	}

    /* Join strings together */
    for (j=0, i=0; strv[i] != NULL; i++) {

        /* Last string, don't append separator */
        if (strv[i+1] == NULL)
            j += snprintf(&js[j], n-j, "%s", strv[i]);
        else
            j += snprintf(&js[j], n-j, "%s%s", strv[i], str);
    }

    return(js);
}

/*
 * Split string into tokens using space as deliminator.
 * Pointers to the strings are stored at strv, where max 
 * indicates the maximum number of pointers to store, including
 * the terminating NULL pointer.
 * Returns the number of valid string pointers set in strv.
 * Note that deliminating characters found in str is
 * replaced with the terminating '\0' character.
 */
unsigned int
str_to_argv(char *str, char **strv, unsigned int maxv)
{
    char *pt = str;
    unsigned int i;

	if ((str == NULL) || (strv == NULL) || (maxv == 0))
		return(0);

	i = 0;
    while (*pt && (i < (maxv - 1)) ) {

        strv[i++] = pt;

        for (; !isspace((int)*pt) && *pt; pt++);

        if (*pt == '\0')
            break;

        *pt = '\0';
        pt++;

        for (; isspace((int)*pt); pt++);
    }

    strv[i] = (char *)NULL;
    return(i);

}


/*
 * Check if string is a numeric value.
 * Zero is returned if string contains base-illegal
 * characters, 1 otherwise.
 * Binary value should have the prefix '0b'
 * Octal value should have the prefix '0'.
 * Hexadecimal value should have the prefix '0x'.
 * A string with any digit as prefix except '0' is
 * interpreted as decimal. 
 * If val in not NULL, the converted value is stored
 * at that address.
 */
int 
str_isnum(const char *str, unsigned long *val)
{
    int base = 0;
    char *endpt;
 
    if (str == NULL)
        return(0);
 
    while (isspace((int)*str))
        str++;

    /* Binary value */
    if (!strncmp(str, "0b", 2) || !strncmp(str, "0B", 2)) {
        str += 2;
        base = 2;
    }

    if (*str == '\0')
        return(0);

    if (val == NULL)
        strtoul(str, &endpt, base);
    else
        *val = strtoul(str, &endpt, base);

    return((int)*endpt == '\0');
}


/*
 * Returns 1 if str is a dotted decimal IPv4 address
 * 0 otherwise.
 */
int
str_isipv4(const char *str)
{
	int ip[4];
	unsigned char c;
	#define ISu8(x) (((x) >= 0) && ((x) <= 0xff))
	
	if (sscanf(str, "%d.%d.%d.%d%c", &ip[0], &ip[1], &ip[2], &ip[3], &c) != 4)
		return(0);

	return(ISu8(ip[0]) && ISu8(ip[1]) && ISu8(ip[2]) && ISu8(ip[3]));
	#undef ISu8
}


/*
 * Conver time given in seconds to a string.
 * If fmt is NULL, time is given as 'year-month-day hour:min:sec'
 * Returns a pointer to the time string on succes, NULL on error
 * with errno set to indicate the error.
 */
const char *
str_time(time_t caltime, const char *fmt)
{
	static char tstr[256];
	struct tm *tm;

	if (fmt == NULL)
		fmt = "%Y-%m-%d %H:%M:%S";
	
	memset(tstr, 0x00, sizeof(tstr));

	if ( (tm = localtime(&caltime)) == NULL)
		return(NULL);

	if (strftime(tstr, sizeof(tstr) -1, fmt, tm) == 0)
		return(NULL);

	return(tstr);
}

/*
 * Generate len bytes of random bytes and store it in buf.
 * Code influed by lib/libkern/random.c (NetBSD 1.6.1).
 */
void
str_rand(unsigned char *buf, size_t len)
{
    static unsigned long randseed = 1;
    struct timeval tv;
    long x;
    size_t i;
    long hi;
    long lo;
    long t;

    /* First time throught, create seed */
    if (randseed == 1) {
#ifdef WINDOWS
        tv.tv_sec = time(NULL);
        tv.tv_usec = GetTickCount() ^ getpid();
#else
		int fd;
		if ( (fd = open("/dev/random", O_RDONLY)) > 0) {
			read(fd, &randseed, sizeof(randseed));
			close(fd);
		}
        tv.tv_sec = time(NULL); /* In case gettimeofday fails */
        gettimeofday(&tv, NULL);
        tv.tv_usec ^= getpid();
#endif
        randseed = tv.tv_sec ^ tv.tv_usec;
    }

    for (i=0; i<len; i++) {

        /*
         * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
         * From "Random number generators: good ones are hard to find",
         * Park and Miller, Communications of the ACM, vol. 31, no. 10,
         * October 1988, p. 1195.
         */
        x = randseed;
        hi = x / 127773;
        lo = x % 127773;
        t = 16807 * lo - 2836 * hi;
        if (t <= 0)
            t += 0x7fffffff;
        randseed = t;

        buf[i] = (unsigned char)t;
    }
}

/*
 * Convert string-size to value.
 * Nothing/b   -> Bytes
 * K/k/KB/Kb/kB/kb -> Kilo Bytes
 * M/m/MB/Mb/mB/mb -> Mega Bytes
 * G/g/GB/Gb/gB/gb -> Giga Bytes
 * Returns the size on success, 0 on error.
 */
size_t
str_to_size(const char *str)
{
    double size;
    unsigned long base;
    char *ep;

    size = strtod(str, &ep);
    base = 0;

    if (*ep == '\0')
        base = 1;
    else if (!strcasecmp(ep, "b"))
        base = 1;
    else if (!strcasecmp(ep, "KB") || !strcasecmp(ep, "K"))
        base = 1024;
    else if (!strcasecmp(ep, "MB") || !strcasecmp(ep, "M"))
        base = 1024*1024;
    else if (!strcasecmp(ep, "GB") || !strcasecmp(ep, "G"))
        base = 1024*1024*1024;
    else
        return(0);

    if ((size*base) < size) {
        fprintf(stderr, "** Error: Integer "
            "overflow in string to size conversion\n");
        return(0);
    }

    return(size*base);
}

/*
 * Convert size to human readable string.
 */
const char *
str_hsize(size_t size)
{
    static char hstr[24];
    double num;
    char *pad = "";

    if (size / (1024*1024*1024) > 0) {
        num = size / (1024*1024*1024);
        pad = "G";
    }
    else if (size / (1024*1024) > 0) {
        num = size / (1024*1024);
        pad = "M";
    }
    else if (size / 1024 > 0) {
        num = size / (1024);
        pad = "K";
    }
    else
        num = size;

    if (num == size)
        snprintf(hstr, sizeof(hstr), "%u", size);
    else
        snprintf(hstr, sizeof(hstr), "%.1f%s", num, pad);
    return(hstr);
}


/*
 * Convert seconds into hh:mm:ss string
 */
const char *
str_hms(time_t sec)
{
	static char hms[48];
	unsigned int h, m;

	h = sec / 3600;
	sec -= (h*3600);
	m = sec / 60;
	sec -= m * 60;
	snprintf(hms, sizeof(hms), "%02u:%02u:%02u", h, m, (unsigned int)sec);
	return(hms);
}
