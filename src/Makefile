
#
# Makefile
# 
# Author Claes M Nyberg <pocpon@fuzzpoint.com>
# $Id: Makefile,v 1.5 2005-06-08 18:44:28 cmn Exp $
#

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SHELL        = /bin/sh
CC           = gcc
CFLAGS       = -Wall -O -pedantic -fomit-frame-pointer -s
OBJS         = ringcapd.o print.o str.o capture.o daemon.o ringbuf.o
LIBS         = -lpcap
PROG         = ringcapd

INIT_OBJ     = ringcap.sh
INIT_SCRIPT  = ringcap

DUMP_OBJ     = ringcap_dump.pl
DUMP_SCRIPT  = ringcap_dump

STATUS_OBJ    = ringcap_status.pl
STATUS_SCRIPT = ringcap_status
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#staticl

INSTALL_ROOT = /
INSTALL_INIT = ${INSTALL_ROOT}etc/init.d
INSTALL_SBIN = ${INSTALL_ROOT}sbin
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#staticl


none:
	@echo
	@echo "make <target>"
	@echo "Targets: "
	@echo "  all      - Build all"
	@echo "  static   - Build static binary"
	@echo "  new      - Clean and rebuild all"
	@echo "  clean    - Clean up this directory"
	@echo "  install  - Install tools"
	@echo "  tgz      - Create a tgz archive"
	@echo 

new: clean all

all: ${OBJS} ${INIT_OBJ} ${DUMP_OBJ}
	${CC} ${CFLAGS} -o ${PROG} ${OBJS} ${LIBS}
	cp -f ${INIT_OBJ} ${INIT_SCRIPT}
	chmod 711 ${INIT_SCRIPT}
	cp -f ${DUMP_OBJ} ${DUMP_SCRIPT}
	chmod 711 ${DUMP_SCRIPT}
	cp -f ${STATUS_OBJ} ${STATUS_SCRIPT}
	chmod 711 ${STATUS_SCRIPT}

static:
	@make CFLAGS='-static' all

debug: clean
	@make CFLAGS='-Wall -ggdb' all

clean:
	rm -f ${PROG} ${PROG}.tgz ${STATUS_SCRIPT} ${INIT_SCRIPT} ${DUMP_SCRIPT} ${OBJS} *.core

tgz:
	@mkdir -p .tmp$$/${INSTALL_BIN}
	@mkdir -p .tmp$$/${INSTALL_SBIN}
	@mkdir -p .tmp$$/${INSTALL_INIT}
	@cp -f ${PROG} .tmp$$/${INSTALL_SBIN}/${PROG}
	@chmod 511 .tmp$$/${INSTALL_SBIN}/${PROG}
	@cp -f ${DUMP_SCRIPT} .tmp$$/${INSTALL_SBIN}/${DUMP_SCRIPT}
	@chmod 711 .tmp$$/${INSTALL_SBIN}/${DUMP_SCRIPT}
	@cp -f ${STATUS_SCRIPT} .tmp$$/${INSTALL_SBIN}/${STATUS_SCRIPT}
	@chmod 711 .tmp$$/${INSTALL_SBIN}/${STATUS_SCRIPT}
	@cp -f ${INIT_SCRIPT} .tmp$$/${INSTALL_INIT}/${INIT_SCRIPT}
	@chmod 711 .tmp$$/${INSTALL_INIT}/${INIT_SCRIPT}
	cd .tmp$$; tar -czvpf ../${PROG}.tgz . ; cd ..
	@rm -rf .tmp$$

install: all
	@mkdir -p ${INSTALL_SBIN}
	@chmod 755 ${INSTALL_SBIN}
	cp -f ${PROG} ${INSTALL_SBIN}/${PROG}
	cp -f ${DUMP_SCRIPT} ${INSTALL_SBIN}/${DUMP_SCRIPT}
	cp -f ${STATUS_SCRIPT} ${INSTALL_SBIN}/${STATUS_SCRIPT}
	@mkdir -p ${INSTALL_INIT}
	@chmod 755 ${INSTALL_INIT}
	cp -f ${INIT_SCRIPT} ${INSTALL_INIT}/${INIT_SCRIPT}

uninstall::
	rm -f ${INSTALL_SBIN}/${PROG}
	rm -f ${INSTALL_INIT}/${INIT_SCRIPT}
	rm -f ${INSTALL_SBIN}/${DUMP_SCRIPT}
	rm -f ${INSTALL_SBIN}/${STATUS_SCRIPT}
	@echo "** Remember to remove empty directories"
