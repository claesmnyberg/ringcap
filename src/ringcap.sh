#!/bin/sh

#
# ringcapd init script
#
# Author: Claes M Nyberg <pocpon@fuzzpoint.com>
# $Id: ringcap.sh,v 1.4 2005-07-17 10:05:00 cmn Exp $
#

# Config
PIDFILE="/var/run/ringcapd.pid"
LOGFILE="/var/log/ringcapd.log"
TARGET="/sbin/ringcapd"
VERBOSE="-v"
DUMPDIR="/tmp/"
BUFSIZE="50M"
INTERFACE="xl1"
FILTER_STRING=""


if [ "$1" = "" ]; then
	echo "Usage `basename $0` <start | stop  | restart>"
	exit 1
fi


function start_ringcap
{
	if [ -f $PIDFILE ]; then
		echo "** ringcap seems to be running ($PIDFILE exists)"
		exit 1
	fi
	
	echo "Starting $TARGET"
	$TARGET $DUMPDIR $VERBOSE -m $BUFSIZE \
		-i $INTERFACE -f $LOGFILE -p $PIDFILE $FILTER_STRING

	if [ $? -ne 0 ]; then
		echo "** Failed to start daemon, look in $LOGFILE"
		exit 1
	fi
}

function stop_ringcap
{
	if [ ! -f $PIDFILE ]; then
		echo "** Can't find PID file $PIDFILE"
		return
	fi
	PID=`cat $PIDFILE`
	echo "Shutting down ringcapd [$PID]"
	kill $PID
}

case x"$1" in
	xstart)
		start_ringcap
		;;
	
	xstop)
		stop_ringcap
		;;

	xrestart)
		stop_ringcap
		sleep 1
		start_ringcap
		;;
esac
