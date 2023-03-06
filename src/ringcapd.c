/*
 * ringcapd.c - Sniff a ringbuffer of packets
 *
 *  Copyright (c) 2005 Claes M. Nyberg <pocpon@fuzzpoint.com>
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
 * $Id: ringcapd.c,v 1.9 2007-01-05 21:31:20 cmn Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include "ringcapd.h"
#include "capture.h"


/* Global options */
struct options opt;

/* Local variables */
static struct ringbuf *rbuf;
static struct capture *cap;


/* Local routines */
static int isdir(const char *);
static void usage(const char *);
static void logpid(const char *);
static void dumppackets(int);
static void write_status(int);
static void unlink_pidfile(void);

/*
 * Capture packets and add them to buffer
 * Flush buffer to dumpdir when we receive a SIGUSR1
 */
static void
capture_pkts(u_char *arg, 
	const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	char buf[8192];

	if (pkthdr->len + sizeof(struct pcap_pkthdr) > sizeof(buf)) {
		warn("Refusing to copy packet, buffer to small!!\n");
		return;
	}
	
	memcpy(buf, pkthdr, sizeof(struct pcap_pkthdr));
	memcpy(&buf[sizeof(struct pcap_pkthdr)], packet, pkthdr->len);
	ringbuf_add(rbuf, buf, pkthdr->len + sizeof(struct pcap_pkthdr));
}


/*
 * Write status when verbose
 */
static void
sigalrm_handler(int signo)
{
	raise(SIGUSR2);
	alarm(STAT_SEC_INTERVAL);

	/* Set signal handler for status dump */
	signal(SIGUSR2, write_status);
	
}

/*
 * Write status when SIGUSR2 is received
 */
static void
write_status(int signo)
{
	struct pcap_pkthdr *first, *last;
	char buf[8192];
	
	buf[0] = '\0';
	first = (struct pcap_pkthdr *)ringbuf_peek_first(rbuf);
	last = (struct pcap_pkthdr *)ringbuf_peek_last(rbuf);
	
	if ((first && last) && (first != last)) {
		snprintf(buf, sizeof(buf), 
			"backlog_time=%s backlog_packets=%u backlog_size=%s", 
			str_hms(last->ts.tv_sec - first->ts.tv_sec), 
			ringbuf_elements(rbuf), str_hsize(ringbuf_currsize(rbuf)));
		verbose(0, "Status: %s\n", buf);
	}
	else
		verbose(0, "Status: Not enough data in buffer\n");	
}


/*
 * Dump packets into file when SIGUSR1 is received 
 */
static void
dumppackets(int signo)
{
	char path[2048];	
	char path2[2048];
	char first_pkt_time[128];
	char last_pkt_time[128];
	size_t packet_count;
	size_t buffer_size;
	pcap_dumper_t *pcd;
	struct pcap_pkthdr *pkthdr;

	first_pkt_time[0] = '\0';

	verbose(1, "Caught signal %u (SIGUSR1) - Request to dump buffer\n", signo);
	/* No packets to dump */
	if ( (packet_count = ringbuf_elements(rbuf)) == 0) {
		verbose(0, "Request to dump empty buffer, ignoring\n");
		signal(SIGUSR1, dumppackets);
		return;
	}
	buffer_size = ringbuf_currsize(rbuf);
	write_status(0);
	
#define DUMPDATE	"%Y%m%d_%H:%M:%S"
	/* Ignore signal while dumping */
	signal(SIGUSR1, SIG_IGN);

	/* Temporary file */
	snprintf(path, sizeof(path), "%s/%s_%s.%d", opt.dumpdir, 
		cap->c_dev == NULL ? "any" : cap->c_dev,
		str_time(time(NULL), (char *)NULL), getpid());


	/* Open file */
	if ( (pcd = pcap_dump_open(cap->c_pcapd, path)) == NULL) {
		err("Failed to open dump file: %s\n", pcap_geterr(cap->c_pcapd));
		signal(SIGUSR1, dumppackets);
		return;
	}

	
	/* Fetch the logged packets from the buffer and write them to the pcap file */
	while ( (pkthdr = (struct pcap_pkthdr *)ringbuf_first(rbuf, NULL)) != NULL) {
		
		/* Save timestamp of first packet in buffer */	
		if (first_pkt_time[0] == '\0') {
			snprintf(first_pkt_time, sizeof(first_pkt_time), 
				"%s", str_time(pkthdr->ts.tv_sec, DUMPDATE));
		}
			
		pcap_dump((u_char *)pcd, pkthdr, (u_char *)((char *)pkthdr + 
			sizeof(struct pcap_pkthdr)) );
#ifdef HAVE_PCAP_DUMP_FLUSH
		pcap_dump_flush(pcd);
#endif
	
		/* Save timestamp of last packet in buffer */
		if (ringbuf_elements(rbuf) == 0) {
			snprintf(last_pkt_time, sizeof(last_pkt_time), "%s", 
				str_time(pkthdr->ts.tv_sec, DUMPDATE));
		}

		free(pkthdr);
	}
	pcap_dump_close(pcd);

	/* Real file name, start and end time */
	snprintf(path2, sizeof(path2), "%s/%s_%s-%s.pcap", opt.dumpdir,
		cap->c_dev == NULL ? "any" : cap->c_dev,
		first_pkt_time, last_pkt_time);
	
	if (rename(path, path2) < 0) {
		err_errno("Failed to rename '%s' to '%s'\n", path, path2);
		signal(SIGUSR1, dumppackets);
		return;
	}
#undef DUMPDATE

	{
		char *str;

		if ( (str = strchr(first_pkt_time, '_')) != NULL)
			*str = ' ';
		if ( (str = strchr(last_pkt_time, '_')) != NULL)
			*str = ' ';
		verbose(0, "Dumped %s bytes with %u packets from %s to %s\n",
			str_hsize(buffer_size), packet_count, first_pkt_time, last_pkt_time);

	}
	signal(SIGUSR1, dumppackets);
}


/*
 * Exit handler, remove PID file.
 */
static void
exit_handler(int signo)
{
	char *sig = "Unknown";

	if (signo == SIGSEGV) 
		sig = "SIGSEGV";
	else if (signo == SIGTERM)
		sig = "SIGTERM";
	else if (signo == SIGBUS)
		sig = "SIGBUS";
	else if (signo == SIGILL)
		sig = "SIGILL";
	else if (signo == SIGPIPE)
		sig = "SIGPIPE";
	
	verbose(0, "Capture ended (received signal %u [%s])\n", 
		signo, sig);
	unlink_pidfile();
	exit(EXIT_SUCCESS);
}

/*
 * Unlink PID file
 */
static void
unlink_pidfile(void)
{
	if (unlink(opt.pidfile) < 0)
		err_errno("Failed to unlink PID file '%s'", opt.pidfile);
}


/*
 * Returns 1 if str is a directory, 0 otherwise
 * and -1 on error.
 */
static int
isdir(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) < 0) {
		err_errno("Failed to stat '%s'", path);
		return(-1);
	}

	if (!S_ISDIR(sb.st_mode)) {
		err("%s: Not a directory\n", path);
		return(0);
	}

	return(1);
}

/*
 * Log PID  to file
 */
static void
logpid(const char *file)
{
	int fd;
	char buf[34];

	snprintf(buf, sizeof(buf), "%d\n", getpid());

	if ( (fd = open(file, O_RDWR |  O_TRUNC | O_CREAT, 0644)) < 0) {
		err_errno("Failed to open PID file '%s'\n", file);
		return;
	}

	if (write(fd, buf, strlen(buf)) != strlen(buf))
		err_errno("Failed to write PID to '%s'", file);
	
	close(fd);
	return;
	
}


static void
usage(const char *pname)
{
	printf("\n-=[ Capture packets into a fixed size ring buffer ]=-\n");
	printf("Usage: %s <dumpdir> [Option(s)] [expression]\n", pname);
	printf("Buffer will be written to <dumpdir> when SIGUSR1 is received\n");
	printf("Options:\n");
	printf("  -d         - Debug, do not become daemon\n");
	printf("  -f logfile - Logfile, default is %s\n", LOGFILE);
	printf("  -i iface   - Listen for packets on interface iface\n");
	printf("  -m max     - Maximum size of packet buffer, default is %s bytes\n", 
		str_hsize(DEFAULT_MAX_SIZE_BYTES));
	printf("  -p pidfile - PID file, default is %s\n", PIDFILE);
	printf("  -P         - Do not listen in promiscuous mode\n");
	printf("  -v         - Be verbose, repeat to increase\n");
	printf("\n");
	exit(EXIT_FAILURE);
}



int
main(int argc, char *argv[])
{
	int i;

	memset(&opt, 0x00, sizeof(opt));
	opt.ringbuf_max = DEFAULT_MAX_SIZE_BYTES;
	opt.argv0 = argv[0];
	opt.iface = NULL;
	opt.dumpdir = NULL;
	opt.promisc = 1;
	opt.pidfile = PIDFILE;
	opt.logfile = LOGFILE;
	opt.filter = NULL;
	opt.debug = 0;

	if ((argv[1] == NULL) || (argv[1][0] == '-'))
		usage(opt.argv0);

	opt.dumpdir = argv[1];
	argc--;
	argv++;

	if (!isdir(opt.dumpdir))
		exit(EXIT_FAILURE);

	while ( (i = getopt(argc, argv, "dvp:m:i:Pf:")) != -1) {
		switch(i) {
			case 'v': opt.verbose++; break;
			case 'P': opt.promisc = 0; break;
			case 'f': opt.logfile = optarg; break;
			case 'm':
				if ( (opt.ringbuf_max = str_to_size(optarg)) == 0)
					errx("Failed to convert max buffer size\n");
				break;
			case 'p': opt.pidfile = optarg; break;
			case 'd': opt.debug = 1; break;
			case 'i': opt.iface = optarg; break;
			default: usage(opt.argv0);
		}
	}

	/* Become daemon and reopen logfile as standard out */
	if (opt.debug == 0) {
        int fd;

		if (daemonize() < 0)
			exit(EXIT_FAILURE);
	
		/* printf("[%d]\n", getpid()); */
        if ( (fd = open(opt.logfile, O_RDWR|O_CREAT|O_APPEND, 0600)) < 0)
            err_errnox("open(%s)\n", opt.logfile);

        fflush(stdout);
        fflush(stderr);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

		/* Log PID */
		logpid(opt.pidfile);
		atexit(unlink_pidfile);
    }

    /* Close unused files */
    close(STDIN_FILENO);
    for (i=STDERR_FILENO+1; i<1024; i++)
        close(i);
		
	verbose(0, "+-+-+-+-+-+ Capture Started +-+-+-+-+-+\n");
	/* Open device */
	if ( (cap = cap_open(opt.iface, opt.promisc)) == NULL) 
		errx("Failed to open device.\n");

	/* Build and set filter */
	if (argv[optind] != NULL) {
		opt.filter = str_join(" ", &argv[optind]);
		if (cap_setfilter(cap, opt.filter) < 0)
			exit(EXIT_FAILURE);
	}
	
	verbose(0, "Dump directory: %s\n", opt.dumpdir);
	if (!opt.debug) {
		verbose(0, "Log file: %s\n", opt.logfile);
		verbose(0, "PID file: %s\n", opt.pidfile);
	}
	verbose(0, "Buffer size: %s bytes\n", str_hsize(opt.ringbuf_max));
	if (opt.filter)
		verbose(0, "Filter: %s\n", opt.filter);
	else
		verbose(0, "No capture filter\n");

	if (opt.verbose)
		verbose(1, "Status log interval %s [%u seconds]\n", 
			str_hms(STAT_SEC_INTERVAL), STAT_SEC_INTERVAL);

	/* Init ring buffer */
	if ( (rbuf = ringbuf_init(opt.ringbuf_max)) == NULL)
		exit(EXIT_FAILURE);
	
	/* Set signal handler for dumping of packets */
	signal(SIGUSR1, dumppackets);

	/* Set signal handler for status dump */
	signal(SIGUSR2, write_status);
	
	if (!opt.debug) {
		/* Set exit handler */
		signal(SIGTERM, exit_handler);
		signal(SIGSEGV, exit_handler);
		signal(SIGBUS, exit_handler);
		signal(SIGILL, exit_handler);
		signal(SIGPIPE, exit_handler);

		/* Signals we ignore */
		signal(SIGQUIT, SIG_IGN);
		signal(SIGABRT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
	}
	
	/* Dump statistics in verbose mode */
	if (opt.verbose) 
		signal(SIGALRM, sigalrm_handler);
		
	/* Start capturing packets, restart if the interface goes down */
	for (;;) {
		size_t retry_time;

		/* Align alarm call */
		if (opt.verbose) {
			char buf[256];
			
			snprintf(buf, sizeof(buf), "%s", str_time(time(NULL) + 
				(STAT_SEC_INTERVAL - (time(NULL) % STAT_SEC_INTERVAL)), NULL));
			
			verbose(1, "First status output aligned to %s\n", buf);
			alarm(STAT_SEC_INTERVAL - (time(NULL) % STAT_SEC_INTERVAL));
		}
		pcap_loop(cap->c_pcapd, -1, capture_pkts, (u_char *)cap);
		retry_time = 10;

		for (;;) {

			/* Abort alarm */
			if (opt.verbose)
				alarm(0);

			warn("pcap_loop: '%s', retrying in %u seconds\n", 
				pcap_geterr(cap->c_pcapd), retry_time);
			
			if (cap != NULL) {
				cap_close(cap);
				cap = NULL;
			}
			sleep(retry_time);
	
			if ( (cap = cap_open(opt.iface, opt.promisc)) != NULL) 
				break;
			retry_time += 10;

			/* Wait a maximum of five minutes */
			if (retry_time > 300)
				retry_time = 300;
		}
	}
	exit(EXIT_FAILURE);
}
