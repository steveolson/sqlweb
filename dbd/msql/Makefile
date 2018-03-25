#
# MSql Makefile
#
MSQL	=/homeb/msql-1.0.10
CFLAGS	=-I. -I../../include -I${MSQL}/include
LDFLAGS=-L../../lib -lSQLweb -llist -L${MSQL}/lib -lmsql -lsocket -lmalloc

sqlweb:		msqldb.o 
	$(CC) $(CFLAGS) -o sqlweb ../../lib/swmain.o msqldb.o $(LDFLAGS)
msqldb.o:	msqldb.c
	$(CC) $(CFLAGS) -c msqldb.c
msqldb:		msqldb.c
	$(CC) $(CFLAGS) -DMAIN_TEST -c msqldb.c
	$(CC) $(CFLAGS) -o msqldb msqldb.o $(LDFLAGS)
	rm msqldb.o

clean:
	rm -f msqldb.o msqldb sqlweb
