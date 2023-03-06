#ifndef _RINGCAPD_H
#define _RINGCAPD_H

#include <sys/types.h>
#include "ringbuf.h"
#include "print.h"
#include "str.h"

/* Default size of fixed size buffer in bytes */
#define DEFAULT_MAX_SIZE_BYTES	(50*1024*1024)
#define LOGFILE	"/var/log/ringcapd.log"
#define PIDFILE "/var/run/ringcapd.pid"

/* Interval in seconds between status output in verbose mode */
#define STAT_SEC_INTERVAL	(3600)

struct options {
	unsigned char verbose;	
	
	char *argv0;
	char *iface;
	char *dumpdir;
	char *logfile;
	char *pidfile;
	char *filter;
	
	unsigned int promisc:1;
	unsigned int debug:1;
	size_t ringbuf_max;
};

/* daemonize.c */
extern int daemonize(void);
#endif /* _RINGCAPD_H */
