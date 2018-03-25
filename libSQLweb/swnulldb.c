/* swnulldb.c - No-DB SQLweb interface
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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
/* #include <malloc.h>
 */
#include "sqlweb.h"

char  *gpIfExpr = ""
    ,*gpNullSelect = ""
    ;

/*
/* Connect to the NODB DATABASE.
/* The pConnect parameter should be passed as (char *)0 to instruct
/*     the DBI to use the ORACLE_CONNECT Symobl.  The pConnect is
/*	only used to Override the ORACLE_CONNECT symbol.
/* usage:
/*	SQLWEB_LDA lLda;
/*	RETeFalse( DbConnect((char*)0     ,&lLda), "Connect Failed");
/*	RETeFalse( DbConnect("scott/tiger",&lLda), "Connect Failed");
 */
eBoolean_t
DbConnect(char *pConnect	/* Database Specific Connect String */
	 ,SQLWEB_LDA *pLDA	/* This Function must Fills-in
				/* the SQLWEB_LDA structure
				 */
	 )
{
    return(eTrue);
}

/*
/* Disconnect from the DATABASE.
/* bCommit Flag:  bCommit=eTrue  means essentially "COMMIT RELEASE"
/*		  bCommit=eFalse means essentially "ROLLBACK RELEASE"
/* usage:
/*	SQLWEB_LDA lLda;
/*	... connect ...
/*	RETeFalse( DbDisonnect(&lLda,eTrue), "Connect Failed"); -- Commit
/*	RETeFalse( DbDisonnect(&lLda,eFalse),"Connect Failed"); -- Rollback
 */
eBoolean_t
DbDisconnect(SQLWEB_LDA *pLDA	/* LDA struct from DbConnect() */
	    ,eBoolean_t bCommit	/* Commit/Rollback Flag */
	    )
{
    /* Perform the COMMIT/ROLLBACK
     */
    if(ISeTrue(bCommit)) (void)DbCommit(pLDA);
    else		 (void)DbRollback(pLDA);
    return(eTrue);
}

/*
/* Commit an NODB Transaction
 */
eBoolean_t
DbCommit(SQLWEB_LDA *pLDA)
{
    return(eTrue);
}

/*
/* Rollback a Transaction.
 */
eBoolean_t
DbRollback(SQLWEB_LDA *pLDA)
{
    return(eTrue);
}

/*
/* Open a CURSOR. Must deal with SELECT and non-SELECT statements.
/* All BIND variables are taken from the SYMBOL TABLE
/* All SELECT-LIST variables are installed into the SYMBOL TABLE
/* usage:
/*	SQLWEB_LDA lLda;
/*	SQLWEB_CURSOR cCur;
/*	char *pSQL = "select ename from emp where ename like :SYM_NAME";
/*	... connect ...
/*	RETeFalse( DbOpenCursor(&lLda,pSQL,&cCur), "Open Failed");
 */
eBoolean_t
DbOpenCursor(SQLWEB_LDA *pLDA	/* SQLWEB LDA struct created by DbConnect() */
	    ,char *pStmt	/* SQL Statement, All BIND Variables will
				/* be extracted from the SYMBOL TABLE and
				/* The SELECT LIST must be INSTALLED into
				/* The SYMBOL TABLE
				 */
	    ,SQLWEB_CURSOR *pCursor
				/* This Function must Populate
				/* The SQLWEB_CURSOR structure
				 */
	    )
{
    pCursor->bOpen = eFalse;
    return(eTrue);
}

/*
/* Fetch a ROW from the CURSOR and install each SELECT ITEM into
/* The SYMBOL TABLE.
/* bClose Flag:   bClose=eTrue  means CLOSE CURSOR after the FETCH
/*		  bClose=eFalse means CLOSE CURSOR when exhausted
/*
/* ******* SPECIAL NOTE ABOUT RETURN CODE: *******
/* ONLY return(eFalse) on DATABASE ERROR
/* USE pCursor->bFound=eFalse; to indicate NOT FOUND
/* ******* SPECIAL NOTE ABOUT RETURN CODE: *******
/* usage:
/*	SQLWEB_CURSOR cCur;
/*	... connect ...
/*	... open    ...
/*	for(;;) {
/*	    RETeFalse( DbFetchCursor(&cCur,eFalse), "Close Failed");
/*	    if(cCur.bFound) break;
/*	}
 */
eBoolean_t
DbFetchCursor(SQLWEB_CURSOR *pCursor	/* the CURSOR (from Open) */
	     ,eBoolean_t bClose		/* Force Close after Fetch */
	     )
{
    pCursor->bOpen = eFalse;
    pCursor->bFound = eFalse;
    return(eTrue);
}
