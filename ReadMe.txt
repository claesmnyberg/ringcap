-=[ Ring Buffer Capture Program 
-=[ Copyright Claes M. Nyberg <cmn@signedness.org>, 2004

This program captures packets into a FIFO queue.
The size of the FIFO is 50MB by default, but can be changed on the
commandline using the -m option.
Once the buffer is full, packets will be removed until there is enough
space for the newly arrived packet.

The ringcap_dump.pl script dumps the buffer into a pcap(3) file into the
directory specified when the daemon was started.
If you plan to use a PID or log file different from the default, you
will have to set the path(s) in ringcap_dump.pl.

-=[ Commandline Options

Usage: ./ringcapd <dumpdir> [Option(s)] [expression]
Buffer will be written to <dumpdir> when SIGUSR1 is received
Options:
  -d         - Debug, do not become daemon
  -f logfile - Logfile, default is /var/log/ringcapd.log
  -i iface   - Listen for packets on interface iface
  -m max     - Maximum size of packet buffer, default is 50.0M bytes
  -p pidfile - PID file, default is /var/run/ringcapd.pid
  -P         - Do not listen in promiscuous mode
  -v         - Be verbose, repeat to increase

