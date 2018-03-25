/* swbltin.c - built-in processing routines (if,cursor,symbol, etc)
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
/* #include <sys/wait.h>
 */

#include "sqlweb.h"

/*
/* Internal Private Functions....
 */
static eBoolean_t iCp2CookList(PI *pPI);
static eBoolean_t iCp2PIList(PI *pPI);
static eBoolean_t iCp2PIAList(SYMBOL *pPIA);
static eBoolean_t ChopPI(PI *pPI,eBoolean_t b);
/* Global Variables defined elsewhere...
 */
extern SQLWEB_LDA gLDA;
extern eBoolean_t gbBufferOutput;

/*
/* Global Variables used by all modules
/* but owned by this one!
 */

/* Global Variables Private to THIS FILE
 */
static LIST *glCookList
	   ,*glPIList
	   ,*glPIAList
	;


eBoolean_t
sqlweb_if(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char sSQL[MAX_TOKVAL_SIZE];
    char *pSQL, *pBuf, *pExpr;
    SQLWEB_CURSOR cda;

    *pbExpr = eFalse;
    if(!is_casematch(pPI->pTagName,"IF")
	&& !is_casematch(pPI->pTagName,"SQLWEB-IF"))
    {
	MsgPush("sqlweb_if:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }

    /*
    /* Process IF Tag
     */
    (*pbExpr) = eFalse;
    if(ISeTrue(GetPIAValueREF(pPI,"EXPR",&pExpr))) {
	if(ISeTrue(GetPIAValueREF(pPI,"EVAL",&pBuf))
	    && (is_casematch(pBuf,"TRUE")) )
	{
	    ExpandString(pExpr,&pBuf);
	    RemoveCharCodes(pBuf);
	    pExpr = pBuf;
	}

	sprintf(sSQL,"%s %s",gpIfExpr,pExpr);
	RETeFalse(DbOpenCursor(&gLDA,sSQL,&cda)
		 ,"DbOpenCursor Failed"
		 );
	RETeFalse(DbFetchCursor(&cda,eTrue) /* Close */
		 ,"DbFetchCursor Failed"
		 );
	(*pbExpr) = cda.bFound;

	ChopPI(pPI,(*pbExpr));
		/* Chop Children IFF expression is FALSE
		 */

    } /* Got the "Required EXPR" symbol... */
    return(eTrue);
}

eBoolean_t
sqlweb_cursor(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char *pSQL, *pBuf;
    SQLWEB_CURSOR cda;
    LIST *lBINDList;
    LIST *lTmpCookList;
    LIST *lTmpPIList;
    int  iLoopCount=0;

    *pbExpr = eFalse;

    if(!is_casematch(pPI->pTagName,"CURSOR")
	&&!is_casematch(pPI->pTagName,"SQLWEB-CURSOR"))
    {
	MsgPush("sqlweb_cursor:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }
    /*
    /* Process a CURSOR Tag
     */
    DebugHTML(__FILE__,__LINE__,4,"Got a CURSOR!");
    if(ISeFalse(GetPIAValueREF(pPI,"SQL",&pSQL))){
	pSQL= gpNullSelect;
    }

    if(ISeTrue(GetPIAValueREF(pPI,"EVAL",&pBuf))
	&&(is_casematch(pBuf,"TRUE")))
    {
	DebugHTML(__FILE__,__LINE__,2,"Opening SQL=\"%s\"",pSQL);
	ExpandString(pSQL,&pBuf);
	RemoveCharCodes(pBuf);
	pSQL = pBuf;
    }
    DebugHTML(__FILE__,__LINE__,2,"Opening SQL=\"%s\"",pSQL);

    RETeFalse(DbOpenCursor(&gLDA,pSQL,&cda)
	     ,"DbOpenCursor Failed"
	     );
    DebugHTML(__FILE__,__LINE__,4,"Its OPEN!");

    /*
    /* Save the CURSOR's PI List the lBINDList.
    /* and create a New, clean, consolidated list and
    /* setup a global pointer to the new list.
     */
    lTmpCookList = glCookList;
    lTmpPIList   = glPIList;

    lBINDList = pPI->lPI;
    pPI->lPI = l_create("QUEUE");
    glPIList  = pPI->lPI;

    /*
    /* Fetch LOOP
     */
    for(iLoopCount=0;;){
	/*
	/* Do the FETCH
	 */
	DebugHTML(__FILE__,__LINE__,4
		    ,"Attempting a FETCH(%d)!"
		    ,cda.iCursorIndex);
	RETeFalse(DbFetchCursor(&cda,eFalse)
		 ,"DbFetchCursor Failed"
		 );
	if(ISeFalse(cda.bFound)) {
	    DebugHTML(__FILE__,__LINE__,4,"Fetch Done.");
	    break;
	}
	iLoopCount++;

	DebugHTML(__FILE__,__LINE__,4,"Got a ROW, attempting LIST Dup");

	/* Step 1
	/* Duplicate the Origional LIST to BIND to the FETCHED ROW
	 */
	if( !(glCookList = l_create("QUEUE"))) {
	    MsgPush("l_create(lglookList) Failed");
	    return(eFalse);
	}
	RETeFalse(l_scan(lBINDList,(PFI)iCp2CookList)
		 ,"l_scan(iCp2CookList) Failed"
		 );

	/* Step 2
	/* Cook the Newly build glCookList built in the above step
	 */
	RETeFalse(l_scan(glCookList,(PFI)CookPI)
		 ,"CookPI Failed in w/in CURSOR"
		 );
	    
	if(ISeTrue(gbBufferOutput)){
	    /* Step 3
	    /* Add the newly Cooked Items to the pPI-lPI LIST
	    /* where it belongs.
	     */
	    RETeFalse(l_scan(glCookList,(PFI)iCp2PIList)
		     ,"l_scan(iCp2PIlist) Failed"
		     );
	}
	l_destroy(glCookList);
    }

    /* Chop "template" Children IFF no rows processed */
    if(iLoopCount==0) {
	ChopPI(pPI,eFalse);
    }

    /* Never Re-Cook Looped Stuff */
    *pbExpr = eFalse;

    glCookList = lTmpCookList;
    glPIList   = lTmpPIList;
    return(eTrue);
}

eBoolean_t
sqlweb_while(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char *pExpr, *pBuf;
    LIST *lBINDList;
    LIST *lTmpCookList;
    LIST *lTmpPIList;

    int iLoopCount=0;

    *pbExpr = eFalse;

    if(!is_casematch(pPI->pTagName,"WHILE")
	&&!is_casematch(pPI->pTagName,"SQLWEB-WHILE"))
    {
	MsgPush("sqlweb_while:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }
    /*
    /* Process a WHILE Tag
     */
    DebugHTML(__FILE__,__LINE__,3,"Got a WHILE!");

    if(ISeFalse(GetPIAValueREF(pPI,"EXPR",&pExpr))) {
	DebugHTML(__FILE__,__LINE__,0
		 ,"WARN: No EXPR for While on line %d"
		 ,pPI->iLineNbr
		 );
	MsgPush("sqlweb_while:No EXPR");
	return(eTrue);
    }
    if(ISeTrue(GetPIAValueREF(pPI,"EVAL",&pBuf))
	&&(is_casematch(pBuf,"TRUE")))
    {
	ExpandString(pExpr,&pBuf);
	RemoveCharCodes(pBuf);
	pExpr = pBuf;
    }

    /*
    /* Save the WHILE's PI List the lBINDList.
    /* and create a New, clean, consolidated list and
    /* setup a global pointer to the new list.
     */
    lTmpCookList = glCookList;
    lTmpPIList   = glPIList;

    lBINDList = pPI->lPI;
    pPI->lPI = l_create("QUEUE");
    glPIList  = pPI->lPI;

    /*
    /* Fetch LOOP
     */
    for(iLoopCount=0;;){
	DebugHTML(__FILE__,__LINE__,2,"While-Expr=\"%s\"",pExpr);
	/*
	/* Process EXPR
	 */
	RETeFalse2(ParseIf(pExpr,pbExpr)
		  ,"Failed to expand WHILE:%s"
		  ,pExpr
		  );

	DebugHTML(__FILE__,__LINE__,4,"While-Expr:Done:(%s %d Children)"
				    ,ISeFalse((*pbExpr))?"CHOP":"KEEP"
				    ,pPI->lPI? l_size(pPI->lPI): -1
				    );
	if(ISeFalse(*pbExpr)) {
	    DebugHTML(__FILE__,__LINE__,4,"While Done.");
	    break;
	}
	iLoopCount++;

	DebugHTML(__FILE__,__LINE__,4,"Got a TRUE, attempting LIST Dup");

	/* Step 1
	/* Duplicate the Origional LIST to BIND to the FETCHED ROW
	 */
	if( !(glCookList = l_create("QUEUE"))) {
	    MsgPush("l_create(lglookList) Failed");
	    return(eFalse);
	}
	RETeFalse(l_scan(lBINDList,(PFI)iCp2CookList)
		 ,"l_scan(iCp2CookList) Failed"
		 );

	/* Step 2
	/* Cook the Newly build glCookList built in the above step
	 */
	RETeFalse(l_scan(glCookList,(PFI)CookPI)
		 ,"CookPI Failed in w/in CURSOR"
		 );
	    
	if(ISeTrue(gbBufferOutput)){
	    /* Step 3
	    /* Add the newly Cooked Items to the pPI-lPI LIST
	    /* where it belongs.
	     */
	     RETeFalse(l_scan(glCookList,(PFI)iCp2PIList)
		     ,"l_scan(iCp2PIlist) Failed"
		     );
	}
	l_destroy(glCookList);
    }
    /* Chop "template" Children IFF no rows processed */
    if(iLoopCount==0) {
	ChopPI(pPI,eFalse);
    }

    /* Never Re-Cook Looped Stuff */
    *pbExpr = eFalse;

    glCookList = lTmpCookList;
    glPIList   = lTmpPIList;
    return(eTrue);
}

eBoolean_t
sqlweb_connect(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char *pBuf, *pName;

    *pbExpr = eFalse;
    if(!is_casematch(pPI->pTagName,"CONNECT")
	&&!is_casematch(pPI->pTagName,"SQLWEB-CONNECT"))
    {
	MsgPush("sqlweb_connect:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }

    if(ISeFalse(GetPIAValueREF(pPI,"NAME",&pName))) {
	if(ISeFalse(GetPIAValueREF(pPI,"SCHEMA",&pName))) {
	    /* If no name or schema; use default found in "ini" file 
	     */
	    RETeFalse(DbConnect(0,&gLDA),"Failed to CONNECT to Database");
	    return(eTrue);
	}
    }
    ExpandString(pName,&pBuf);
    RemoveCharCodes(pBuf);
    pName = pBuf;

    RETeFalse(DbConnect(pName,&gLDA),"Failed to CONNECT to Database");
    *pbExpr = eFalse;
    return(eTrue);
}

static eBoolean_t
iCp2CookList(
	 PI *pPI
	)
{
    PI *pPI2;
    LIST *lTmpList = glCookList;


    DebugHTML(__FILE__,__LINE__,5,"iCp2CookList(%s)", pPI->pTagName);

    pPI2 = NewPI();
    if(!pPI2){
	MsgPush("NewPI failed");
	return(eFalse);
    }
    /* (void)memset(pPI2,0,sizeof(PI));
     */

    pPI2->iLevel	= pPI->iLevel;
    pPI2->pTagName	= DupBuf(pPI->pTagName);
    pPI2->pPiContents	= DupBuf(pPI->pPiContents);

    pPI2->pTag		= pPI->pTag;
    pPI2->pPIContext	= pPI->pPIContext;
			/* Works only if pPIContext does NOT BIND
			 */
    pPI2->lPI		= l_create("QUEUE");
    pPI2->lPIA		= l_create("INORDER");	/* Searchable... */
    pPI2->iLineNbr	= pPI->iLineNbr;

    glPIAList = pPI2->lPIA;
    RETeFalse(l_scan(pPI->lPIA,(PFI)iCp2PIAList)
	     ,"Failed to Duplicate PIA List"
	     );

    /* Add self to current COOK List
     */
    RETeFalse(ENQ(glCookList,pPI2)
	     ,"Cp2CookList:ENQ Failed"
	     );
    glCookList = pPI2->lPI;
    RETeFalse(l_scan(pPI->lPI,(PFI)iCp2CookList)
	     ,"Failed to Duplicate Cooked List"
	     );
    glCookList = lTmpList;
    return(eTrue);
}
static eBoolean_t
iCp2PIList(PI *pPI)
{
    DebugHTML(__FILE__,__LINE__,4,"iCp2PIList(%s)", pPI->pTagName);
    RETeFalse(ENQ(glPIList,pPI)
	     ,"Cp2PIList:ENQ Failed"
	     );
    return(eTrue);
}
static eBoolean_t
iCp2PIAList(SYMBOL *pPIA)
{
    SYMBOL *pSym;
    pSym = NewPIA();
    if(!pSym){
	MsgPush("NewPIA failed");
	return(eFalse);
    }
    pSym->iType		= pPIA->iType;
    pSym->pName		= DupBuf(pPIA->pName);
    pSym->pValue	= DupBuf(pPIA->pValue);
    pSym->esDataType	= pPIA->esDataType;

    return(ENQ(glPIAList,pSym));
}

eBoolean_t
sqlweb_include(PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char sFileName[BUFSIZ];
    char *pFieldName
     	,*pExpandedName
     	,*pType
	;
    PAGE *pPage;

    *pbExpr = eTrue;
    if(!is_casematch(pPI->pTagName,"INCLUDE")
	&& !is_casematch(pPI->pTagName,"SQLWEB-INCLUDE"))
    {
	MsgPush("sqlweb_include:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }
    /*
    /* The INCLUDE Tag
     */

    /*
    /* Get the NAME Attribute
     */
    if(ISeFalse(GetPIAValueREF(pPI,"NAME",&pFieldName))){
	/* Get the FILENAME Attribute, for backward compat.
	 */
	if(ISeFalse(GetPIAValueREF(pPI,"FILENAME",&pFieldName))){
	    /* Get the PAGENAME Attribute, for backward compat.
	     */
	    if(ISeFalse(GetPIAValueREF(pPI,"PAGENAME",&pFieldName))){
		DebugHTML(__FILE__,__LINE__,0,"Nothing to INCLUDE.");
		return(eTrue);
	    } else {
		DebugHTML(__FILE__,__LINE__,0
		    ,"Warning, replace old PAGENAME attribute with FILENAME");
	    }
	}
    }
    ExpandString(pFieldName,&pExpandedName);

    /*
    /* Pagename starts with '/' then it's based on SQLWEB_DOC_ROOT
     */
    if(*pExpandedName=='/') {
	char *pIncRoot;
	if(ISeFalse(GetSymbolValueREF("SQLWEB_DOC_ROOT",&pIncRoot))) {
	    DebugHTML(__FILE__,__LINE__,0
		     ,"Warning: SQLWEB_DOC_ROOT not found in sqlweb.ini."
		     );
	    return(eTrue);
	}
	sprintf(sFileName,"%s%s",pIncRoot,pExpandedName);
    }
    /*
    /* else its relative to current file.
     */
    else {
	char *pDirName = strrchr(gpFileName,'/');
	if(pDirName) {
	    char *p = gpFileName;
	    int i;
	    for(i=0,p=gpFileName;(p<pDirName);i++,p++)
		sFileName[i]=*p;
	    sFileName[i++]='/';
	    sFileName[i++]=0;
	    strcat(sFileName,pExpandedName);
	} else {
	    sprintf(sFileName,"./%s",pExpandedName);
	}
    }

    /*
    /* Get the FILETYPE Attribute
     */
    if(ISeFalse(GetPIAValueREF(pPI,"TYPE",&pType))){
	if(ISeFalse(GetPIAValueREF(pPI,"FILETYPE",&pType))){
	    pType="SQLWEB";
	}
    }

    DebugHTML(__FILE__,__LINE__,3,"Loading: %s (type=%s)",sFileName,pType);
    if(is_casematch(pType,"SQLWEB")) {
	if(ISeFalse(LoadHTML(sFileName,&pPage))){
	    DebugHTML(__FILE__,__LINE__,0
		     ,"INCLUDE: Failed to load html: %s", sFileName
		     );
	    return(eTrue);
	}
	/*
	/* Now that the new page is loaded use its 
	/* PI-List as this PI's PI-List
	 */
	pPI->lPI = pPage->lPI;
	return(eTrue);
    }
    if(ISeFalse(LoadTEXT(sFileName,pType,pPI))){
	DebugHTML(__FILE__,__LINE__,0
		 ,"INCLUDE: Failed to load %s file: %s"
		 ,pType
		 ,sFileName
		 );
	return(eTrue);
    }
    return(eTrue);
}

/*
/* overrides the default IF processing depending on DB...
 */
eBoolean_t
sqlweb_if2(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char *pExpr, *pBuf;

    *pbExpr = eFalse;

    if(!is_casematch(pPI->pTagName,"IF2")
	&& !is_casematch(pPI->pTagName,"SQLWEB-IF2")
	&& !is_casematch(pPI->pTagName,"SQLWEB-SYMBOL")
	&& !is_casematch(pPI->pTagName,"SYMBOL"))
    {
	MsgPush("sqlweb_if2:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }

    /*
    /* Process IF2 Tag
     */
    (*pbExpr) = eFalse;

    if(ISeTrue(GetPIAValueREF(pPI,"EXPR",&pExpr))) {
	if(ISeTrue(GetPIAValueREF(pPI,"EVAL",&pBuf))
	    && (is_casematch(pBuf,"TRUE")) )
	{
	    ExpandString(pExpr,&pBuf);
	    RemoveCharCodes(pBuf);
	    pExpr = pBuf;
	}

	DebugHTML(__FILE__,__LINE__,3,"Expr=\"%s\"",pExpr);

	RETeFalse2(ParseIf(pExpr,pbExpr)
		  ,"Failed to expand IF:%s"
		  ,pExpr
		  );

	ChopPI(pPI,(*pbExpr));
		/* Chop Children IFF expression is FALSE
		 */

    } /* Got the "Required EXPR" symbol... */
    return(eTrue);
}

eBoolean_t
sqlweb_host(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    char sBuf[MAX_TOKVAL_SIZE]
	,*p;
    char *pCmd, *pOutput, *pBuf;
    int c, iCharCount, iStatus;
    FILE *pF;

    DebugHTML(__FILE__,__LINE__,3,"Host");
    *pbExpr = eTrue;

#ifdef WIN32
    MsgPush("SQLweb/NT: Doesn't Support HOST");
    return(eTrue);
#endif

#ifndef WIN32
    if(!is_casematch(pPI->pTagName,"HOST")
	&& !is_casematch(pPI->pTagName,"SQLWEB-HOST"))
    {
	MsgPush("sqlweb_host:Can't Process %s",pPI->pTagName);
	return(eFalse);
    }
    /*
    /* Get the CMD Attribute
     */
    if(ISeFalse(GetPIAValueREF(pPI,"CMD",&pCmd))){
	DebugHTML(__FILE__,__LINE__,1,"W:No \"CMD\" Attribute");
	return(eTrue);
    }
    ExpandString(pCmd,&pBuf);
    pCmd = pBuf;
    DebugHTML(__FILE__,__LINE__,3,"Running: (%s)",pCmd);

    /*
    /* Get the OUTPUT Attribute
     */
    if(ISeTrue(GetPIAValueREF(pPI,"OUTPUT",&pOutput))
	&& is_casematch(pOutput,"FALSE"))
    {
	*pbExpr = eFalse;
    }

    pF = popen(pCmd,"r");
    if(!pF){
	MsgPush("sqlweb_host:Can't popen(%s,\"r\")", pCmd);
	return(eFalse);
    }

    DebugHTML(__FILE__,__LINE__,3,"Open'd(%x)",pF);
    for( c=fgetc(pF), iCharCount=0, p=sBuf
	;c != EOF
	;c=fgetc(pF), iCharCount++ )
    {
	if(iCharCount<sizeof(sBuf))
	    *p++ = (char)c;
    }
    *p = 0;
    DebugHTML(__FILE__,__LINE__,3,"Read %d chars",iCharCount);
    iStatus = pclose(pF);
    DebugHTML(__FILE__,__LINE__,3,"popen:%d:%d",iStatus,c);

    if(sBuf[0]) {
	FreeBuf(pPI->pPiContents);
	pPI->pPiContents= DupBuf(sBuf);
	SELSym("SQLWEB_HOST_OUTPUT",pPI->pPiContents);
    }
    return(eTrue);
#endif
}

/*
/* This one is not yet ready for prime time-- it would be best
/* to run "CookPI" to an alternate buffer, and then run
/* LoadHTML on the alternate buffer to reparse the output
/* -- that would be a "read" EVAL
/*
/* This one just DON'T WORK!
 */
eBoolean_t
sqlweb_eval(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    eBoolean_t b = gbBufferOutput;

    DebugHTML(__FILE__,__LINE__,0,"EVAL:Pass1\n");
    gbBufferOutput=eTrue;
    if(ISeFalse(l_scan(pPI->lPI,(PFI)CookPI))) {
	MsgPush("EVAL: pass1 failed");
	return(eTrue);
    }

    DebugHTML(__FILE__,__LINE__,0,"EVAL:Pass2\n");
    gbBufferOutput=b;
    if(ISeFalse(l_scan(pPI->lPI,(PFI)CookPI))) {
	MsgPush("EVAL: pass1 failed");
	return(eTrue);
    }

    *pbExpr = eTrue;
    return(eTrue);
}

eBoolean_t
sqlweb_demo(
	 PI *pPI
	,eBoolean_t *pbExpr
	)
{
    pPI->pPiContents="This is a Demo, ::HOME = ':home', this is only a demo";
    *pbExpr = eTrue;
    return(eTrue);
}

static eBoolean_t
ChopPI(PI *pPI,eBoolean_t bExpr)
{
    DebugHTML(__FILE__,__LINE__,3,"ChopPI(%s:%s %d Children)"
	    ,pPI->pTagName
	    ,ISeFalse(bExpr)?"CHOP":"KEEP"
	    ,pPI->lPI? l_size(pPI->lPI): -1
	    );

    if(ISeFalse(bExpr)) {
	if(ISeTrue(gbBufferOutput)){
	    pPI->lPI = l_create("QUEUE");
	    pPI->pPiContents = DupBuf(0);
	} else {
	    FreePI(pPI);
	}
    }
    DebugHTML(__FILE__,__LINE__,5,"ChopPI:Done.");
    return(eTrue);
}
