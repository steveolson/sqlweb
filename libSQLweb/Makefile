#
# %W% Da' Makefile %D% %T%
# SQLweb Makefile
#
#
# SQLweb Library
#
#
LIB_INSTALL_DIR	=../lib
INC_INSTALL_DIR	=../include

#
#
CC	=gcc
#CFLAGS	=-I../include -g -DUSE_LIBMALLOC -Wall
CFLAGS	=-I../include -g 
YFLAGS	=-dv
#LDFLAGS	=libSQLweb.a -L../lib -llist -lsocket -lmalloc
# LDFLAGS	=libSQLweb.a -L../lib -llist -lsocket
LDFLAGS	=libSQLweb.a -L../lib -llist
INC	=boolean.h	\
	 sqlweb.h	\
	 sqlwebdb.h	\
	 userexit.h	\
	 ifwords.h	\
	 scalar.h
CSRC	=ldhtml.c	\
	 sqlweb.c	\
	 swnulldb.c	\
	 swinput.c	\
	 swoutput.c	\
	 swutil.c	\
	 swbltin.c	\
	 ifwords.c	\
	 ctSetup.c	\
	 scalar.c	\
	 ifparse.y
CMAIN	=swmain.c
MAIN	=$(CMAIN:.c=.o)
LIBMEM	=$(LIB)(ldhtml.o)	\
	 $(LIB)(sqlweb.o)	\
	 $(LIB)(swnulldb.o)	\
	 $(LIB)(swinput.o)	\
	 $(LIB)(swoutput.o)	\
	 $(LIB)(swutil.o)	\
	 $(LIB)(swbltin.o)	\
	 $(LIB)(ifwords.o)	\
	 $(LIB)(ctSetup.o)	\
	 $(LIB)(scalar.o)	\
	 $(LIB)(ifparse.o)
SRC	=$(CSRC)	\
	 $(INC)		\
	 $(CMAIN)	\
	 makefile
LIB	=libSQLweb.a

default:		$(LIB) $(MAIN) sqlweb
$(LIB):			$(LIBMEM)
	-ranlib $(LIB)
$(LIB)(ldhtml.o):	ldhtml.c
$(LIB)(sqlweb.o):	sqlweb.c
$(LIB)(swnulldb.o):	swnulldb.c
$(LIB)(swinput.o):	swinput.c
$(LIB)(swoutput.o):	swoutput.c
$(LIB)(swutil.o):	swutil.c
$(LIB)(swbltin.o):	swbltin.c
$(LIB)(ctSetup.o):	ctSetup.c
$(LIB)(scalar.o):	scalar.c ifparse.y
$(LIB)(ifwords.o):	ifwords.c  ifparse.y
$(LIB)(ifparse.o) ifparse.h:	ifparse.y
	$(YACC) $(YFLAGS) ifparse.y
	mv y.tab.c ifparse.c
	mv y.tab.h ifparse.h
	$(CC) $(CFLAGS) -c ifparse.c
	$(AR) $(ARFLAGS) $(LIB) ifparse.o
	rm ifparse.[oc]
	
$(LIBMEM):		$(INC)

sqlweb:	$(LIB) $(MAIN)
	$(CC) $(CFLAGS) -o sqlweb $(MAIN) $(LDFLAGS)

install:		$(LIB) $(MAIN)
	@if [ ! -d $(LIB_INSTALL_DIR) ];then mkdir $(LIB_INSTALL_DIR); fi
	@if [ ! -d $(INC_INSTALL_DIR) ];then mkdir $(INC_INSTALL_DIR); fi
	cp $(LIB) $(LIB_INSTALL_DIR)
	cp $(MAIN) $(LIB_INSTALL_DIR)
	cp $(INC) ifparse.h $(INC_INSTALL_DIR)
clean:
	-rm -f *.o $(LIB) ifparse.[ch] y.tab.[ch] y.output sqlweb


