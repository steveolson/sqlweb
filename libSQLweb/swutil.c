/* swutil.c - general purpose utilities for SQLweb
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

/* #define USEHASHTABLE
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
/* #include <time.h>
 */
#include <sys/stat.h>

#include "sqlweb.h"

/*
/* internal functions 
 */
static eBoolean_t GetSymbolREF(char *pName,SYMBOL **pSym);
static eBoolean_t GetARRSymbolREF(char *pName,int iIndex,SYMBOL **pSym);
static eBoolean_t iPrintSymbol(SYMBOL *pSym);
static eBoolean_t ClearHTMLBuf();
static void unescape_url(char *url);
static int CmpTags(TAG *pTag1, TAG *pTag2);
static int CmpTagName(TAG *pTag1, char *pTagName);
static eBoolean_t FreePI_opt(PI *pPI, eBoolean_t bFreelPI);
static eBoolean_t AddHTMLHeader(char *pHeader);
static eBoolean_t CalcFileSize
		(char *pFileName
		,long *plFileSize
		,eBoolean_t bEscapeFlag
		);

/*
/* Global variables
 */
static LIST *glSymbolTable;		/* Global Symbol Table */
static LIST *glMsg = NULL_LIST;		/* Error/Message Stack */
static LIST *glTagList = NULL_LIST;	/* The Tag List */
static char gsHTMLBuf[MAX_HTMLBUF_SIZE];/* The HTML Buffer */
static char *gpHTMLBuf = gsHTMLBuf;	/* Pointer into HTML Buffer */
static int giHTMLDebugLevel=0;		/* Debugging Level */

static LIST *glPIFreeList = NULL_LIST;	/* List of Free'd PI's */
static LIST *glPIAFreeList =NULL_LIST;	/* List of Free'd PIA's */
static char *gpEmptyString = "";	/* Empty String for DupBuf */

static LIST *glHeaderList = NULL_LIST;	/* List of HTML Headers */

eBoolean_t
PrintHTMLErrStack()
{
    char sBuf[MAX_BUFSIZ], *pBuf;
    SYMBOL *pSym;
    char *pErrHeader = "<H1>SQLweb Error</H1>";

    PrintHTMLHeader("text/html");
    if(ISeTrue(GetSymbolValueREF("SQLWEB_ERROR_HEADER",&pBuf))) {
	pErrHeader=pBuf;
    }
    OutputHTML("%s\n", pErrHeader);
/*  if(ISeTrue(GetSymbolREF("SQLWEB_MESSAGE",&pSym))){
/*	OutputHTML("Last SQLWEB_MESSAGE: %s<BR>\n",pSym->pValue);
/*  } */
    while( MsgPop(sBuf) ) {
	OutputHTML("%s<BR>\n",sBuf);
    }
    FlushHTMLBuf();
    return(eTrue);
}

eBoolean_t
BuildSymbolTable(char *pBuf)
{
    char *pName
	,*pEndName
	,*pValue
	,*pEndValue
    ;

    /*
    /* Load Entire Environment into SYMBOL Table
     */
    for(pName=pBuf;pName; pName = pEndValue? pEndValue+1 :0){
	pEndName=strchr(pName,'=');
	if(pEndName) {
	    *pEndName=0;
	    pValue = pEndName+1;
	    pEndValue=strchr(pValue,'&');
	    if(pEndValue) {
		*pEndValue=0;
	    }

	    unescape_url(pName);
	    unescape_url(pValue);

	    FRMSym(DupBuf(pName),DupBuf(pValue));

	}
    }
    return(eTrue);
}

eBoolean_t
GetSymbolValue
    (char *pName
    ,char *pOutValue
    )
{
    SYMBOL *pSym;
    DebugHTML(__FILE__,__LINE__,6,"GetSymbolValue(%s)",pName);
    if(ISeTrue(GetSymbolREF(pName,&pSym))){
	strcpy(pOutValue,pSym->pValue);
	return(eTrue);
    }
    *pOutValue = 0;
    return(eFalse);
}

eBoolean_t
GetSymbolValueREF
    (char *pName
    ,char **pOutValue
    )
{
    SYMBOL *pSym;
    DebugHTML(__FILE__,__LINE__,6,"GetSymbolValueREF(%s)",pName);
    if(ISeTrue(GetSymbolREF(pName,&pSym))){
	(*pOutValue) = pSym->pValue;
	return(eTrue);
    }
    (*pOutValue) = 0;
    return(eFalse);
}

eBoolean_t
GetRawSymbolValueREF
    (char *pName
    ,char **pOutValue
    ,long *lSize
    )
{
    SYMBOL *pSym;
    DebugHTML(__FILE__,__LINE__,6,"GetRawSymbolValueREF(%s)",pName);
    if(ISeTrue(GetSymbolREF(pName,&pSym))){
	(*pOutValue) = pSym->pValue;
	(*lSize)     = pSym->lSize;
	return(eTrue);
    }
    (*pOutValue) = 0;
    (*lSize) = 0;
    return(eFalse);
}

eBoolean_t
GetARRSymbolValueREF
    (char *pName
    ,int iIndex
    ,char **pOutValue
    )
{
    SYMBOL *pSym;
    DebugHTML(__FILE__,__LINE__,6,"GetARRSymbolValueREF(%s,%d)",pName,iIndex);
    if(ISeTrue(GetARRSymbolREF(pName,iIndex,&pSym))){
	(*pOutValue) = pSym->pValue;
	return(eTrue);
    }
    (*pOutValue) = 0;
    return(eFalse);
}

static eBoolean_t
GetSymbolREF
    (char *pName
    ,SYMBOL **pSym
    )
{
    SYMBOL Sym;		/* HASH Table restriction... */
    Sym.pName = pName;	/* only search by struct.. */

    (*pSym)= l_find(glSymbolTable,GetSymbolName,&Sym);
    if(*pSym) {
	return(eTrue);
    }
    return(eFalse);
}

static eBoolean_t
GetARRSymbolREF
    (char *pName
    ,int iIndex
    ,SYMBOL **pSym
    )
{
    SYMBOL Sym;		/* HASH Table restriction... */
    Sym.pName = pName;	/* only search by struct.. */

    for(  (*pSym)= l_find(glSymbolTable,GetSymbolName,&Sym)
	; (*pSym) && (iIndex-->0)
	; (*pSym)=l_fnext(glSymbolTable))
    {
	if((*pSym) && !is_casematch((*pSym)->pName,pName)) {
	    *pSym=0;
	    return(eFalse);
	}
    }
    if(*pSym && is_casematch((*pSym)->pName,pName)) {
	return(eTrue);
    }
    *pSym=0;
    return(eFalse);
}

eBoolean_t
IsSymbolValue
    (char *pName
    ,char *pValue
    )
{
    SYMBOL Sym,*pSym;
    DebugHTML(__FILE__,__LINE__,6,"IsSymbolValue(%s,%s)",pName,pValue);

    Sym.pName = pName;
    Sym.pValue = pValue;
    for( pSym=l_find(glSymbolTable,GetSymbolName,&Sym);
	 pSym && is_casematch(pSym->pName,pName);
	 pSym=l_fnext(glSymbolTable))
    {
	if(is_match(pSym->pValue,pValue))
	    return(eTrue);
    }
    return(eFalse);
}

/*
/* Supports the HASH function requirements
/* returns the "HASH" key string for the struct.
 */
#ifdef USEHASHTABLE
char *
GetSymbolName(SYMBOL *pSym)
{
    /* fprintf(stderr,"KEY(%s)\n",pSym->pName);
     */
    return(pSym->pName);
}
#else
int
GetSymbolName(SYMBOL *pSym,SYMBOL *p2)
    { return(iStrCaseCmp(pSym->pName,p2->pName)); }
#endif


/*
/* Compare two symbols, supports LIST functions
 */
int
iCmpSymbolNames(SYMBOL *pSym1, SYMBOL *pSym2)
{
    return( iStrCaseCmp( pSym1->pName, pSym2->pName) );
}

/*
/* Compare a SYMBOL to a "Symbol Name"
 */
int
iCmpSymbolpName(SYMBOL *pSym, char *pName)
{
    return(iStrCaseCmp(pSym->pName,pName));
}

static eBoolean_t
iPrintSymbol(SYMBOL *pSym)
{
    OutputHTML("<!%s=\"%s\">\n",pSym->pName,pSym->pValue);
    return(eTrue);
}

eBoolean_t
iPrintHTMLSymbol(SYMBOL *pSym)
{
    char sBuf[MAX_TOKVAL_SIZE]
	,*pBuf
	;
    if(pSym->pValue && iStrLen(pSym->pValue)>0) {
	char *pValue = pSym->pValue;
	pBuf = sBuf;
	while(*pValue && (pBuf-sBuf<sizeof(sBuf)) ){
	    if(*pValue == '<') {
		strcpy(pBuf,"&lt;");
		pBuf += 4;
		++pValue;
	    } else if(*pValue == '>') {
		strcpy(pBuf,"&gt;");
		pBuf += 4;
		++pValue;
	    } else {
		*pBuf++ = *pValue++;
	    }
	}
	*pBuf=0;
	OutputHTML(" %s=\"%s\"", pSym->pName, sBuf);
    }
    else
	OutputHTML(" %s", pSym->pName);
    return(eTrue);
}

char
x2c(char *what)
{
    register char digit;

    digit  =(char)(what[0] >= 'A' ?((what[0] & 0xdf)-'A')+10 :(what[0]-'0'));
    digit *=16;
    digit +=(char)(what[1] >= 'A' ?((what[1] & 0xdf)-'A')+10 :(what[1]-'0'));
    return(digit);
}

void
unescape_url(char *url)
{
    register int x,y;

    for(x=0,y=0;url[y];++x,++y) {
	if(url[y]=='+') {
	    url[y]=' ';
	}

        if((url[x] = url[y]) == '%') {
            url[x] = x2c(&url[y+1]);
            y+=2;
        }
    }
    url[x] = '\0';
    return;
}

/*
/* Push Message onto the Error Stack for
/* later display to the end user.
 */
void
MsgPush
    (
    const char *pFmt
    , ...
    )
{
    va_list pParms;
    register char *pMsg;
    char sBuf[BUFSIZ];

    /* Build PrintF style buffer
     */
    va_start(pParms,pFmt);
    vsprintf(sBuf,pFmt,pParms);

    if(glMsg==NULL_LIST) {
        if((glMsg=l_create("QUEUE"))==NULL_LIST) {
	   DebugHTML(__FILE__,__LINE__,0,"MsgPush:Failed:%s",sBuf);
	   /* fprintf(stderr,"MsgStack:Push:l_create Failed,%s\n",sBuf);
	   /* fflush(stderr);
	    */
           return; /* (eFalse) */
	}
    }


    /*
    /* Store message in list 
     */
    pMsg=DupBuf(sBuf);

    if(!ENQ(glMsg,pMsg) ) {
	return; /* (eFalse); */
    }
    return; /* (eTrue); */
}

/*
/* Pop Message off the Error Stack to
/* display to the end user.
 */
eBoolean_t
MsgPop(char *pOutMsgBuf )
{
    char *pMsg;

    DebugHTML(__FILE__,__LINE__,9,"MsgPop:%x:%d\n",glMsg,l_size(glMsg));
    if(!glMsg) {
	*pOutMsgBuf=0;
	return(eFalse);
    }

    pMsg=DEQ(glMsg);
    if(!pMsg) {
	*pOutMsgBuf=0;
	return(eFalse);
    }
    strcpy(pOutMsgBuf, pMsg);
    /* free(pMsg);
     */

    return(eTrue);
}

eBoolean_t
AddSymbol
    (int iType
    ,char *pName
    ,char *pValue
    ,eBoolean_t bReplace
    ,long lSize
    )
{
    SYMBOL *pSym;

    /*
    /* Replace Symbol (if bReplace FLAG && if found)
     */
    if(ISeTrue(bReplace) && ISeTrue(GetSymbolREF(pName,&pSym))){
	/*
	fprintf(stderr,"Replace:%s:%s=%s\n",pName,pSym->pValue,pValue);
	fflush(stderr);
	 */
	if((iType!=ENV_SYMBOL && iType!=INI_SYMBOL)
	    && (pSym->iType == ENV_SYMBOL || pSym->iType == INI_SYMBOL))
	{
	    DebugHTML(__FILE__,__LINE__,0
		     ,"AddSymbol:Attempted to overwrite CGI or INI Symbol:%s"
		     ,pName
		     );
	} else {
	    pSym->pValue = pValue;
	    pSym->lSize = lSize;
	}
	return(eTrue);
    }

    /* Duplicate or NOT FOUND
     */
    DebugHTML(__FILE__,__LINE__,5,"Calling NewPIA(%s)",pName);
    pSym = NewPIA();

    if(!pSym){
	MsgPush("NewPIA Failed in AddSymbol");
	return(eFalse);
    }
    /* (void)memset((pSym),0,sizeof(SYMBOL));
     */

    pSym->iType = iType;
    pSym->pValue= pValue;
    pSym->pName = DupBuf(pName);
    pSym->lSize = lSize;

    if( glSymbolTable==NULL_LIST) {
#	ifdef USEHASHTABLE
	    glSymbolTable=l_create("HASH");
#	else
	    glSymbolTable=l_create("ASCEND");
#	endif
	if(glSymbolTable==NULL_LIST){
	    MsgPush("l_create Failed in AddSymbol");
	    return(eFalse);
	}
    }

    RETeFalse(l_insert(glSymbolTable,GetSymbolName,pSym)
	     ,"l_insert Failed in AddSymbol"
	     );
    return(eTrue);
}

eBoolean_t
OutputHTML
    (
    const char *pFmt
    ,...
    )
{
    va_list pParms;
    unsigned int iBufLen;
    char sBuf[MAX_BUFSIZ];
    /*
    /* Build PrintF style buffer
     */
    va_start(pParms,pFmt);
    
    iBufLen = vsprintf(sBuf,pFmt,pParms);
    if(iBufLen>sizeof(gsHTMLBuf)) {
	MsgPush("OutputHTML:Buffer Overflow");
	return(eFalse);
    }
    if(iBufLen>(sizeof(gsHTMLBuf)-(gpHTMLBuf-gsHTMLBuf))) {
	FlushHTMLBuf();
    }
    strcpy(gpHTMLBuf,sBuf);
    gpHTMLBuf += iBufLen;
    return(eTrue);
}

static eBoolean_t
ClearHTMLBuf()
{
    gpHTMLBuf=gsHTMLBuf;
    *gpHTMLBuf=0;
    return(eTrue);
}

eBoolean_t
FlushHTMLBuf()
{
    /* First ensure the text/html header has been issued.
     */
    PrintHTMLHeader("text/html");

    fprintf(stdout,"%s",gsHTMLBuf);
    fflush(stdout);

    return( ClearHTMLBuf() );
}

eBoolean_t
PrintHTMLHeader(char *pContentType)
{
    static int iHeader;
    if( iHeader==0) {
	char *p;
	if(ISCOOKED){
	    fprintf(stdout,"Content-type: %s\n",pContentType);
	    fprintf(stdout,"Cache-Control: no-cache\n");
	    while( (p=POP(glHeaderList)) ) {
		fprintf(stdout,"%s\n",p);
	    }
	/*     fprintf(stdout,"Date: %s\n",sNow);
	/*     fprintf(stdout,"Expires: %s\n",sNow);
	 */
	    fprintf(stdout,"\n");
	    fflush(stdout);
	} else {
	    fprintf(stdout
		   ,"<!doctype html public \"-//IETF//DTD HTML//EN\">\n"
		   );
/*	    fprintf(stdout,"<!Copyright (c) 1995 Applied Information Technologies, Inc.>\n");
/*	    fprintf(stdout,"<!\tAll Rights Reserved.>\n");
 */
	}
	fflush (stdout);
    }
    iHeader=1;
    return(eTrue);
}

eBoolean_t
DebugHTMLSet(int iDebugLevel)
{
    giHTMLDebugLevel=iDebugLevel;
    return(eTrue);
}

int
DebugHTMLGet()
{
    return(giHTMLDebugLevel);
}

eBoolean_t
DebugHTML
    (char *pFileName
    ,int iLine
    ,int iDebugLevel
    ,char *pFmt
    ,...
    )
{
    va_list pParms;
    char sBuf[MAX_BUFSIZ];

    if(iDebugLevel>giHTMLDebugLevel)
	return(eTrue);

    /* Build PrintF style buffer
     */
    va_start(pParms,pFmt);
    vsprintf(sBuf,pFmt,pParms);

    OutputHTML("<! %8.8s:%5.5d:%1.1d:%s>\n"
	,pFileName
	,iLine
	,iDebugLevel
	,sBuf
	);
    FlushHTMLBuf();

    return(eTrue);
}

eBoolean_t
ShowSymbolTable()
{
    return(l_scan(glSymbolTable,(PFI)iPrintSymbol));
}

static int
CmpTags(TAG *pTag1, TAG *pTag2)
{
    return(iStrCmp(pTag1->pTagName,pTag2->pTagName));
}
static int
CmpTagName(TAG *pTag1, char *pTagName)
{
    return(iStrCaseCmp(pTag1->pTagName,pTagName));
}

eBoolean_t
GetTAGByName
    (char *pTagName
    ,TAG **pTAG
    )
{
    (*pTAG) = l_find(glTagList,CmpTagName,pTagName);
    if(!(*pTAG)) {
	/* DebugHTML(__FILE__,__LINE__,2,"GetTAGByName(%s):DFLT",pTagName);
	 */
	(*pTAG) = (TAG*)malloc(sizeof(TAG));
	if( !(*pTAG) ) {
	    MsgPush("malloc failed");
	    return(eFalse);
	}
	(void)memset((*pTAG),0,sizeof(TAG));
	(*pTAG)->pTagName = DupBuf(pTagName);
	(*pTAG)->pTagEmptyInd = "N";
	(*pTAG)->pTagDesc = 0;
	(*pTAG)->pTagAfterInd = "N";
	(*pTAG)->pTagLinkDesc = "";

	/* Since, we've got a "DEFAULT" Tag, there is no need to increase
	/* the size of the list (it'll just slow down subsequent lookups.
	/*
	 */
	l_insert(glTagList,CmpTags,(*pTAG));
	return(eTrue);
    }
    /* else {
    /*	DebugHTML(__FILE__,__LINE__,2,"GetTAGByName(%s):FOUND",pTagName);
    /* }
     */
    return(eTrue);
}

/* Load a Tag given a line like so:
/* NAME | EMPTY_IND | AFTER_IND | LINK_DESC |
 */
eBoolean_t
LoadTag(char *pTagData)
{
    char *pStart, *pSep, SEP_CHAR='|';
    TAG *pTag;

    pTag = (TAG*)malloc(sizeof(TAG));
    if(!pTag){
	MsgPush("malloc failed");
	return(eFalse);
    }
    memset(pTag,0,sizeof(TAG));

    pStart = pTagData;
    pSep=strchr(pTagData,SEP_CHAR);
    if(pSep) *pSep++=0 ;
    pTag->pTagName = DupBuf(pStart);

    pStart = pSep;
    pSep=strchr(pSep,SEP_CHAR);
    if(pSep) *pSep++ = 0;
    pTag->pTagEmptyInd = DupBuf(pStart);

    pStart = pSep;
    pSep=strchr(pSep,SEP_CHAR);
    if(pSep) *pSep++ = 0;
    pTag->pTagAfterInd = DupBuf(pStart);

    pStart = pSep;
    pSep=strchr(pSep,SEP_CHAR);
    if(pSep) *pSep++ = 0;
    pTag->pTagLinkDesc = DupBuf(pStart);

    if(!glTagList) {
	glTagList = l_create("BTREE");
    }
/*
/*    fprintf(stderr,"T=%s,A=%s,e=%s\n"
/*	   ,pTag->pTagName
/*	   ,pTag->pTagAfterInd
/*	   ,pTag->pTagEmptyInd
/*	   );
 */
    l_insert(glTagList,CmpTags,pTag);
    return(eTrue);
}

void
LogMsg
    (
    const char *pFmt
    ,...
    )
{
    va_list pParms;
    char sBuf[MAX_BUFSIZ]
	,*pFilename;
    static FILE *pLogFile;
    if(pLogFile== (FILE*)0) {
	if(ISeTrue(GetSymbolValueREF("SQLWEB_LOGFILE",&pFilename))) {
	    pLogFile=fopen(pFilename,"a");
	    if(!pLogFile)
	       return; /* (Failed) */
	}
    }
    /* Build PrintF style buffer
     */
    va_start(pParms,pFmt);
    vsprintf(sBuf,pFmt,pParms);
    fprintf(pLogFile,"%s\n",sBuf);
    fflush(pLogFile);
    return;
}

void *
DupMem(const char *pMem, long lSize)
{
    char *p;
    if(pMem && lSize>0) {
	p = malloc(lSize);
	if(p){
	    (void)memset(p,0,lSize);
	    memcpy(p,pMem,lSize);
	    return(p);
	}
    }
    DebugHTML(__FILE__,__LINE__,5,"DupMem(EmptyString)");
    return(gpEmptyString);
}

char *
DupBuf(const char *pBuf)
{
    if(pBuf && iStrLen(pBuf)>0 ) {
	DebugHTML(__FILE__,__LINE__,5,"DupBuf(%s)",pBuf);
	return( strdup(pBuf) );
    }
    DebugHTML(__FILE__,__LINE__,5,"DupBuf(**Null**,[%x])",pBuf);
    return(gpEmptyString);
}

void
FreeBuf(char *pBuf)
{
    if(pBuf && pBuf!=gpEmptyString)
	free(pBuf);
    return;
}

PI *
NewPI()
{
    PI *pPI;
    if(!glPIFreeList){
	glPIFreeList=l_create("QUEUE");
	if(!glPIFreeList) {
	    MsgPush("NewPI:l_create failed");
	    return(0);
	}
    }
    if(l_size(glPIFreeList)>0) {
	DebugHTML(__FILE__,__LINE__,5,"NewPI:POP(%d)",l_size(glPIFreeList));
	pPI=(PI*)POP(glPIFreeList);
    } else {
	DebugHTML(__FILE__,__LINE__,5,"NewPI:Malloc");
	pPI=(PI*)malloc(sizeof(PI));
    }
    if(!pPI) {
	MsgPush("NewPI:malloc or POP failed");
	return(0);
    }
    (void)memset(pPI,0,sizeof(PI));
    return(pPI);
}


eBoolean_t
FreePIr(PI *pPI)
{
    return( FreePI_opt(pPI,eTrue) );
}

eBoolean_t
FreePI(PI *pPI)
{
    return( FreePI_opt(pPI,eFalse) );
}

static eBoolean_t
FreePI_opt(PI *pPI, eBoolean_t bFreelPI)
{
    DebugHTML(__FILE__,__LINE__,5,"FreePI[%x](%s,%d)"
		,pPI
		,pPI->pTagName
		,pPI->iLineNbr
		);
    if(!glPIFreeList){
	glPIFreeList=l_create("QUEUE");
	if(!glPIFreeList) {
	    MsgPush("FreePI:l_create failed");
	    return(eFalse);
	}
    }

    /*
    /* Free Attributes
     */
    RETeFalse(l_scan(pPI->lPIA,(PFI)FreePIA)
	     ,"FreePI:l_scan(PIA) Failed"
	     );
    l_destroy(pPI->lPIA);

    /* DON'T Call Recursively to l_scan(lPI,FreePI)
    /* Because each PI Free's itself before returning
     */
    if(ISeTrue(bFreelPI)) {
	l_scan(pPI->lPI,(PFI)FreePIr);
    }
    l_destroy(pPI->lPI);

    FreeBuf(pPI->pTagName);
    FreeBuf(pPI->pPiContents);

    /*
    /* Store This PI on the FreeLIst
     */
    DebugHTML(__FILE__,__LINE__,5,"FreePI:PUSH(%d)",l_size(glPIFreeList));

    (void)memset(pPI,0,sizeof(PI));
    PUSH(glPIFreeList,pPI);

    return(eTrue);
}

SYMBOL *
NewPIA()
{
    SYMBOL *pPIA;
    if(!glPIAFreeList){
	glPIAFreeList=l_create("QUEUE");
	if(!glPIAFreeList) {
	    MsgPush("NewPI:l_create failed");
	    return(0);
	}
    }
    if(l_size(glPIAFreeList)>0) {
	DebugHTML(__FILE__,__LINE__,5,"NewPIA:POP(%d)",l_size(glPIAFreeList));
	pPIA=(SYMBOL*)POP(glPIAFreeList);
    } else {
	DebugHTML(__FILE__,__LINE__,5,"NewPIA:Malloc",l_size(glPIAFreeList));
	pPIA=(SYMBOL*)malloc(sizeof(SYMBOL));
    }
    if(!pPIA) {
	MsgPush("NewPIA:malloc or POP failed");
	return(0);
    }
    (void)memset(pPIA,0,sizeof(SYMBOL));
    return(pPIA);
}

eBoolean_t
FreePIA(SYMBOL *pPIA)
{
    if(!glPIAFreeList){
	glPIAFreeList=l_create("QUEUE");
	if(!glPIAFreeList) {
	    MsgPush("NewPI:l_create failed");
	    return(eFalse);
	}
    }
	
    FreeBuf(pPIA->pName);
    FreeBuf(pPIA->pValue);
    DebugHTML(__FILE__,__LINE__,5,"FreePIA:PUSH(%d)",l_size(glPIAFreeList));
    (void)memset(pPIA,0,sizeof(SYMBOL));
    PUSH(glPIAFreeList,pPIA);
    return(eTrue);
}


static eBoolean_t
AddHTMLHeader(char *pHeader)
{

    if(glHeaderList==NULL_LIST) {
        if((glHeaderList=l_create("QUEUE"))==NULL_LIST) {
	   DebugHTML(__FILE__,__LINE__,0,"MsgPush:Failed:%s",pHeader);
           return(eFalse);
	}
    }
    PUSH(glHeaderList,DupBuf(pHeader));
    return(eTrue);
}
eBoolean_t
LoadTEXT(char *pFileName
	,char *pFileType
	,PI *pPI
	)
{
    long lContentLength = 0;
    int c;
    eBoolean_t bEscapeFlag;
    FILE *pFile;
    char *pContents, *p;

    bEscapeFlag = eFalse;
    if(is_casematch(pFileType,"TEXT")) {
	bEscapeFlag = eTrue;
    }
    if(ISeFalse(CalcFileSize(pFileName, &lContentLength, bEscapeFlag))) {
	DebugHTML(__FILE__,__LINE__,0,"Failed to CalcFileSize %s",pFileName);
	return(eFalse);
    }
    DebugHTML(__FILE__,__LINE__,3,"FileSize:%s:%d",pFileName,lContentLength);
    pContents = malloc( lContentLength +1 );
    if( !pContents ){
	DebugHTML(__FILE__,__LINE__,0
		,"LoadTEXT: Failed to malloc(%d) for %s"
		,lContentLength
		,pFileName
		);
	return(eFalse);
    }
    (void)memset(pContents,0,lContentLength+1);
    if( (FILE *)0 == (pFile = fopen(pFileName,"r")) ) {
	DebugHTML(__FILE__,__LINE__,0
		     ,"LoadTEXT: Failed to open: %s", pFileName
		     );
	return(eFalse);
    }

    /* lContentLength = read ( iFile, pContents, lContentLength );
     */
    p = pContents;
    while( EOF != (c=getc(pFile)) ) {
	if(ISeTrue(bEscapeFlag)){
	    switch(c){
		case '<': /* &lt; */
		    strcpy(p,"&lt;");
		    p += 4;
		    break;
		case '>': /* &gt; */
		    strcpy(p,"&gt;");
		    p += 4;
		    break;
		case '&': /* &amp; */
		    strcpy(p,"&amp;");
		    p += 5;
		    break;
		default:
		    *p++ = c;
	    }
	} else {
	    *p++ = c;
	}
    }
    *p=0;
    fclose(pFile);

    pPI->pPiContents=pContents;
    return(eTrue);

/*
	DebugHTML(__FILE__,__LINE__,0
		 ,"INCLUDE: Unknown FILETYPE(%s) failed to load text file: %s"
		,sFileType
		,sFileName
	     );
 */
}

static eBoolean_t
CalcFileSize(
	 char *pFileName
	,long *plFileSize
	,eBoolean_t bEscapeFlag
	)
{
    FILE *pF;
    struct stat StatBuf;
    int c;

    (*plFileSize)=0;	/* firstly, init "out" variable */

    if(ISeFalse(bEscapeFlag)) {
	if( -1 == stat(pFileName,&StatBuf)) {
	    DebugHTML(__FILE__,__LINE__,0
			 ,"CalfFileSize: Failed to stat: %s", pFileName
			 );
	    return(eFalse);
	}
	(*plFileSize) = StatBuf.st_size;
	return(eTrue);
    }

    pF = fopen(pFileName,"r");
    if(!pF){
	DebugHTML(__FILE__,__LINE__,0
		     ,"CalcFileSize: Failed to open: %s", pFileName
		     );
	return(eFalse);
    }
    while( EOF != (c=getc(pF)) ){
	switch(c){
	    case '<': /* &lt; */
	    case '>': /* &gt; */
		(*plFileSize) += 4;
		break;
	    case '&': /* &amp; */
		(*plFileSize) += 5;
		break;
	    default:
		(*plFileSize)++;
	}
    }
    fclose(pF);
    return(eTrue);
}

#define EXPAND_CHUNK_SIZE 64
eBoolean_t
ExpandString(char *pIn, char **pRet)
{
    char sName[MAX_TOK_SIZE]
	,*pValue
	;
    char  *pOut=0
	 ,*pTmp
	;
    unsigned long ulOutSize = EXPAND_CHUNK_SIZE;
    eBoolean_t bExpand = eTrue;
    int iExpandCount=1;

    DebugHTML(__FILE__,__LINE__,5
	    ,"ExpandString(%x:%s)"
	    ,pIn
	    ,(pIn && *pIn)?pIn:"Huh?"
	    );

    if(!pIn || !(*pIn)) {
	DebugHTML(__FILE__,__LINE__,5,"ExpandString:empty");
	(*pRet) = DupBuf(0);
	return(eTrue);
    }

    /*
    /* Allocate a Buffer to hold Processed Output
     */
    pOut = (*pRet) = (char*)malloc(ulOutSize+1);
    if(!(*pRet)) {
	MsgPush("malloc failed");
	return(eFalse);
    }
    (void)memset((*pRet),0,ulOutSize+1);

    /* Don't automatically expand the initial buffer */
    bExpand = eFalse;

    while(*pIn) {

	/* Increase string size in increments of EXPAND_CHUNK_SIZE */
	if( ISeTrue(bExpand) || (pOut-(*pRet)) >= ulOutSize ){
	    iExpandCount++;

	    DebugHTML(__FILE__,__LINE__,3
		,"ExpandString:expanding string(F=%s:pos=%d,max=%d)"
		,ISeTrue(bExpand)?"TRUE":"FALSE"
		,(pOut-(*pRet))
		,ulOutSize
		);

	    bExpand=eFalse;
	    if(pOut) *pOut=0;
	    ulOutSize += (EXPAND_CHUNK_SIZE * iExpandCount);
	    (*pRet) = realloc((*pRet),ulOutSize+1);
	    if(!(*pRet)) {
		MsgPush("realloc failed");
		return(eFalse);
	    }
	    pOut =(*pRet) +((*pRet)? iStrLen(*pRet):0);
	}
	switch((int)*pIn){
	    char *pColon;
	    case ':':
	        pColon = pIn;
		if( isalpha(*(pIn+1)) ){
		    char *pTmp = sName;
		    while( isalnum(*pTmp = *(++pIn)) || *pTmp=='_' )
			++pTmp;
		    *pTmp=0;

		    if(ISeTrue(GetSymbolValueREF(sName,&pValue))) {
			DebugHTML(__FILE__,__LINE__,4
				,"ExpandString(%s=%s):pOut=%s,Pos=%d,VLen=%d"
				,sName
				,pValue
				,pOut
				,pOut-(*pRet)
				,(iStrLen(pValue)+1)
				);
			if((iStrLen(pValue)+1) > (ulOutSize-(pOut-(*pRet))))
			{
			    bExpand=eTrue;
			    pIn = pColon;
			    continue;
			}

			/* This implements simple, one-level
			/* symbol name expansion
			 */
			strcpy(pOut,pValue);
			pOut += iStrLen(pValue);

			/* Either way, continue with next element in loop
			 */
			continue;
		    }
		    DebugHTML(__FILE__,__LINE__,3,"NoSym(%s)",sName);
		    *pOut++=':';
		    strcpy(pOut,sName);
		    pOut += iStrLen(sName);
		    continue;
		}
		if( (*(pIn+1))==':' ){
		    pIn++;	/* Skip it */
		}
		*pOut++ = *pIn++;
		continue;
	    default :
		*pOut++ = *pIn++;
		break;
	}
    }

    *pOut=0;
    pTmp = (*pRet);
    (*pRet) = DupBuf(pTmp);
    free(pTmp);

    return(eTrue);
}

/*
/* Also, replace any non-standard HTML character
/* with its CODE %34, or whatever....
 */
eBoolean_t
ExpandContents(PI *pPI)
{
    char *pPiContents=0;

    DebugHTML(__FILE__,__LINE__,6
	    ,"ExpandContents:(%x:%x(%s=%s)):t=%s;l=%d"
	    ,pPI
	    ,pPI->pPiContents
	    ,pPI->pPiContents?pPI->pPiContents:"Huh?"
	    ,pPI->pTagName
	    ,pPI->pTag->pTagName
	    ,pPI->iLineNbr
	    );

    if(!pPI->pPiContents|| !(*pPI->pPiContents)) {
	DebugHTML(__FILE__,__LINE__,3
		,"ExpandContents:empty(%x):t=%s==%s;l=%d"
		,pPI->pPiContents
		,pPI->pTagName
		,pPI->pTag->pTagName
		,pPI->iLineNbr
		);
	pPI->pPiContents = DupBuf(0);
	return(eTrue);
    }

    RETeFalse(ExpandString(pPI->pPiContents,&pPiContents)
	     ,"ExpandString Failed in ExpandContents"
	     );
    pPI->pPiContents = pPiContents;
    return(eTrue);
}

/* eBoolean_t
/* RemoveCharCodes(char *pIn, char *pOut)
/* {
/*     if(!pIn) {
/* 	/* Make sure pout exists before writing to it
/* 	 */
/* 	if(pOut) {
/* 	    *pOut=0;
/* 	    return(eTrue);
/* 	}
/* 	return(eFalse);
/*     }
/* 
/*     while(*pIn){
/* 	if(*pIn=='&'){
/* 	    if(strncasecmp("&GT;",pIn,4)==0){
/* 		*pOut++ = '>';
/* 		pIn += 4;
/* 	    }
/* 	    else if(strncasecmp("&LT;",pIn,4)==0){
/* 		*pOut++ = '<';
/* 		pIn += 4;
/* 	    }
/* 	    else if(strncasecmp("&AMP;",pIn,5)==0){
/* 		*pOut++ = '&';
/* 		pIn += 5;
/* 	    }
/* 	    else if(strncasecmp("&QUOT;",pIn,6)==0){
/* 		*pOut++ = '"';
/* 		pIn += 6;
/* 	    }
/* 	    else {
/* 		*pOut++ = *pIn++;
/* 	    }
/* 	} else {
/* 	    *pOut++ = *pIn++;
/* 	}
/*     }
/*     *pOut=0;
/*     return(eTrue);
/* }
/* */

eBoolean_t
RemoveCharCodes(char *pIn)
{
    char *pOut, *pStart;
    if(!pIn) {
	DebugHTML(__FILE__,__LINE__,4,"RemoveCharCodes(Empty)");
	return(eTrue);
    }

    pOut = pStart = pIn;
    while(*pIn){
	if(*pIn=='&'){
	    if(bStrNCaseMatch("&GT;",pIn,4)){
		*pOut++ = '>';
		pIn += 4;
	    }
	    else if(bStrNCaseMatch("&LT;",pIn,4)){
		*pOut++ = '<';
		pIn += 4;
	    }
	    else if(bStrNCaseMatch("&AMP;",pIn,5)){
		*pOut++ = '&';
		pIn += 5;
	    }
	    else if(bStrNCaseMatch("&QUOT;",pIn,6)){
		*pOut++ = '"';
		pIn += 6;
	    }
	    else {
		if(pOut != pIn) {
		    *pOut++ = *pIn++;
		}
		else {
		    pOut++; pIn++;
		}
	    }
	} else {
	    if(pOut != pIn) {
		*pOut++ = *pIn++;
	    }
	    else {
		pOut++; pIn++;
	    }
	}
    }
    *pOut=0;
    unescape_url(pStart);
    return(eTrue);
}

extern int
iStrLen(char *s)
{
    if(!s) return(0);
    return(strlen(s));
}
extern int
iStrCmp(char *s, char *t)
{
    if(s&&t) return(strcmp(s,t));
    if(!s&&!t) return(0);
    return(1);
}
extern int
iStrCaseCmp(char *s, char *t)
{
    if(s&&t) return(strcasecmp(s,t));
    if(!s && !t) return(0);
    return(1);
}
extern eBoolean_t
bStrMatch(char *s,char *t)
{
    if(s&&t) return( (strcmp(s,t)==0)?eTrue:eFalse);
    if(!s && !t) return(eTrue); /* Never mind Codd&Date */
    return(eFalse);
}
extern eBoolean_t
bStrNMatch(char *s,char *t,int n)
{
    if(s&&t) return( (strncmp(s,t,n)==0)?eTrue:eFalse);
    if(!s && !t) return(eTrue); /* Never mind Codd&Date */
    return(eFalse);
}
extern eBoolean_t
bStrCaseMatch(char *s,char *t)
{
    if(s&&t) return( (strcasecmp(s,t)==0)?eTrue:eFalse);
    if(!s && !t) return(eTrue); /* Never mind Codd&Date */
    return(eFalse);
}
extern eBoolean_t
bStrNCaseMatch(char *s,char *t,int n)
{
    if(s&&t) return( (strncasecmp(s,t,n)==0)?eTrue:eFalse);
    if(!s && !t) return(eTrue); /* Never mind Codd&Date */
    return(eFalse);
}

extern eBoolean_t
GetCookieValueREF(char *pName, char **pBuf, long *lSize)
{
    SYMBOL *pSym, Sym;
    Sym.pName=pName;

    *pBuf=0;
    *lSize=0L;

    if(glCookie==NULL_LIST)
	return(eTrue);

    pSym=(SYMBOL*)l_find(glCookie,GetSymbolName,&Sym);
    if(pSym) {
	(*pBuf)=pSym->pValue;
	*lSize =pSym->lSize;
    }
    return(eTrue);
}
