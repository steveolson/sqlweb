#
# LIST Library
#
#
LIB_INSTALL_DIR	=../lib
INC_INSTALL_DIR	=../include

#
#
CC	=gcc
CFLAGS	=-g 
LISTINC	=list.h
LISTSRC	=list.c		\
	 l_bind.c	\
	 l_ffirst.c	\
	 l_flast.c	\
	 l_fnext.c	\
	 l_fprev.c	\
	 l_func.c	\
	 l_size.c
LISTMEM	=$(LIB)(list.o)		\
	 $(LIB)(l_bind.o)	\
	 $(LIB)(l_ffirst.o)	\
	 $(LIB)(l_flast.o)	\
	 $(LIB)(l_fnext.o)	\
	 $(LIB)(l_fprev.o)	\
	 $(LIB)(l_func.o)	\
	 $(LIB)(l_size.o)
INC	=$(LISTINC)
SRC	=$(LISTSRC)	\
	 $(INC)		\
	 makefile
LIB	= liblist.a
LIBMEM	=$(LISTMEM)

default:	$(LIB)
$(LIB):		$(LIBMEM)
	-ranlib $(LIB)
$(LISTMEM):	$(LISTSRC) $(LISTINC)

install:	$(LIB)
	@if [ ! -d $(LIB_INSTALL_DIR) ];then mkdir $(LIB_INSTALL_DIR); fi
	@if [ ! -d $(INC_INSTALL_DIR) ];then mkdir $(INC_INSTALL_DIR); fi
	cp $(LIB) $(LIB_INSTALL_DIR)
	cp $(INC) $(INC_INSTALL_DIR)
clean:
	-rm -f *.o $(LIB)

print:	$(SRC)
	@for f in $(SRC);\
	    do echo "$$f\t`pr -l60 -f $$f|grep -c \`echo '\f'\`` page(s)";\
	done
