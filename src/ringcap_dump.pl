#!/usr/bin/perl

#
# Script for dumping packets captured with ringcapd
# Author Claes M Nyberg <pocpon@fuzzpoint.com>
# $Id: ringcap_dump.pl,v 1.4 2005-06-07 19:17:31 cmn Exp $
#

use Config;

$pidfile = '/var/run/ringcapd.pid';
$logfile = '/var/log/ringcapd.log';
$target_pid = 0;


sub basename($)
{
	my @arr;
	@arr = split(/\//, $_[0]);
	return($arr[$#arr]);
}

# Print usage
sub usage()
{
	print "\n-=[ Dump packets captured with ringcapd ]=-\n";
	print "Usage: ", basename($0), "[rincapd-pid]\n";
	print "\n";
}

# Read PID from file
sub read_pid($)
{
	my $file = $_[0];
	my $pid;

	open(PIDF, "<$file") or
		die("Failed to open PID file '$file': $!\n");
	$pid = <PIDF>;
	close(PIDF);
	return(int($pid));
}

# Read last dump and status entry for PID
sub read_loginfo($$)
{
	my $pid = $_[0];
	my $logfile = $_[1];
	my @lines;

	open(LOG, "<$logfile") or
		die("Failed to open $logfile: $!\n");
	@lines = <LOG>;
	close(LOG) or
		warn("Failed to close logfile: $!\n");

	foreach my $line (reverse(@lines))	{
		my @arr = split(/\] /, $line);
		
		if ($line =~ / \[$pid\] Status/) {
			print "[$pid] @arr[1]";
			return;
		}

		if ($line =~ / \[$pid\] Dumped /) {
			print "[$pid] @arr[1]";
		}
		
		if ($line =~ / \[$pid\] Request to dump empty buffer/) {
			print "[$pid] @arr[1]";
			return;
		}
	}
}

# Set up signals
defined $Config{sig_name} or die("No signals !?");
foreach $name (split(' ', $Config{sig_name})) {
	$signo{$name} = $i;
	$signame[$i] = $name;
	$i++;
}

if ($ARGV[0]) 
	{ $target_pid = $ARGV[0]; }
else
	{ $target_pid = read_pid($pidfile); }

# Make sure process exists
kill(0, $target_pid) or
	die("** No process with PID $target_pid\n");

# Send dump signal
kill($signo{USR1}, $target_pid) or
	die("** Could not send signal to target process: $!\n");

# Read status and dump information from log file
sleep(1);
read_loginfo($target_pid, $logfile);
exit(0);

