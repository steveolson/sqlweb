/* oradb.c - Oracle Interface for SQLweb
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

/*
/* Oracle INCLUDES.  These are defined in the "${ORACLE_HOME}/rdbms/demo"
/* directory for copyright purposes, these files must be included from your
/* local distribution.
 */
#include "oratypes.h"	/* Oracle Datatypes */
#include "ocidfn.h"	/* Oracle CDA and LDA structs */
#include "ociapr.h"	/* Oracle function defs */

/*
/* The DBINFO struct.  
 */
typedef struct {
    sb2 ind;		/* Oracle Indicator Variable BIND & SELECT */
    ub2 rlen;		/* Oracle Return Length of Variable */
    ub2 rcode;		/* Oracle "Return Code" for SELECT variables */
    char *pValue;	/* Another pointer to the SYMBOL Value */
    sword iLen;		/* Symbol pValue's (Malloced) Length */
} DbInfo_t;

/*
/* Internal/Private Functions
 */
static eBoolean_t DescribeBindVars  (SQLWEB_CURSOR *pCursor);
static eBoolean_t DescribeSelectVars(SQLWEB_CURSOR *pCDA);
static eBoolean_t GetCursorIndex(int *piCursorIndex);
static void       PushDBErr(Lda_Def *pLDA,Cda_Def *pCDA,char *pErrMsg);
static eBoolean_t SetDbNulls(DbInfo_t *pDbInfo);
static DbInfo_t * NewDbInfo();
static eBoolean_t FreeDbInfo(DbInfo_t *pDbInfo);


/*
/* oparse flags
 */
#define PARSE_DEFER	0	/* defflg */
#define PARSE_NO_DEFER	1	/* defflg */
#define LNG_FL_V6	0	/* lngflg */
#define LNG_FL_SRV	1	/* lngflg */
#define LNG_FL_V7	2	/* lngflg */

#define MAX_ITEM_BUFFER_SIZE    33
#define MAX_SQL_IDENTIFIER      31

#define VARCHAR2_TYPE            1
#define NUMBER_TYPE              2
#define INT_TYPE                 3
#define FLOAT_TYPE               4
#define STRING_TYPE              5
#define ROWID_TYPE              11
#define DATE_TYPE               12
#define LONG_TYPE		 8
/* #define LONGRAW_TYPE		24

/*  ORACLE error codes used in demonstration programs */
#define VAR_NOT_IN_LIST       1007
#define NO_DATA_FOUND         1403
#define NULL_VALUE_RETURNED   1405

/*  some SQL and OCI function codes */
#define FT_INSERT		3
#define FT_SELECT		4
#define FT_UPDATE		5
#define FT_DELETE		9
#define FC_OOPEN		14
#define FC_OCLOSE		16

/*
/* Global Variables
 */
char 
     *gpIfExpr = "SELECT 1 SQLWEB_IF_EXPR FROM DUAL WHERE"
    ,*gpNullSelect = "SELECT 1 FROM DUAL WHERE 1=2"
    ;

static LIST *glDbInfoList=NULL_LIST;

/*
/* THE LDA Array....
 */
Lda_Def gaLDA[MAX_CONNECTIONS];
#if (defined(__osf__) && defined(__alpha)) || defined(CRAY) || defined(KSR)
ub1     gaHDA[MAX_CONNECTIONS][512];
#else
ub1     gaHDA[MAX_CONNECTIONS][256];
#endif

/*
/* THE CURSOR Array
 */
Cda_Def gaCDA[MAX_CURSORS];

/*
/* MAIN for testing only
#define MAIN_TEST
 */
#ifdef MAIN_TEST
eBoolean_t gbCookFlag;

main(){
    /* ctSetup();
	*/
    char sBuf[BUFSIZ];
    FRMSym("name","steve");
    FRMSym("nameb","so");
    FRMSym("V2PGN","55");
    (void)x();
    while( MsgPop(sBuf) ) {
	printf("%s\n",sBuf);
    }
}

int x(){
    SQLWEB_LDA lda;
    SQLWEB_CURSOR cda;
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
    RETeFalse(DbConnect("sqlweb/sqlweb",&lda),"Connect Failed");
    RETeFalse(DbOpenCursor(&lda,p1,&cda),"Open Failed");
    GetSymbolValueREF("PI_CONTENTS",&pDaDat);
    GetSymbolValueREF("PI_SEQ_NBR",&pPageName);
    GetSymbolValueREF("TAG_NAME",&pPageTitle);
    do{
	RETeFalse(DbFetchCursor(&cda,eFalse),"Fetch Failed");
	fprintf(stderr,"%s %s C(%s)\n",pPageName,pPageTitle,pDaDat);
    } while(cda.bFound==eTrue);
    RETeFalse(DbDisconnect(&lda,eFalse),"Disconnect Failed");

    return(0);
}
#endif /* MAIN_TEST */

/*
/* Connect to the ORACLE DATABASE.
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
	;

    if( siConnectIndex > MAX_CONNECTIONS ){
	MsgPush("Exceeded Maximum Connections");
	return(eFalse);
    }

    /*
    /* Prepare to Connect to ORACLE
     */

    /*
    /* Must be Sure ORACLE_HOME is in the Environment!
     */
    pBuf = getenv("ORACLE_HOME");
	/* The Environment overrides the SYMBOL TABLE
	 */
    if(!pBuf) {
	RETeFalse(GetSymbolValueREF("ORACLE_HOME",&pBuf)
		 ,"Can't find ORACLE_HOME"
		 );
	sprintf(sEnvBuf,"ORACLE_HOME=%s",pBuf);
	putenv(strdup(sEnvBuf));
    }

    /*
    /* Check for ORACLE_SID, this is NOT REQUIRED because the
    /* connect string (remote) databases don't use ORACLE_SID
     */
    pBuf = getenv("ORACLE_SID");
	/* Again, Environment overrides the SYMBOL Table
	 */
    if(!pBuf) {
	if(ISeTrue(GetSymbolValueREF("ORACLE_SID",&pBuf))) {
	    sprintf(sEnvBuf,"ORACLE_SID=%s", pBuf);
	    putenv(strdup(sEnvBuf));
	}
    }

    /*
    /* pConnect Parameter overrides the ORACLE_CONNECT env variable
     */
    if( pConnect ) {
	pBuf=pConnect;
    } else {
	RETeFalse(GetSymbolValueREF("ORACLE_CONNECT",&pBuf)
		 ,"Can't find ORACLE_CONNECT"
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
    if(orlon(&gaLDA[pLDA->iLdaIndex]	/* LDA Array,supports multiple con's */
	    ,gaHDA[pLDA->iLdaIndex]	/* HDA Array, " */
	    ,(text*)pLDA->pConnect	/* Username/Password@Host */
	    ,-1				/* pConnect is NULL Terminated */
	    , 0				/* NULL Password, uses pConnect */
	    ,-1				/* NULL Password */
	    ,-1				/* No longer used */
	    ))
    {
	/* Actual Database Login Failed
	 */
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,(Cda_Def *)&gaLDA[pLDA->iLdaIndex]	/* no cursor, yet */
		 ,"Connect Failed."
		 );
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

    if(ologof( &gaLDA[pLDA->iLdaIndex])) {
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,(Cda_Def *)&gaLDA[pLDA->iLdaIndex]	/* use LDA as CDA! */
		 ,"Disconnect Failed."
		 );
	return(eFalse);
    }
    return(eTrue);
}

/*
/* Commit an Oracle Transaction
 */
eBoolean_t
DbCommit(SQLWEB_LDA *pLDA)	/* LDA struct from DbConnect() */
{
    if(ocom(&gaLDA[pLDA->iLdaIndex])){
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,(Cda_Def *)&gaLDA[pLDA->iLdaIndex]	/* Use LDA for CDA */
		 ,"Commit Failed."
		 );
	return(eFalse);
    }
    return(eTrue);
}

/*
/* Rollback a Transaction.
 */
eBoolean_t
DbRollback(SQLWEB_LDA *pLDA)	/* LDA struct ftom DbConnect() */
{
    if(orol(&gaLDA[pLDA->iLdaIndex])){
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,(Cda_Def *)&gaLDA[pLDA->iLdaIndex]	/* Use LDA for CDA */
		 ,"Rollback Failed."
		 );
	return(eFalse);
    }
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
    int iCursorIndex;
    ub2 sql_ft;

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
    pCursor->lBind	 = l_create("INORDER");	/* cleanup */

    /*
    /* Open the CURSOR
     */
    if(oopen(&gaCDA[pCursor->iCursorIndex]
	    ,&gaLDA[pLDA->iLdaIndex]
	    ,(text*)0
	    ,-1
	    ,-1
	    ,(text*)0
	    ,-1
	    ))
    {
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,&gaCDA[pCursor->iCursorIndex]
		 ,"oopen failed."
		 );
	return(eFalse);
    }

    /*
    /* Parse the SQL Statement
     */
    if(oparse(&gaCDA[pCursor->iCursorIndex]
	     ,(text*)pStmt
	     ,-1
	     ,PARSE_NO_DEFER	/* see 4-70 in OCI manual */
	     ,LNG_FL_V7
	     ))
    {
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,&gaCDA[pCursor->iCursorIndex]
		 ,"oparse failed."
		 );
	return(eFalse);
    }
    sql_ft=gaCDA[pCursor->iCursorIndex].ft;


    /*
    /* Describe the BIND variables, (no thanks to Oracle!)
     */
    RETeFalse(DescribeBindVars(pCursor),"Describe Bind Variables Failed");

    if(sql_ft==FT_SELECT) {
	/*
	/* For SELECT Statements, describe the SELECT Variables.
	 */
	RETeFalse(DescribeSelectVars(pCursor)
		,"Describe Select Variables Failed"
		);
    }

    /*
    /* Execute the SQL Statement
     */
    if(oexec(&gaCDA[pCursor->iCursorIndex])){
	PushDBErr(&gaLDA[pLDA->iLdaIndex]
		 ,&gaCDA[pCursor->iCursorIndex]
		 ,"oexec failed."
		 );
	return(eFalse);
    }

    if(sql_ft==FT_SELECT) {
	/* Update the SQLWEB_CURSOR status for SELECT STATEMENTS ONLY
	/* This CURSOR is OPEN for BUSINESS!
	 */
	pCursor->bOpen = eTrue;
    } else {
	/* NON-SELECT statements get CLOSED right away
	 */
	if(oclose(&gaCDA[pCursor->iCursorIndex])) {
	    PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
		     ,&gaCDA[pCursor->iCursorIndex]
		     ,"oclose failed."
		     );
	    return(eFalse);
	}
    }

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
    eBoolean_t bStatus;

    if(ISeFalse(pCursor->bOpen)) {
	DebugHTML(__FILE__,__LINE__,1,"DbFetchCursor:Not Open");
	pCursor->bFound=eFalse;
	return(eTrue);
    }

    if(ofetch(&gaCDA[pCursor->iCursorIndex])
	/* && gaCDA[pCursor->iCursorIndex].rc != NULL_VALUE_RETURNED
	 */)
    {
	pCursor->bFound = eFalse;

	if (gaCDA[pCursor->iCursorIndex].rc == NO_DATA_FOUND) {
	    bStatus = eTrue;
	} else {
	    PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
		     ,&gaCDA[pCursor->iCursorIndex]
		     ,"ofetch failed."
		     );
	    bStatus = eFalse;
	}
	/*
	/* Close the Cursor
	 */
	if(oclose(&gaCDA[pCursor->iCursorIndex])) {
		PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
			 ,&gaCDA[pCursor->iCursorIndex]
			 ,"oclose failed."
			 );
		bStatus = eFalse;
	} else {
	    pCursor->bOpen = eFalse;
	}
	/*
	/* Free the DbInfo_t's Created for this Cursor
	 */
	l_scan(pCursor->lBind,FreeDbInfo);
	l_scan(pCursor->lSelect,FreeDbInfo);

	return(bStatus);
    }
    /*
    /* Found a ROW
     */
    l_scan(pCursor->lSelect,SetDbNulls);

    pCursor->bFound=eTrue;
    bStatus = eTrue;

    /*
    /* Force a Close, good for single row queries
     */
    if(bClose) {
	/* Only required if the CURSOR will be ReUsed
	/* if(ocan(&gaCDA[pCursor->iCursorIndex])) {
	/*  PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
	/*	     ,&gaCDA[pCursor->iCursorIndex]
	/*	     ,"ocan failed."
	/*	     );
	/*  bStatus = eFalse;
	/*}
	 */
	if(oclose(&gaCDA[pCursor->iCursorIndex])) {
	    PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
		     ,&gaCDA[pCursor->iCursorIndex]
		     ,"oclose failed."
		     );
	    bStatus = eFalse;
	}
    }
    return(bStatus);
}


/*************************************************************
/* 
/* Supporting Routines, not part of the API, all additional
/* functions should be declared static!
 */

/*
/* Set NULLs based on the INDICATOR Variable
/* This is called from the DbFetchCursor() routine
 */
static eBoolean_t
SetDbNulls(DbInfo_t *pDbInfo)
{
    if(pDbInfo->ind<0 && pDbInfo->pValue) {
	memset(pDbInfo->pValue,0,pDbInfo->iLen);
    }
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
    int iStart, i;

    for(i=0, iStart=siCursorIndex
	    ;i==0 || (iStart != siCursorIndex && i>0)
	    ;i++, siCursorIndex = (siCursorIndex+1)%MAX_CURSORS)
    {
	if( gaCDA[siCursorIndex].fc == 0	/* Globals init'd to Zero */
	    || gaCDA[siCursorIndex].fc == FC_OCLOSE)
	{
	    (*piCursorIndex) = siCursorIndex;
	    return(eTrue);
	}
    }
    MsgPush("Exceeded Maximum Cursors");
    return(eFalse);
}

/*
/* Describe the BIND Variables...
 */
static eBoolean_t
DescribeBindVars(SQLWEB_CURSOR *pCursor)
{
    sword i, n;
    eBoolean_t bLiteral;
    text *cp;

    /* Find and bind input variables for placeholders.
     */
    for (i=0, bLiteral=eFalse, cp=pCursor->pStmt; *cp; cp++)
    {
	DbInfo_t *pDbInfo;

	/* Single Quoted Strings are SQL literals */
        if (*cp == '\'') {
            bLiteral = ISeTrue(bLiteral)?eFalse:eTrue; /* Toggle */
	}

        if(*cp == ':' && ISeFalse(bLiteral) && *(cp+1) != '=')
        {
	    sb1 buf[MAX_SQL_IDENTIFIER];
            for ( ++cp, n=0;
                 *cp && (isalnum(*cp) || *cp == '_')
                     && n < MAX_SQL_IDENTIFIER;
                 cp++, n++
                )
	    {
                buf[n]=*cp;
	    }
            buf[n]=0;		/* NULL the 'buf' Buffer */
	    if(! *cp) --cp;	/* If this the end of the buffer, there
				/* is a problem because the outer loop
				/* increments the cp pointer again, 
				/* BEYOND THE END OF THIS STRING!!!
				 */
	    /*
	    /* buf now points to the SYMBOL NAME
	    /* so...  go-n-get it!
	     */
	    /* pDbInfo = (DbInfo_t*)malloc(sizeof(DbInfo_t));
	     */
	    pDbInfo = NewDbInfo();
	    if(!pDbInfo){
		MsgPush("malloc failed");
		return(eFalse);
	    }
	    ENQ(pCursor->lBind,pDbInfo);

	    if(ISeFalse(GetSymbolValueREF(buf,&pDbInfo->pValue))){
		/* There is no symbol by that name, stick in NULL
		/* WARNING?
		 */
		pDbInfo->pValue = 0; /* should have been done by Get.. */
	    }

	    pDbInfo->iLen = pDbInfo->pValue ? strlen(pDbInfo->pValue) : 0;
	    pDbInfo->ind  = pDbInfo->iLen>0 ? (sb2)0 : (sb2)-1;

	    DebugHTML(__FILE__,__LINE__,2
		    ,"DbOpen:GetS(%s):L=%d, I=%d"
		    ,buf
		    ,pDbInfo->iLen
		    ,pDbInfo->ind
		    );

            if(obndrv(&gaCDA[pCursor->iCursorIndex]
		     ,buf
		     ,-1
		     ,&pDbInfo->pValue[0]
		     ,-1 	/* pDbInfo->iLen>2000?pDbInfo->iLen+1:-1 */
		     ,VARCHAR2_TYPE
				/*pDbInfo->iLen>2000?LONG_TYPE:VARCHAR2_TYPE*/
		     ,-1
		     ,&pDbInfo->ind
		     ,(text *) 0
		     ,-1
		     ,-1
		     ))
            {
		PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
			 ,&gaCDA[pCursor->iCursorIndex]
			 ,"obndrv failed."
			 );
		MsgPush("s=%s,b=%x,rc=%d"
		       ,buf
		       ,pDbInfo->pValue
		       ,gaCDA[pCursor->iCursorIndex].rc
		       );
                return(eFalse);
            }

            i++;

        }   /* end if (*cp == ...) */
    }       /* end for () */
    return(eTrue);
}

/*
/* Describe the SELECT Variables...
 */
static eBoolean_t
DescribeSelectVars(SQLWEB_CURSOR *pCursor)
{
    sword col;
    char *pBuf;
    sb4 dbsize;
    sb2 dbtype;
    sb1 buf[MAX_ITEM_BUFFER_SIZE];
    sb4 buflen = MAX_ITEM_BUFFER_SIZE;
    sb4 dsize;
    sb2 precision;
    sb2 scale;
    sb2 nullok;

    DbInfo_t *pDbInfo;

    /* Describe the select-list items. */
    for (col = 0; ; col++)
    {
	buflen = MAX_ITEM_BUFFER_SIZE;
        if(odescr(&gaCDA[pCursor->iCursorIndex]
		 ,col+1
		 ,&dbsize
		 ,&dbtype
		 ,&buf[0]
		 ,&buflen
		 ,&dsize
                 ,&precision
		 ,&scale
		 ,&nullok
		 ))
        {
            /* Break on end of select list. */
            if ((&gaCDA[pCursor->iCursorIndex])->rc == VAR_NOT_IN_LIST)
                break;
            else
            {
		PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
			 ,&gaCDA[pCursor->iCursorIndex]
			 ,"odescr failed."
			 );
                return(eFalse);
            }
        }
        /* adjust sizes
	 */
	buf[buflen]=0;
        switch (dbtype)
        {
	    case NUMBER_TYPE:
	    case INT_TYPE:
	    case FLOAT_TYPE:	dbsize = 44; break;
	    case DATE_TYPE:	dbsize = 44; break;
	    case ROWID_TYPE:	dbsize = 18; break;
	    case LONG_TYPE:	dbsize = 65533; break;
        }

	/*
	/* Build a "DB" Symbol
	 */
	buf[buflen]=0;

	/*
	/* Got a "Select-Item" in buf setup a SYMBOL for it.
	 */
	/* pDbInfo = (DbInfo_t*)malloc(sizeof(DbInfo_t));
	 */
	pDbInfo = NewDbInfo();
	if(!pDbInfo){
	    MsgPush("malloc failed");
	    return(eFalse);
	}

	memset(pDbInfo,0,sizeof(DbInfo_t));
	pDbInfo->pValue = (char*)malloc(dbsize+1);
	pDbInfo->iLen   = dbsize+1;

	if(!pDbInfo->pValue) {
	    MsgPush("malloc failed");
	    return(eFalse);
	}
	memset(pDbInfo->pValue,0,dbsize+1);
	ENQ(pCursor->lSelect,pDbInfo);	/* For NULL Processing */

	if(ISeTrue(GetSymbolValueREF(buf,&pBuf)) && strlen(pBuf)<=dbsize) {
	    strcpy(pDbInfo->pValue,pBuf);
		/* Don't assume a symbol was malloc'd, (you hosehead)
		/* Add Symbol will take care of memory leakage...
	    	/* FreeBuf(pBuf);
		/* Free previous Symbol Value
		 */
	}
/*
/*	SELSym((char*)strdup(buf),pDbInfo->pValue);
 */
	RETeFalse2(AddSymbol(SEL_SYMBOL
			    ,(char*)strdup(buf)
			    ,pDbInfo->pValue
			    ,eTrue
			    ,dbsize+1
			    )
		  ,"AddSymbol(SEL,%s) Failed." \
		  ,buf
		  );

	DebugHTML(__FILE__,__LINE__,2,"DbOpen:AddS(%s)",buf);

	/*
	/* Let Oracle Convert everything to STRING
	 */
	dbtype = STRING_TYPE;
        if (odefin(&gaCDA[pCursor->iCursorIndex]
		  ,col+1
		  ,&pDbInfo->pValue[0]
		  ,dbsize+1
		  ,dbtype
		  ,-1
		  ,&pDbInfo->ind
		  ,(text *) 0
		  ,-1
		  ,-1
		  ,&pDbInfo->rlen
		  ,&pDbInfo->rcode
		  ))
        {
	    PushDBErr(&gaLDA[pCursor->pLDA->iLdaIndex]
		     ,&gaCDA[pCursor->iCursorIndex]
		     ,"odefin failed."
		     );
            return(eFalse);
        }
    }
    return(eTrue);
}

static void
PushDBErr(Lda_Def *pLDA
	 ,Cda_Def *pCDA
	 ,char *pErrMsg
	 )
{
    sword n;
    text tBuf[512];

    /* if(pLDA==(Lda_Def*)0) return;
     */
    if(pCDA==(Cda_Def*)0) return;
    n = oerhms(pLDA,pCDA->rc,tBuf,(sword)sizeof(tBuf));
    tBuf[n]=0;
    MsgPush("DBERR:Oracle(%d):%s",pCDA->rc,tBuf);
    if(pErrMsg)
	MsgPush("%s",pErrMsg);
    return;
}

static DbInfo_t *
NewDbInfo()
{
    DbInfo_t *pDbInfo;
    if(!glDbInfoList){
	glDbInfoList=l_create("STACK");
	if(!glDbInfoList) {
	    MsgPush("NewDbInfo:l_create failed");
	    return(0);
	}
    }
    if(l_size(glDbInfoList)>0) {
	DebugHTML(__FILE__,__LINE__,3,"NewDbInfo:POP(%d)",l_size(glDbInfoList));
	pDbInfo=(DbInfo_t*)POP(glDbInfoList);
    } else {
	pDbInfo=(DbInfo_t*)malloc(sizeof(DbInfo_t));
    }
    if(!pDbInfo) {
	MsgPush("NewDbInfo:malloc or POP failed");
	return(0);
    }
    (void)memset(pDbInfo,0,sizeof(DbInfo_t));
    return(pDbInfo);
}

static eBoolean_t
FreeDbInfo(DbInfo_t *pDbInfo)
{
    DebugHTML(__FILE__,__LINE__,3,"FreeDbInfo[%x]",pDbInfo);
    if(!glDbInfoList) {
	glDbInfoList=l_create("STACK");
	if(!glDbInfoList) {
	    MsgPush("FreeDbInfo:l_create failed");
	    return(eFalse);
	}
    }

    /*
    /* Free pValue....
    Free(pDbInfo->pValue);
     */

    /*
    /* Store This PI on the FreeLIst
     */
    DebugHTML(__FILE__,__LINE__,3,"FreeDbInfo:PUSH(%d)",l_size(glDbInfoList));
    PUSH(glDbInfoList,pDbInfo);

    return(eTrue);
}
