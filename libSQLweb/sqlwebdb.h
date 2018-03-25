/* sqlwebdb.h - SQLWEB Database APU
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
#ifndef _SQLWEBDB_H_
#define _SQLWEBDB_H_

/*
/* Standard INCLUDES
 */
#include <stdarg.h>
#include <stdio.h>

/*
/* SQLweb INCLUDES
 */
#include "list.h"
#include "boolean.h"

typedef struct _sqlweb_lda_ {
    int		iLdaIndex;
    eBoolean_t	bConnected;
    char	*pConnect;
} SQLWEB_LDA;

typedef struct _sqlweb_cursor {
    int		 iCursorIndex;
    eBoolean_t	 bOpen
		,bFound
		;
    char	 *pStmt;
    SQLWEB_LDA	 *pLDA;
    LIST	 *lSelect
		,*lBind
		;
} SQLWEB_CURSOR;

/*
/* MACRO's
 */
#define MAX_CONNECTIONS 10
#define MAX_CURSORS 255

/*
/* Function PROTOTYPES
 */
extern eBoolean_t DbConnect(char *pConnect, SQLWEB_LDA *pLda);
extern eBoolean_t DbOpenCursor(SQLWEB_LDA    *pLda
			      ,char	     *pStmt
			      ,SQLWEB_CURSOR *pCursor
			      );
extern eBoolean_t DbFetchCursor(SQLWEB_CURSOR *pCursor,eBoolean_t bClose);
extern eBoolean_t DbDisconnect(SQLWEB_LDA *pLda, eBoolean_t bCommit);
extern eBoolean_t DbCommit(SQLWEB_LDA *pLda);
extern eBoolean_t DbRollback(SQLWEB_LDA *pLda);

#endif /* _SQLWEBDB_H_ */
