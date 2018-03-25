#
# Informix Makefile
#
INFORMIXDIR=/var/informix/online
LIBDIR	  = ../../lib
CFLAGS    = -DNO_WIN32 -g
#SYSLIBES  = $(LIBDIR)/swmain.o -lifcli -laio -lm -lnsl -lsocket 
SYSLIBES  = -B static $(LIBDIR)/swmain.o -lifcli \
	    -B dynamic -laio -lm -lnsl -lsocket -ldl \
	    -B static -lSQLweb -llist
OBJS	  = ifmxdb.o
ODBCLIB   = -L$(INFORMIXDIR)/lib/cli -L$(LIBDIR)
INCL      = -I$(INFORMIXDIR)/incl/cli -I../../include
CC	  = gcc
sqlweb:	$(OBJS)
	$(CC) $(CFLAGS) -o sqlweb $(OBJS) $(ODBCLIB) $(SYSLIBES)
ifmxdb.o:	ifmxdb.c
	$(CC) $(CFLAGS) $(INCL) -c ifmxdb.c
clean:
	rm -f sqlweb *.o
