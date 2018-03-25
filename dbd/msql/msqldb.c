/*  msqldb.c - MSQL API to SQLweb
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
#include <malloc.h>
#include "sqlweb.h"
/* #include "sqlstr.h"
 */

/*
/* mSQL INCLUDES.  
 */
#include "msql.h"	/* Oracle Datatypes */

/*
/* The DBINFO struct.  
 */
typedef struct {
    m_field *pField;	/* Field definition from mSQL */
    int iPosition;	/* Position in Select-List */
    char *pValue;	/* Pointer to Symbol's String */
} DbInfo_t;

/*
/* Internal/Private Functions
 */
static eBoolean_t DescribeBindVars  (SQLWEB_CURSOR *pCursor);
static eBoolean_t DescribeSelectVars(SQLWEB_CURSOR *pCDA);
static eBoolean_t GetCursorIndex(int *piCursorIndex);
static eBoolean_t SetSelectSymbols(DbInfo_t *pDbInfo);
/*
static void       PushDBErr(Lda_Def *pLDA,Cda_Def *pCDA,char *pErrMsg);
 */


#define MSQL_ERROR		-1
#define MAX_ITEM_BUFFER_SIZE    33
#define MAX_SQL_IDENTIFIER      31

/* Global Variables
 */
static m_row gRow;
char *gpNullSelect = ""
    ,*gpIfExpr="";

/*
/* THE LDA Array....
 */
static int giLDA[MAX_CONNECTIONS];

/*
/* THE CURSOR Array
 */
m_result *gaCDA[MAX_CURSORS];

/*
/* MAIN for testing only
 */

#ifdef MAIN_TEST
main(){
    char sBuf[BUFSIZ];
    FRMSym("name","steve");
    FRMSym("nameb","%ey%");
    FRMSym("V2PGN","55");
    (void)x();
    while( MsgPop(sBuf) ) {
	printf("%s\n",sBuf);
    }
}
int x()
{
    SQLWEB_LDA lda;
    SQLWEB_CURSOR cda;
    char *p = "select c2,c1 from jk where c1 like :nameb";
    char *p1 = "SELECT \n\
             pi.pi_level \n\
            ,pi.page_gen_nbr pgn \n\
            ,nvl(pi.pi_seq_nbr,'0') pi_seq_nbr \n\
            ,pi.pi_gen_nbr \n\
            ,pi.tag_name \n\
            ,nvl(pi.page_gen_nbr_link,'') page_gen_nbr_link \n\
            ,pi.pi_contents \n\
    FROM html_V_page_items pi \n\
    WHERE pi.page_gen_nbr = :V2PGN ";
    char *pSql1 = "select to_char(sysdate,'mm/dd/yyyy hh24:mi') da_dat \n\
		,t.*\n\
		from html_pages t\n\
		where :name='steve' and :nameb='so'";
    char *pDaDat, *pPageName, *pPageTitle;
    RETeFalse(DbConnect("test",&lda),"Connect Failed");
    RETeFalse(DbOpenCursor(&lda,p,&cda),"Open Failed");
/*    RETeFalse(DbOpenCursor(&lda,p1,&cda),"Open Failed");
/*    GetSymbolValueREF("PI_CONTENTS",&pDaDat);
/*    GetSymbolValueREF("PI_SEQ_NBR",&pPageName);
/*    GetSymbolValueREF("TAG_NAME",&pPageTitle);
 */
    for(;;){
	RETeFalse(DbFetchCursor(&cda,eFalse),"Fetch Failed");
	if(ISeFalse(cda.bFound))
	    break;
	GetSymbolValueREF("C2",&pDaDat);
	GetSymbolValueREF("C1",&pPageName);
	fprintf(stderr,"C(%s),N(%s)\n",pDaDat,pPageName);
    }
    RETeFalse(DbDisconnect(&lda,eFalse),"Disconnect Failed");

    return(0);
}
#endif /* MAIN_TEST */

/*
/* Connect to the MSQL DATABASE.
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
    static int siConnectIndex;	/* Connections are NOT re-used. */
    char sEnvBuf[MAX_TOKVAL_SIZE]
	,*pBuf
	,*pHost
	;

    if( siConnectIndex > MAX_CONNECTIONS ){
	MsgPush("Exceeded Maximum Connections");
	return(eFalse);
    }

    /*
    /* Prepare to Connect to MSQL
     */

    /*
    /* Set MSQL_HOME, if it there.
     */
    pBuf = getenv("MSQL_HOME");
	/* The Environment overrides the SYMBOL TABLE
	 */
    if(!pBuf) {
	if(ISeTrue(GetSymbolValueREF("MSQL_HOME",&pBuf))) {
	    sprintf(sEnvBuf,"MSQL_HOME=%s", pBuf);
	    putenv(strdup(sEnvBuf));
	}
    }

    /*
    /* Check for MSQL_HOST, this is NOT REQUIRED because the
    /* connect string (remote) databases don't use MSQL_HOST
     */
    pHost = getenv("MSQL_HOST");
	/* Again, Environment overrides the SYMBOL Table
	 */
    if(!pHost) {
	if(ISeTrue(GetSymbolValueREF("MSQL_HOST",&pHost))) {
	    sprintf(sEnvBuf,"MSQL_HOST=%s", pHost);
	    putenv(strdup(sEnvBuf));
	}
    }

    /*
    /* pConnect Parameter overrides the MSQL_DB env variable
     */
    if( pConnect ) {
	pBuf=pConnect;
    } else {
	RETeFalse(GetSymbolValueREF("MSQL_DB",&pBuf)
		 ,"Can't find MSQL_DB"
		 );
    }

    /*
    /* Update the Generic DBI LDA Struct
     */
    pLDA->iLdaIndex = siConnectIndex;
    pLDA->bConnected= eFalse;
    pLDA->pConnect  = pBuf;

    /*
    /* DO IT!
     */
    giLDA[pLDA->iLdaIndex] = msqlConnect(pHost);
    if(giLDA[pLDA->iLdaIndex] == MSQL_ERROR) {
	MsgPush("DBERR:mSQL:msqlConnect(%s):%s",pHost,msqlErrMsg);
	return(eFalse);
    }
    if(msqlSelectDB(giLDA[pLDA->iLdaIndex],pLDA->pConnect) == MSQL_ERROR){
	MsgPush("DBERR:mSQL:msqlSelectDB(%s):%s",pLDA->pConnect,msqlErrMsg);
	return(eFalse);
    }

    siConnectIndex++; 		/* Bump the Connection Index */
    pLDA->bConnected = eTrue;	/* Set Connected Flag */
    return(eTrue);		/* Oh, sweet success! */
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

    msqlClose(giLDA[pLDA->iLdaIndex]);
    return(eTrue);
}

/*
/* Commit an MSQL Transaction
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
    int iCursorIndex
	;
    eBoolean_t bQuery;

    if(pStmt==(char*)0)			return(eTrue);
    if(pLDA==(SQLWEB_LDA*)0)		return(eTrue);
    if(pCursor==(SQLWEB_CURSOR*)0)	return(eTrue);

    RETeFalse(GetCursorIndex(&iCursorIndex),"GetCursorIndex Failed");

    DebugHTML(__FILE__,__LINE__,2,"DbOpenCursor(%d)",iCursorIndex);

    /*
    /* Initialize the SQLWEB_CURSOR
     */
    pCursor->pLDA	 = pLDA;
    pCursor->iCursorIndex= iCursorIndex;
    pCursor->bOpen	 = eFalse;
    pCursor->bFound	 = eFalse;
    pCursor->pStmt	 = pStmt;
    pCursor->lSelect	 = l_create("INORDER");	/* Indicator Variables */
/*    pCursor->lBind	 = l_create("INORDER");	/* cleanup */

    /* MSQL Thing....
     */

    /*
    /* Describe the BIND variables -- updates pCursor->pStmt!
     */
    RETeFalse(DescribeBindVars(pCursor)
	     ,"Describe Bind Variables Failed"
	     );
    if(msqlQuery(giLDA[pLDA->iLdaIndex],pCursor->pStmt) == MSQL_ERROR){
	MsgPush("DBERR:mSQL:msqlQuery(%s):%s",pCursor->pStmt,msqlErrMsg);
	return(eFalse);
    }
    gaCDA[pCursor->iCursorIndex] = msqlStoreResult();
    if( gaCDA[pCursor->iCursorIndex] == (m_result *)0) {
	/* Non SELECT, I guess... */
	return(eTrue);
    } 
    RETeFalse(DescribeSelectVars(pCursor)
	     ,"Describe Select Variables Failed"
	     );
    
    pCursor->bOpen = eTrue;
    
    /*
    /* Alls well that end well!
     */
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
    int iFields;
    if(ISeFalse(pCursor->bOpen)) {
	DebugHTML(__FILE__,__LINE__,1,"DbFetchCursor:Not Open");
	pCursor->bFound=eFalse;
	return(eTrue);
    }
    gRow = msqlFetchRow(gaCDA[pCursor->iCursorIndex]);
    iFields = msqlNumFields(gaCDA[pCursor->iCursorIndex]);
    if(iFields >l_size(pCursor->lSelect)){
	MsgPush("Too few fields");
	gRow = (m_row**)0;
    }
    if( gRow == (m_row)0 ) {
	pCursor->bFound = eFalse;
	pCursor->bOpen  = eFalse;
	/*
	/* Close the Cursor, (free result set)
	 */
	msqlFreeResult(gaCDA[pCursor->iCursorIndex]);
	return(eTrue);
    }
    RETeFalse(l_scan(pCursor->lSelect,SetSelectSymbols),"SetSelectSym Failed");
    pCursor->bFound=eTrue;

    /*
    /* Force a Close, good for single row queries
     */
    if(bClose) {
	/* Only required if the CURSOR will be ReUsed
	 */
	msqlFreeResult(gaCDA[pCursor->iCursorIndex]);
	pCursor->bOpen  = eFalse;
    }
    return(eTrue);
}
/* 
/* 
/* /*************************************************************
/* /* 
/* /* Supporting Routines, not part of the API, all additional
/* /* functions should be declared static!
/*
 */

/* Setup Symbols from this selected ROW
/* references the global gRow
 */
static eBoolean_t
SetSelectSymbols(DbInfo_t *pDbInfo)
{
    if(gRow[pDbInfo->iPosition])
	 strcpy(pDbInfo->pValue,gRow[pDbInfo->iPosition]);
    else pDbInfo->pValue = "";
    return(eTrue);
}


/*
/* Get the Next Available CursorIndex from
/* the gaCDA array.
 */
static eBoolean_t
GetCursorIndex(int *piCursorIndex)
{
    static int siCursorIndex;

    /* Should add overflow checking...
     */
    (*piCursorIndex) = siCursorIndex++;
    return(eTrue);

/*    int iStart, i;
/*
/*
/*    for(i=0, iStart=siCursorIndex
/*	    ;i==0 || (iStart != siCursorIndex && i>0)
/*	    ;i++, siCursorIndex = (siCursorIndex+1)%MAX_CURSORS)
/*    {
/*	if( gaCDA[siCursorIndex].fc == 0	/* Globals init'd to Zero */
/*	    || gaCDA[siCursorIndex].fc == FC_OCLOSE)
/*	{
/*	    (*piCursorIndex) = siCursorIndex;
/*	    return(eTrue);
/*	}
/*    }
/*    MsgPush("Exceeded Maximum Cursors");
/*    return(eFalse);
 */
}

/*
 * Describe the BIND Variables...
 * The idea here is to replace non-literals which begin with ':'
 * with the appropriate Symbol.  This does not use the same method
 * that the Oracle interface uses (using OCI describe) but could
 * be used in Oracle, Sybase, or MSQL as-is.
 */
static eBoolean_t
DescribeBindVars(SQLWEB_CURSOR *pCursor)
{
    char sCmd[MAX_TOKVAL_SIZE]
	,*cmd
	;
    int i, n, iLen;
    eBoolean_t bLiteral, bBack_Bumped = eFalse;
    char *cp;
    char *pValue;

    /* Find and bind input variables for placeholders.
     */
    memset(sCmd,0,MAX_SQL_IDENTIFIER);
    i = 0;
    bLiteral = eFalse;

    /* Parse the string through the null-terminator */
    for (cmd = sCmd, cp = pCursor->pStmt; *cp; )
    {
	/* Single Quoted Strings are SQL literals */
	if (*cp == '\'') {
	    bLiteral = ISeTrue(bLiteral)?eFalse:eTrue; /* Toggle */
	}

	/* Did we find a colon ':'?  Also, make sure not in a literal */
	if (*cp == ':' && ISeFalse(bLiteral) )
	{
	    static char buf[MAX_SQL_IDENTIFIER];

            n = 0;
	    for (++cp;
		 *cp && (isalnum(*cp) || *cp == '_')
		     && (n < MAX_SQL_IDENTIFIER);
		 cp++
		)
	    {
		buf[n++] = *cp; /* Copy the BIND symbol */
	    }
	    buf[n]=0;           /* NULL the 'buf' Buffer */
	    /*
	    /* buf now points to the SYMBOL NAME
	    /* so...  go-n-get it!
	     */
	    if(ISeFalse(GetSymbolValueREF(buf,&pValue))){
		/* There is no symbol by that name, stick in NULL WARNING?
		 */
		pValue = 0; /* should have been done by Get.. */
	    }
	    iLen = pValue ? strlen(pValue) : 0;

            /* Copy the bind variable into the cmd buffer */
	    if (iLen > 0)
	    {
		/* Wrap the value in single quotes */
        	*cmd++ = '\'';
        	for(i = 0; i < iLen; i++)
		{
		    *cmd++ = pValue[i];
		    /* Was the character copied a single quote?  Fix it with
		     * with another one
		     */
		    if (pValue[i] == '\'')
		    {
		        *cmd++ = '\'';
		    }
		}
		*cmd++ = '\'';
	    }
	    else
            {
		strcpy(cmd, "NULL");	/* no ticks */
                cmd += 4;
            }
	}   /* end if (*cp == ...) BIND VARIABLE FOUND */

	*cmd++ = *cp++;

    }   /* end for () */

    /* Null terminate the cmd string */
    *cmd = '\0';
    pCursor->pStmt	 = strdup(sCmd);

    return(eTrue);
}

/*
/* Describe the SELECT Variables...
 */
static eBoolean_t
DescribeSelectVars(SQLWEB_CURSOR *pCursor)
{
    DbInfo_t *pDbInfo;
    int i;
    m_field *pField;

    for(i=0;;i++) {
	pField = msqlFetchField(gaCDA[pCursor->iCursorIndex]);
	if(pField==(m_field *)0) {
	    break;
	}
	pDbInfo = (DbInfo_t*)malloc(sizeof(DbInfo_t));
	if(!pDbInfo){
	    MsgPush("malloc failed");
	    return(eFalse);
	}
	pDbInfo->pField = pField;
	pDbInfo->pValue = (char*)malloc(pField->length+1);
	memset(pDbInfo->pValue,0,pField->length+1);
	pDbInfo->iPosition = i;

	SELSym((char*)strdup(pField->name),pDbInfo->pValue);
	ENQ(pCursor->lSelect,pDbInfo);	/* For NULL Processing */
    }
    return(eTrue);
}
/* 
/* static void
/* PushDBErr(Lda_Def *pLDA
/* 	 ,Cda_Def *pCDA
/* 	 ,char *pErrMsg
/* 	 )
/* {
/*     sword n;
/*     text tBuf[512];
/* 
/*     /* if(pLDA==(Lda_Def*)0) return;
/*      */
/*     if(pCDA==(Cda_Def*)0) return;
/*     n = oerhms(pLDA,pCDA->rc,tBuf,(sword)sizeof(tBuf));
/*     tBuf[n]=0;
/*     MsgPush("DBERR:Oracle(%d):%s",pCDA->rc,tBuf);
/*     if(pErrMsg)
/* 	MsgPush("%s",pErrMsg);
/*     return;
/* }
/* */
