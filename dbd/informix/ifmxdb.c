/* ifmxdb.c - Informix SQLweb Interface
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
/* Informix INCLUDES.  
 */
#include "infxcli.h"

/*
/* The DBINFO struct.  
 */
typedef struct {
    long ind;		/* Indicator Variable BIND & SELECT */
    SDWORD rlen;	/* Bind/Fetch Return Length  */
    char *pValue;	/* Another pointer to the SYMBOL Value */
    long iLen;		/* Symbol pValue's (Malloced) Length */
    SWORD iScale;	/* Scale of Number ... */
    SWORD iColType;	/* Type of Number ... */
} DbInfo_t;

/*
/* Internal/Private Functions
 */
static eBoolean_t DescribeBindVars  (SQLWEB_CURSOR *pCursor);
static eBoolean_t DescribeSelectVars(SQLWEB_CURSOR *pCDA);
static eBoolean_t GetCursorIndex(int *piCursorIndex);
static void       PushDBErr(HENV pLDA,HSTMT pCDA,char *pErrMsg);
static eBoolean_t SetDbNulls(DbInfo_t *pDbInfo);
static DbInfo_t * NewDbInfo();
static eBoolean_t FreeDbInfo(DbInfo_t *pDbInfo);
static eBoolean_t PrepareParameters(SQLWEB_CURSOR *pCursor);
static eBoolean_t FreeStmt(SQLWEB_CURSOR *pCursor);

#define MAX_ITEM_BUFFER_SIZE    33
#define MAX_SQL_IDENTIFIER      31

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
HENV	gaLDA[MAX_CONNECTIONS]; /* Lda_Def gaLDA[MAX_CONNECTIONS]; */
HDBC	gaHDA[MAX_CONNECTIONS]; /* ub1     gaHDA[MAX_CONNECTIONS][512]; */

/*
/* THE CURSOR Array
 */
HSTMT gaCDA[MAX_CURSORS];	/* Cda_Def gaCDA[MAX_CURSORS]; */
eBoolean_t gbaOpen[MAX_CURSORS];	/* Open indicator */

/*
/* MAIN for testing only
#define MAIN_TEST
 */

#ifdef MAIN_TEST
eBoolean_t gbCookFlag;
main(){
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
/* Connect to the INFORMIX DATABASE.
/* The pConnect parameter should be passed as (char *)0 to instruct
/*     the DBI to use the INFORMIX_DRIVERCONNECT Symobl.  The pConnect is
/*	only used to Override the INFORMIX_DRIVERCONNECT symbol.
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
	,sBuf[BUFSIZ]
	,*pBuf
	;
    short sOutSize;
    RETCODE rc;

    if( siConnectIndex > MAX_CONNECTIONS ){
	MsgPush("Exceeded Maximum Connections");
	return(eFalse);
    }

    /*
    /* Must be Sure INFORMIXDIR is in the Environment!
     */
    pBuf = getenv("INFORMIXDIR");
	/* The Environment overrides the SYMBOL TABLE
	 */
    if(!pBuf) {
	RETeFalse(GetSymbolValueREF("INFORMIXDIR",&pBuf)
		 ,"Can't find INFORMIXDIR"
		 );
	sprintf(sEnvBuf,"INFORMIXDIR=%s",pBuf);
	putenv(DupBuf(sEnvBuf));
    }

    /*
    /* pConnect Parameter overrides the INFORMIX_DRIVERCONNECT env variable
     */
    if( pConnect ) {
	pBuf=pConnect;
    } else {
	RETeFalse(GetSymbolValueREF("INFORMIX_DRIVERCONNECT",&pBuf)
		 ,"Can't find INFORMIX_DRIVERCONNECT"
		 );
    }

    /*
    /* Update the Generic DBI LDA Struct
     */
    pLDA->iLdaIndex = siConnectIndex;
    pLDA->bConnected= eFalse;
    pLDA->pConnect  = pBuf;

    /* Allocate the Environment handle
     */
    if( SQLAllocEnv( &gaLDA[pLDA->iLdaIndex] ) ) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
		 ,"Connect Failed (SQLAllocEnv)."
		 );
        return(eFalse);
    }

    /* Allocate the Connection handle
     */
    rc=SQLAllocConnect(gaLDA[pLDA->iLdaIndex],&gaHDA[pLDA->iLdaIndex]);
    if(rc){
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
		 ,"Connect Failed (SQLAllocConnect)."
		 );
        return(eFalse);
    }

    rc=SQLDriverConnect(gaHDA[pLDA->iLdaIndex]
			 ,NULL
			 ,(UCHAR*)pBuf
			 ,SQL_NTS
			 ,(UCHAR*)sBuf
			 ,BUFSIZ
			 ,&sOutSize
			 ,SQL_DRIVER_NOPROMPT
			 );
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
		 ,"Connect Failed (SQLDriverConnect)."
		 );
	return(eFalse);
    }

    siConnectIndex++; 		/* Bump the Connection Index */
    pLDA->bConnected = eTrue;	/* Set Connected Flag */
    DebugHTML(__FILE__,__LINE__,3,"DbConnect(%s) OK!",pBuf);
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

    if(SQLDisconnect(&gaLDA[pLDA->iLdaIndex])){
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
		 ,"Disconnect Failed."
		 );
	return(eFalse);
    }
    pLDA->bConnected = eFalse;	/* Set Connected Flag */
    return(eTrue);
}

/*
/* Commit Transaction
 */
eBoolean_t
DbCommit(SQLWEB_LDA *pLDA)	/* LDA struct from DbConnect() */
{
    RETCODE rc;
    rc=SQLTransact(gaLDA[pLDA->iLdaIndex],gaHDA[pLDA->iLdaIndex],SQL_COMMIT);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
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
    RETCODE rc;
    rc=SQLTransact(gaLDA[pLDA->iLdaIndex],gaHDA[pLDA->iLdaIndex],SQL_ROLLBACK);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,(HSTMT)0
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
    RETCODE rc;

    if(pStmt==(char*)0)			return(eTrue);
    if(pLDA==(SQLWEB_LDA*)0)		return(eTrue);
    if(pCursor==(SQLWEB_CURSOR*)0)	return(eTrue);

    /* Supports Delayed Connect
     */
    if( ISeFalse(pLDA->bConnected)) {
	RETeFalse(DbConnect(0,pLDA),"DbOpen (Connect) Failed");
    }
    RETeFalse(GetCursorIndex(&iCursorIndex),"GetCursorIndex Failed");

    DebugHTML(__FILE__,__LINE__,4,"DbOpenCursor:CursorIndex=%d",iCursorIndex);

    /*
    /* Initialize the SQLWEB_CURSOR
     */
    pCursor->pLDA	 = pLDA;
    pCursor->iCursorIndex= iCursorIndex;
    pCursor->bOpen	 = eFalse;
    gbaOpen[iCursorIndex] = eFalse;
    pCursor->bFound	 = eFalse;
    pCursor->pStmt	 = pStmt;
    pCursor->lSelect	 = l_create("INORDER");	/* Indicator Variables */
    pCursor->lBind	 = l_create("INORDER");	/* cleanup */

    /* Modify the SQL Stmt to meet Informix standards
    /* -- replace symbol refs with '?'
     */
    RETeFalse(PrepareParameters(pCursor),"PrepareParameters Failed");

    /* Allocate the statement handle
     */
    rc=SQLAllocStmt(gaHDA[pLDA->iLdaIndex],&gaCDA[iCursorIndex]);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO)
    {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,gaCDA[iCursorIndex]
		 ,"SQLAllocStmt Failed."
		 );
	return(eFalse);
    }

    rc=SQLPrepare(gaCDA[iCursorIndex],(UCHAR*)pCursor->pStmt,SQL_NTS);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,gaCDA[iCursorIndex]
		 ,"SQLPrepare Failed."
		 );
	return(eFalse);
    }

    /*
    /* Describe the BIND & SELECT-List variables
     */
    RETeFalse(DescribeBindVars(pCursor),"Describe Bind Variables Failed");
    RETeFalse(DescribeSelectVars(pCursor) ,"Describe Select Variables Failed");

    /* Off with his head! --EXECUTE THE STMT
     */
    rc=SQLExecute( gaCDA[iCursorIndex] );
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pLDA->iLdaIndex]
		 ,gaCDA[iCursorIndex]
		 ,"SQLExecute Failed."
		 );
	return(eFalse);
    }

    /* pCursor->bOpen is set by "side-effect" in DescribeSelectVars...
     */
    if(ISeFalse(pCursor->bOpen)) {
	RETeFalse(FreeStmt(pCursor),"FreeStmt Failed");
    }

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
    RETCODE rc;

    if(ISeFalse(pCursor->bOpen)) {
	DebugHTML(__FILE__,__LINE__,3,"DbFetchCursor:Not Open");
	pCursor->bFound=eFalse;
	return(eTrue);
    }

    /* Process the Fetch Request 
     */
    rc=SQLFetch(gaCDA[pCursor->iCursorIndex]);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	pCursor->bFound = eFalse;

	if(rc==SQL_NO_DATA_FOUND) {
	    bStatus =eTrue;
	} else {
	    PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		     ,gaCDA[pCursor->iCursorIndex]
		     ,"SQLFetch failed."
		     );
	    bStatus = eFalse;
	}
	/*
	/* Close the Cursor
	 */
	bStatus = FreeStmt(pCursor);

	/*
	/* Free the DbInfo_t's Created for this Cursor
	 */
	l_scan(pCursor->lBind,(PFI)FreeDbInfo);
	l_scan(pCursor->lSelect,(PFI)FreeDbInfo);

	return(bStatus);
    }

    /* OK, Found a ROW, Check for NULLs
     */
    l_scan(pCursor->lSelect,(PFI)SetDbNulls);

    pCursor->bFound=eTrue;
    bStatus = eTrue;

    /*
    /* Force a Close, good for single row queries
     */
    if(bClose)
	return( FreeStmt(pCursor) );
    return(eTrue);
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
    if(pDbInfo->rlen==SQL_NULL_DATA && pDbInfo->pValue) {
	DebugHTML(__FILE__,__LINE__,3,"Fetched a Null");
	memset(pDbInfo->pValue,0,pDbInfo->iLen);
    }
        switch (pDbInfo->iColType)
        {
	    case SQL_DECIMAL:
	    case SQL_DOUBLE:
	    case SQL_REAL:
	    case SQL_INTEGER:
	    case SQL_SMALLINT:
		if((pDbInfo->iScale)==0) {
		    char *p;
		    for(p=pDbInfo->pValue;*p;p++)if(*p=='.')*p=0;
		}
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
	/* the gbaOpen array simply lets me know when
	/* a cursor has been opened or closed so that
	/* in this routine I can re-use cursors!
	 */
	if( gbaOpen[siCursorIndex]==eFalse ) {
	    (*piCursorIndex) = siCursorIndex;
	    return(eTrue);
	}
    }
    MsgPush("Exceeded Maximum Cursors");
    return(eFalse);
}

/*
/* Describe the BIND Variables...
/*    this is much simplified since the parsing of the SQL
/*    string itself is handled in the PrepareParameters() function
/*    which was a hack of the origional decrbindvars...
 */
static eBoolean_t
DescribeBindVars(SQLWEB_CURSOR *pCursor)
{
    int iParamCnt=1;
    DbInfo_t *pDbInfo;

    for( pDbInfo=l_ffirst(pCursor->lBind);
	 pDbInfo;
	 pDbInfo=l_fnext(pCursor->lBind))
    {
	RETCODE rc;
	if( !pDbInfo->pValue) {
	     pDbInfo->pValue = DupBuf("");
	     pDbInfo->rlen = SQL_NULL_DATA;
	}
	else if( !*pDbInfo->pValue) {
	     pDbInfo->rlen = SQL_NULL_DATA;
	}
	else pDbInfo->rlen = SQL_NTS;

	/* bug fix ... */
	pDbInfo->rlen = SQL_NTS;
	/* bug fix ... */

	rc=SQLBindParameter( gaCDA[pCursor->iCursorIndex]
		    ,iParamCnt++
		    ,SQL_PARAM_INPUT
		    ,SQL_C_CHAR /* SQL_C_CHAR */
		    ,SQL_CHAR /* SQL_CHAR */
		    ,pDbInfo->iLen+1
		    ,0
		    ,pDbInfo->pValue /* &pDbInfo->pValue[0] */
		    ,pDbInfo->iLen+1
		    ,&pDbInfo->rlen
		    );
	if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	    PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		     ,gaCDA[pCursor->iCursorIndex]
		     ,"SQLBindParameter failed."
		     );
	    MsgPush("s='?',b=%x",pDbInfo->pValue);
	    return(eFalse);
	}
	DebugHTML(__FILE__,__LINE__,2,"Describe:Bound(%d: %s)",iParamCnt-1,pDbInfo->pValue);
    }
    return(eTrue);
}

/*
/* Describe the SELECT Variables...
 */
static eBoolean_t
DescribeSelectVars(SQLWEB_CURSOR *pCursor)
{
    SWORD NumCols, Col;
    RETCODE rc;
    UCHAR sColName [ MAX_ITEM_BUFFER_SIZE ];
    SWORD iColNameSize = sizeof(sColName)
	    ,iColType
	    ,iScale
	    ,iNullable
	;
    UDWORD iLength;
    char *pBuf;
    DbInfo_t *pDbInfo;

    rc=SQLNumResultCols(gaCDA[pCursor->iCursorIndex], &NumCols);
    if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		 ,gaCDA[pCursor->iCursorIndex]
		 ,"SQLNumResultCols failed."
		 );
	return(eFalse);
    }

    if(NumCols!=0) {
	gbaOpen[pCursor->iCursorIndex]=pCursor->bOpen=eTrue;
    }

    /* Describe each select-list item.
     */
    for(Col=1;Col<=NumCols;Col++) {
	rc=SQLDescribeCol(gaCDA[pCursor->iCursorIndex]
			,Col
			,sColName
			,(SWORD)sizeof(sColName)
			,&iColNameSize
			,&iColType
			,&iLength
			,&iScale
			,&iNullable
			);
	if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	    PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		 ,gaCDA[pCursor->iCursorIndex]
		 ,"SQLDescribe failed."
		 );
	    return(eFalse);
        }
	sColName[iColNameSize]=0;

	DebugHTML(__FILE__,__LINE__,2,"DbOpen:Desc:%s:Len=%d:Scale=%d:T=%d"
		,sColName
		,iLength
		,iScale
		,iColType
		);

        switch (iColType)
        {
	    case SQL_LONGVARBINARY:
	    case SQL_LONGVARCHAR:
		iLength = 65533;
		break;
	    case SQL_DATE:
	    case SQL_TIMESTAMP:
	    case SQL_DECIMAL:
	    case SQL_DOUBLE:
	    case SQL_INTEGER:
	    case SQL_REAL:
	    case SQL_SMALLINT:
		iLength=44;
		break;
	    case SQL_CHAR:
	    case SQL_VARCHAR:
		/* iLength set by Describe
		 */
		break;
	    default:
		iLength = 32768; break;
        }

	/*
	/* Build a "DB" Symbol
	 */
	pDbInfo = NewDbInfo();
	if(!pDbInfo){
	    MsgPush("malloc failed");
	    return(eFalse);
	}
	pDbInfo->pValue  =(char*)malloc(iLength+1);
	pDbInfo->iLen    =iLength+1;
	pDbInfo->iScale  =iScale;
	pDbInfo->iColType=iColType;
	if(!pDbInfo->pValue) {
	    /* Impossible occured! */
	    MsgPush("malloc failed");
	    return(eFalse);
	}
	memset(pDbInfo->pValue,0,iLength+1);
	ENQ(pCursor->lSelect,pDbInfo);	/* For NULL Processing */

	if(ISeTrue(GetSymbolValueREF((char*)sColName,&pBuf))
		&& strlen(pBuf)<=iLength)
	{
	    strcpy(pDbInfo->pValue,pBuf);
		/* Don't assume a symbol was malloc'd, (you hosehead)
		/* Add Symbol will take care of memory leakage...
		 */
	}

	if(! *pDbInfo->pValue)
	     pDbInfo->rlen = SQL_NULL_DATA;
	else pDbInfo->rlen = SQL_NTS;

	RETeFalse2(AddSymbol(SEL_SYMBOL
			    ,DupBuf((const char*)sColName)
			    ,pDbInfo->pValue
			    ,eTrue
			    ,(long)iLength+1
			    )
		  ,"AddSymbol(SEL,%s) Failed." 
		  ,(char*)sColName
		  );

	DebugHTML(__FILE__,__LINE__,2,"DbOpen:AddS(%s)",sColName);

	/* Let Informix Convert everything to STRING
	 */
	rc=SQLBindCol( gaCDA[pCursor->iCursorIndex]
			,Col
			,SQL_C_CHAR
			,&pDbInfo->pValue[0]
			,iLength+1
			,&pDbInfo->rlen
			);
	if(rc!=SQL_SUCCESS && rc!=SQL_SUCCESS_WITH_INFO) {
	    PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		     ,gaCDA[pCursor->iCursorIndex]
		     ,"SQLBindCol failed."
		     );
            return(eFalse);
        }
    }
    return(eTrue);
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
    pDbInfo->rlen = SQL_NTS;
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

/*
/* New function for Informix prot.  change the 
/* symbol references in the sql string to 
/* informix's '?' parameters so something like:
/* select * from dual where a = :a
/* becomes
/* select * from dual where a = ?
/* each symbol is converted into a ? parameter and the
/* DbInfo is stuffed onto the lBind list for the
/* cursor.
 */
static eBoolean_t
PrepareParameters(SQLWEB_CURSOR *pCursor)
{
    char *pNewStmt, *cpNew;
    int i, n;
    eBoolean_t bLiteral;
    char *cp;

    /* New stmt will always be at at most the same size
    /* as the origional because any symbol reference is
    /* requires a ':' and a symbol name and the parameter
    /* is a single '?'
     */
    cpNew = pNewStmt = malloc(strlen(pCursor->pStmt)+1);
    if(!pNewStmt) {
	MsgPush("Malloc Failed in PrepareParameters");
        return(eFalse);
    }
    /* Find and bind input variables for placeholders.
     */
    for (i=0, bLiteral=eFalse, cp=pCursor->pStmt; *cp; ) {
	DbInfo_t *pDbInfo;

	/* Single Quoted Strings are SQL literals */
        if (*cp == '\'') {
            bLiteral = ISeTrue(bLiteral)?eFalse:eTrue; /* Toggle */
	}

	/* Oh, boy, we've got a live one!
	 */
        /* if(*cp == ':' && ISeFalse(bLiteral) && *(cp+1) != '=') */
        if(*cp == ':' && ISeFalse(bLiteral))
        {
	    char buf[MAX_SQL_IDENTIFIER];
            for ( ++cp, n=0;
                 *cp && (isalnum(*cp) || *cp == '_')
                     && n < MAX_SQL_IDENTIFIER;
                 cp++, n++
                )
	    {
                buf[n]=*cp;
	    }
            buf[n]=0;		/* NULL the 'buf' Buffer */
	    /* if(! *cp) --cp;	/* If this the end of the buffer, there
				/* is a problem because the outer loop */
				/* increments the cp pointer again, 
				/* BEYOND THE END OF THIS STRING!!!
				 */
	    /*
	    /* buf now points to the SYMBOL NAME
	    /* so...  go-n-get it!
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
		pDbInfo->pValue=0; /* should have been done by Get.. */
		/* pDbInfo->rlen = SQL_NULL_DATA;
		 */
	    }
	    DebugHTML(__FILE__,__LINE__,2
		    ,"DbOpen:GetS(%s:%s)"
		    ,buf
		    ,pDbInfo->pValue? pDbInfo->pValue: ""
		    );

	    pDbInfo->iLen = pDbInfo->pValue ? strlen(pDbInfo->pValue) : 0;
	    /* pDbInfo->ind  = pDbInfo->iLen>0 ? 0 : -1;
	     */

            i++;

	    /* Install the Informix place holder
	     */
	    *cpNew++ = '?';
	    /* *cpNew++ = ' ';
	     */

        }   /* end if (*cp == ':') */
	else {
	    /* Update the Modified SQLStmt
	     */
	    *cpNew++ = *cp++;
	}
    }       /* end for () */
    /* NULL terminate the newly processed SQL stmt
     */
    *cpNew=0;

    /* Update the pCursor Structure 
     */
    pCursor->pStmt = pNewStmt;
    DebugHTML(__FILE__,__LINE__,3,"DbSQL(%s) OK!",pNewStmt);
    return(eTrue);
}

#define ErrMsgLen 200
static void
PushDBErr(HENV pLDA
	 ,HSTMT pCDA
	 ,char *pErrMsg
	 )
{
    RETCODE     rc = SQL_NO_DATA_FOUND;
    SWORD       pcbErrorMsgLen = 0;
    SDWORD      pfNativeError;
    UCHAR	ErrMsg[ErrMsgLen];
    char        SqlState[6];
    HENV        henv = (HENV)0;
    HDBC        hdbc = (HDBC)0;
    HSTMT       hstmt = pCDA;

    if(pErrMsg)
	MsgPush("%s",pErrMsg);

    if(hstmt) {
	rc=SQLError(NULL,NULL,hstmt
		,(UCHAR *)SqlState
		,&pfNativeError
		,(UCHAR *)ErrMsg
		,ErrMsgLen
		,&pcbErrorMsgLen
		);
    } else if(hdbc) {
	rc=SQLError(NULL,hdbc,NULL
		,(UCHAR *)SqlState
		,&pfNativeError
		,(UCHAR *)ErrMsg
		,ErrMsgLen
		,&pcbErrorMsgLen
		);
    } else if (henv) {
	rc=SQLError(henv,NULL,NULL
		,(UCHAR *)SqlState
		,&pfNativeError
		,(UCHAR *)ErrMsg
		,ErrMsgLen
		,&pcbErrorMsgLen
		);
    }

    if(rc==SQL_SUCCESS || rc==SQL_SUCCESS_WITH_INFO) {
	MsgPush("DBERROR:IFMX:%d:%s:%s",pfNativeError,SqlState,ErrMsg);
    } else if (rc== SQL_NO_DATA_FOUND ) {
	MsgPush("DBERROR:IFMX:?:?:Unknown error");
    } else {
	MsgPush("DBERROR:IFMX:?:?:Unknown error rc=%d",rc);
    }
    return;
}

static eBoolean_t
FreeStmt(SQLWEB_CURSOR *pCursor)
{
    RETCODE rc;

    rc=SQLFreeStmt( gaCDA[pCursor->iCursorIndex], SQL_CLOSE );
    if( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
	PushDBErr(gaLDA[pCursor->pLDA->iLdaIndex]
		 ,gaCDA[pCursor->iCursorIndex]
		 ,"oclose failed."
		 );
	return(eFalse);
    }
    gbaOpen[pCursor->iCursorIndex]=eFalse;
    return(eTrue);
}
