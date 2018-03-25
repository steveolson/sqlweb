/*  userexit.h - SQLweb userexits 
/*
/* Copyright (c) 1995-1999 Applied Information Technologies, Inc.
/* All Rights Reserved.
/*  
/* Distributed uder the GNU General Public License which was included in
/* the file named "LICENSE" in the package that you recieved.
/* If not, write to:
/* The Free Software Foundation, Inc.,
/* 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _USEREXIT_H_
#define _USEREXIT_H_

/*
 */
typedef	eBoolean_t (*PTHF)(PI *pPI,eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_if(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_if2(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_while(PI *pPI, eBoolean_t *bExpandChildren);
/* extern  eBoolean_t sqlweb_eval(PI *pPI, eBoolean_t *bExpandChildren);
 */
extern  eBoolean_t sqlweb_connect(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_cursor(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_include(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_host(PI *pPI, eBoolean_t *bExpandChildren);
extern  eBoolean_t sqlweb_demo(PI *pPI, eBoolean_t *bExpandChildren);

typedef struct tag_handlers_s {
    char *pTagName;
    PTHF fHandler;
} TAG_HANDLE;

TAG_HANDLE gaTagHandlers[]= {
    /*  pTagName	,fHandler */
     { "CURSOR"		,sqlweb_cursor}
    ,{ "SQLWEB-CURSOR"	,sqlweb_cursor}
    ,{ "IF"		,sqlweb_if}
    ,{ "SQLWEB-IF"	,sqlweb_if}
    ,{ "IF2"		,sqlweb_if2}
    ,{ "SQLWEB-IF2"	,sqlweb_if2}
    ,{ "WHILE"		,sqlweb_while}
    ,{ "SQLWEB-WHILE"	,sqlweb_while}
    ,{ "SYMBOL"		,sqlweb_if2}
/*    ,{ "SQLWEB-EVAL"	,sqlweb_eval}
/*    ,{ "EVAL"		,sqlweb_eval}
 */
    ,{ "SQLWEB-SYMBOL"	,sqlweb_if2}
    ,{ "INCLUDE"	,sqlweb_include}
    ,{ "SQLWEB-INCLUDE"	,sqlweb_include}
    ,{ "HOST"		,sqlweb_host}
    ,{ "SQLWEB-HOST"	,sqlweb_host}
    ,{ "DEMO"		,sqlweb_demo}
    ,{ "SQLWEB-DEMO"	,sqlweb_demo}
    ,{ "CONNECT"	,sqlweb_connect}
    ,{ "SQLWEB-CONNECT"	,sqlweb_connect}
    ,{ 0		,0}
};

#endif /* _USEREXIT_H_ */
