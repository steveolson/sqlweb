/* scalar.c - suupporting functions for parser
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

#include <string.h>
#include <errno.h>
#include "scalar.h"
#include "sqlweb.h"
#include "ifparse.h"
#include "ifwords.h"

/* 
/* The Global Variables for the IfParser
 */
static LIST *glAllScalarElements;	/* All Allocated Stuff, for cleanup */
static Scalar_t gsNullScalar /* = {-1,0,0,0} */ ;

/*
/* External Functions
 */
extern double atof();
extern char *strcat();
extern atoi();

/*
/* Private Functions
 */
static eBoolean_t aToBool(char *p);
/* static void PushScalar(Scalar_t *p);
 */
static double ScalarToDouble(Scalar_t *p);
static char *ScalarToString(Scalar_t *p);
static eBoolean_t CompareDouble(int iOp,double dLeft, double dRight);
static eBoolean_t CompareString(int iOp,char *pLeft, char *pRight);
static eBoolean_t IsNumber(Scalar_t *p);
static void ToUpper(char *pBuf);
static void ToLower(char *pBuf);
static void FormatString(char *pInpStr,char *pFmtStr,char *pOutBuf);
static long GetReplaceSize(char *pBuf, char *pSrch, char *pRepl );
static char * Replace( char *pBuf, char *pSrch, char *pRepl );

/********************************************************
/* Boolean Functions
 */
eBoolean_t 
BetweenCond(Scalar_t *p, Scalar_t *pLo, Scalar_t *pHi)
{
    double d, dLo, dHi;
    if(!p || !pLo || !pHi) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:BETWEEN:Scalar is NULL(%x between %x and %x)"
		,p
		,pLo
		,pHi
		);
	MsgPush("WARN:BETWEEN:Scalar is NULL");
	return(eFalse);
    }

    d   = ScalarToDouble(p);
    dLo = ScalarToDouble(pLo);
    dHi = ScalarToDouble(pHi);

    return( (d>=dLo && d<=dHi)?eTrue :eFalse);
}

eBoolean_t 
LikeCond(Scalar_t *p1, Scalar_t *p2)
{
    char *pStr1, *pStr2;
    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:LIKE:Scalar is NULL(%x like %x)"
		,pStr1
		,pStr2
		);
	MsgPush("WARN:LIKE:Scalar is NULL");
	return(eFalse);
    }

    pStr1 = ScalarToString(p1);
    pStr2 = ScalarToString(p2);

    DebugHTML(__FILE__,__LINE__,0,"WARN:LIKE only supports EQ.");
    DebugHTML(__FILE__,__LINE__,4,"Strcmp(%s,%s)",pStr1,pStr2);
    /* return( (strcmp(pStr1,pStr2)==0)?eTrue :eFalse );
     */
    return( bStrMatch(pStr1,pStr2) );
}

eBoolean_t 
IsNullCond(Scalar_t *p)
{
    char *pStr;

    if(!p){
	DebugHTML(__FILE__,__LINE__,2,"WARN:IS NULL:Scalar is NULL");
	return(eTrue);
    }
    pStr = ScalarToString(p);

    /*
    /* Compare STRINGS
     */
    return(pStr ? (iStrLen(pStr)==0)?eTrue :eFalse 
	        : eTrue
	  );
}

eBoolean_t 
InCond(Scalar_t *pVal, Scalar_t *pList)
{
    Scalar_t *pTop;
    if(!pVal || !pList) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:IN:Scalar is NULL(%x in(%x))"
		,pVal
		,pList
		);
	MsgPush("WARN:IN:Scalar is NULL");
	return(eFalse);
    }
    for(pTop=DEQ(pList->uVal.pList);pTop;pTop=DEQ(pList->uVal.pList)){
	if(ISeTrue(CompareCond(MkScalarToken(EQ),pVal,pTop))){
	    return(eTrue);
	}
    }
    return(eFalse);
}

eBoolean_t 
CompareCond(Scalar_t *pOp,Scalar_t *pLeft,Scalar_t *pRight)
{
    double dLeft, dRight;
    char  *pStrLeft,*pStrRight;

    if(!pOp || !pLeft || !pRight) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:COMPARE:Scalar is NULL(op=%x, %x op %x)"
		,pOp
		,pLeft
		,pRight
		);
	MsgPush("WARN:COMPARE:Scalar is NULL");
	return(eFalse);
    }

    /*
    /* If either operand is a number, they are all numbers 
     */
    if(ISeTrue(IsNumber(pLeft)) || ISeTrue(IsNumber(pRight)))
    {
	/* Comparing Numbers */
	dLeft = ScalarToDouble(pLeft);
	dRight= ScalarToDouble(pRight);
	return(CompareDouble(pOp->uVal.iInt,dLeft,dRight));
    }
    /*
    /* Otherwise treat them as strings
     */
    pStrLeft =ScalarToString(pLeft);
    pStrRight=ScalarToString(pRight);
    return(CompareString(pOp->uVal.iInt,pStrLeft,pStrRight));
}

/********************************************************
/* The Scalar Functions
 */
Scalar_t *
AddScalar(Scalar_t *p1, Scalar_t *p2)
{
    double d1, d2;

    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:ADD:Scalar is NULL(%x + %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
    /*
    /* If either is a number convert to number...
     */
    if( ISeTrue(IsNumber(p1)) || ISeTrue(IsNumber(p2))) {
	d1 = ScalarToDouble(p1);
	d2 = ScalarToDouble(p2);

	p1->esType = eDoubleType;
	p1->uVal.dDouble = d1+d2;
	p1->lSize = 0L;
	return(p1);
    }
    return(CatScalar(p1,p2));
}

Scalar_t *
CatScalar(Scalar_t *p1, Scalar_t *p2)
{
    char *pB1,*pB2,*pRet;
    long lSize;

    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:CAT:Scalar is NULL(%x + %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }

    pB1=ScalarToString(p1);
    pB2=ScalarToString(p2);
    lSize = iStrLen(pB1)+iStrLen(pB2)+1; /* need room for null */

    pRet = (char*)malloc(lSize);
    if(!pRet) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:CAT:Malloc failed(%x,%x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
    (void)memset(pRet,0,lSize);

    strcpy(pRet,pB1);
    strcat(pRet,pB2);
    p1->esType = eStringType;
    p1->uVal.pString = pRet;
    p1->lSize = lSize;
    return(p1);
}

Scalar_t *
SubScalar(Scalar_t *p1, Scalar_t *p2)
{
    double d1, d2;
    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:SUB:Scalar is NULL(%x - %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
/*
/*   Convert to number and subtract...
 */
    d1 = ScalarToDouble(p1);
    d2 = ScalarToDouble(p2);

    p1->esType = eDoubleType;
    p1->uVal.dDouble = d1-d2;
    p1->lSize = 0;
    return(p1);
}
Scalar_t *
MultScalar(Scalar_t *p1, Scalar_t *p2)
{
    double d1, d2;
    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:MULTIPLY:Scalar is NULL(%x * %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
    /* Convert to number and do it!
     */
    d1 = ScalarToDouble(p1);
    d2 = ScalarToDouble(p2);

    p1->esType = eDoubleType;
    p1->uVal.dDouble = d1*d2;
    p1->lSize = 0;
    return(p1);
}
Scalar_t *
DivScalar(Scalar_t *p1, Scalar_t *p2)
{
    double d1, d2;
    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:DIV:Scalar is NULL(%x / %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
    d1 = ScalarToDouble(p1);
    d2 = ScalarToDouble(p2);
    if( (int)d2==0) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:Div by Zero");
	return(&gsNullScalar);
    }

    p1->esType = eDoubleType;
    p1->uVal.dDouble = d1/d2;
    p1->lSize = 0;
    return(p1);
}
Scalar_t *
ModScalar(Scalar_t *p1, Scalar_t *p2)
{
    double d1, d2;
    if(!p1 || !p2) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:MOD:Scalar is NULL(%x / %x)"
		,p1
		,p2
		);
	return(&gsNullScalar);
    }
    d1 = ScalarToDouble(p1);
    d2 = ScalarToDouble(p2);
    if( (int)d2==0) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:Div by Zero");
	return(&gsNullScalar);
    }

    p1->esType = eDoubleType;
    p1->uVal.dDouble = (int)d1 %(int)d2;
    p1->lSize = 0;
    return(p1);
}

Scalar_t *
USubScalar(Scalar_t *p1)
{
    double d1;
    if(!p1) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:USUB:Scalar is NULL(%x)",p1);
	return(&gsNullScalar);
    }
    d1 = ScalarToDouble(p1);

    p1->esType = eDoubleType;
    p1->uVal.dDouble = -d1;
    p1->lSize = 0;
    return(p1);
}

Scalar_t *
CallFunction
	(Scalar_t *pName
	,Scalar_t *pList
	)
{
    Reserved_t *pResWord;
    if(!pName || !pList) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:CALL:Scalar is NULL(%x(%x))"
		,pName
		,pList
		);
	return(&gsNullScalar);
    }

    for(pResWord=gaReservedWordTable; pResWord->pName; pResWord++){
	if(is_casematch(pResWord->pName,pName->uVal.pString)) {
	    /* Got one!
	     */
	    return( (pResWord->pScalarFunc)(pList) );
	}
    }
    DebugHTML(__FILE__,__LINE__,0,"WARN:CALL:Unknown Function or Type.");
    return( &gsNullScalar );
}
Scalar_t *
AddScalarToList(Scalar_t *pList,Scalar_t *p1)
{
    if(!p1) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:ADD LIST:Scalar is NULL(%x)",p1);
	return(&gsNullScalar);
    }
    if(!pList){
	pList=MkScalar(eListType,p1,p1->lSize);
	return(pList);
    }
    ENQ(pList->uVal.pList,p1);
    return(pList);
}

/********************************************************
/* Called by yylex and elsewhere to create a scalar when found
 */
Scalar_t *
MkScalarToken(int iToken)
{
    Scalar_t *p;
    p = (Scalar_t *)malloc(sizeof(Scalar_t));
    if(!p){
	/* out of memory */
	return(0);
    }
    (void)memset(p,0,sizeof(Scalar_t));
    p->esType = eIntType;
    p->uVal.iInt = iToken;
    p->lSize = 0;
    /* PushScalar(p);
     */
    return(p);
}

Scalar_t *
MkScalar(eScalar_t esType, void *pValue, long lSize)
{
    Scalar_t *p;
    p = (Scalar_t *)malloc(sizeof(Scalar_t));
    if(!p){
	/* out of memory */
	return(0);
    }
    (void)memset(p,0,sizeof(Scalar_t));
    p->esType = esType;
    p->lSize  = lSize;
    switch(esType) {
	case eStringType:p->uVal.pString= DupMem(pValue,lSize); break;
	case eIntType:	 p->uVal.iInt   = atoi(pValue);    break;/* Integers */
	case eDoubleType:p->uVal.dDouble= atof(pValue);    break;/* Real */
	case eListType:	 p->uVal.pList  = l_create("QUEUE");
			 ENQ(p->uVal.pList,pValue);
			 break;					/* Lists */
	case eBoolType:  p->uVal.bBool  = aToBool(pValue); break;/* Bool */
	case eRawType:   p->uVal.pRaw   = DupMem(pValue,lSize); break;
    }
    /* PushScalar(p);
     */
    return(p);
}

extern Scalar_t *
ToScalar(eBoolean_t bBool)
{
    Scalar_t *p;
    p = (Scalar_t *)malloc(sizeof(Scalar_t));
    if(!p){
	/* out of memory */
	return(0);
    }
    (void)memset(p,0,sizeof(Scalar_t));
    p->esType = eBoolType;
    p->uVal.bBool  = bBool;

    /* PushScalar(p);
     */
    return(p);
}

extern eBoolean_t
IsScalar(Scalar_t *pScalar)
{
    switch(pScalar->esType) {
	case eStringType:
	    if(is_casematch(pScalar->uVal.pString,"TRUE")) return(eTrue);
	    else return(eFalse);
	case eIntType:
	    if(pScalar->uVal.iInt) return(eTrue);
	    else return(eFalse);
	case eDoubleType:
	    if(pScalar->uVal.dDouble) return(eTrue);
	    else return(eFalse);
/*	case eListType:	 pScalar->uVal.pList  = l_create("QUEUE");
/*			 ENQ(pScalar->uVal.pList,pValue);
/*			 break;					/* Lists */
/* */
	case eBoolType:  return(pScalar->uVal.bBool);
	default: return(eFalse);
    }
    return(eFalse);
}

extern eBoolean_t
OutputScalar(Scalar_t *pScalar)
{
    OutputHTML("%s",ScalarToString(pScalar));
    return(eTrue);
}
/*
/* Internal Private Functions
/* static void
/* PushScalar(Scalar_t *p)
/* {
/*     if(!glAllScalarElements){
/* 	glAllScalarElements=l_create("QUEUE");
/*     }
/*     ENQ(glAllScalarElements,p);
/*     return;
/* }
 */

static double
ScalarToDouble(Scalar_t *p)
{
    if(!p) return(0.0);

    switch(p->esType) {
	case eStringType:return(atof(p->uVal.pString));
	case eIntType:	 return((double)p->uVal.iInt);
	case eDoubleType:return(p->uVal.dDouble);
	case eListType:	 return(0.0);
	case eBoolType:  return ISeTrue(p->uVal.bBool) ? 1.0 :0.0 ;
	default:	 return(0.0);
    }
    return(0.0);
}

static char *
ScalarToString(Scalar_t *p)
{
    char sBuf[BUFSIZ];
    char *pNullStr = "";
    int iLen;
    if(!p) {
	DebugHTML(__FILE__,__LINE__,0,"ToStr:NULLStr");
	return(pNullStr);
    }

    DebugHTML(__FILE__,__LINE__,5,"ToStr:%s.%d(%d),%s"
	    ,p->esType==eStringType    ?"S"
		:p->esType==eIntType   ?"I"
		:p->esType==eDoubleType?"D"
		:p->esType==eListType  ?"L"
		:p->esType==eBoolType  ?"B"
		:p->esType==eRawType   ?"R"
		:"?"
	    ,p->lSize
	    ,p->esType==eStringType?iStrLen(p->uVal.pString):-1
	    ,p->esType==eStringType && p->uVal.pString?p->uVal.pString:"*?*"
	    );
    switch(p->esType) {
	case eStringType:
			if(p->lSize==0) {
			    return(DupBuf(0));
			}
			/* if(p->uVal.pString && p->lSize>=0)
			/*    *(p->uVal.pString + p->lSize)=0; /* set NULL */
			/* */
			
			return(p->uVal.pString);
	case eIntType:	sprintf(sBuf,"%d",p->uVal.iInt);
			return(DupBuf(sBuf));
	case eDoubleType:
			sprintf(sBuf,"%f",p->uVal.dDouble);
			if(strchr(sBuf,'.')) {
			    /* Strip Trailing zeros, if decpt
			     */
			    iLen = iStrLen(sBuf);
			    while( sBuf[--iLen] == '0')
				sBuf[iLen]=0;
			    if(sBuf[iLen] == '.')
				sBuf[iLen]=0;
			}
			return(DupBuf(sBuf));
	case eListType: return(pNullStr);
	case eBoolType: return(ISeTrue(p->uVal.bBool)?"TRUE":"FALSE");
	default:
	    DebugHTML(__FILE__,__LINE__,0,"ToStr:Unknown Type %d",p->esType);
    }
    return(pNullStr);
}
static eBoolean_t
aToBool(char *p)
{
    if(is_casematch(p,"FALSE")) return(eFalse);
    if(is_casematch(p,"TRUE") )  return(eTrue);
    return(eTrue);
}

static eBoolean_t
CompareDouble(int iOp,double dLeft,double dRight)
{
    switch(iOp){
	case EQ:  return( (dLeft==dRight)   ?eTrue:eFalse);
	case NEQ: return( (!(dLeft==dRight))?eTrue:eFalse);
	case GT:  return( (dLeft>dRight)    ?eTrue:eFalse);
	case GE:  return( (dLeft>=dRight)   ?eTrue:eFalse);
	case LT:  return( (dLeft<dRight)    ?eTrue:eFalse);
	case LE:  return( (dLeft<=dRight)   ?eTrue:eFalse);
    }
    DebugHTML(__FILE__,__LINE__,0,"WARN:COMPARE DBL:Unknown OP:%d",iOp);
    return(eFalse);
}

static eBoolean_t
CompareString(int iOp,char *pLeft,char *pRight)
{
    DebugHTML(__FILE__,__LINE__,2,"COMPARE STR:%s %d %s",pLeft,iOp,pRight);
    switch(iOp){
	case EQ:  return(is_match(pLeft,pRight)   ?eTrue:eFalse);
	case NEQ: return(!(is_match(pLeft,pRight))?eTrue:eFalse);
	case GT:  return((iStrCmp(pLeft,pRight)>0)    ?eTrue:eFalse);
	case GE:  return((iStrCmp(pLeft,pRight)>=0)   ?eTrue:eFalse);
	case LT:  return((iStrCmp(pLeft,pRight)<0)    ?eTrue:eFalse);
	case LE:  return((iStrCmp(pLeft,pRight)<=0)   ?eTrue:eFalse);
    }
    DebugHTML(__FILE__,__LINE__,0,"WARN:COMPARE STR:Unknown OP:%d",iOp);
    MsgPush("WARN:COMPARE STR:Unknown OP:%d",iOp);
    return(eFalse);
}

static eBoolean_t
IsNumber(Scalar_t *p)
{
    if(!p){
	DebugHTML(__FILE__,__LINE__,0,"IsNumber:Scalar is NULL");
	MsgPush("IsNumber:Scalar is NULL");
	return(eFalse);
    }

    if(p->esType==eDoubleType || p->esType==eIntType)
	return(eTrue);
    return(eFalse);
}

Scalar_t *
ScalarUpper(Scalar_t *pList)
{
    Scalar_t *pTop;
    char sBuf[MAX_BUFSIZ];

    DebugHTML(__FILE__,__LINE__,3,"Called Upper(%x)",pList);
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarUpper:Scalar is NULL(%x)",pList);
	MsgPush("ScalarUpper:Scalar is NULL");
	return(&gsNullScalar);
    }
    if(pList->esType != eListType) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:UPPER:Invalid arg type passed to Function");
	return(&gsNullScalar);
    }
    sBuf[0]=0;
    while( (pTop = (Scalar_t *)DEQ(pList->uVal.pList)) ) {
	char *pTmp = ScalarToString(pTop);
	strcat(sBuf,pTmp);
    }
    ToUpper(sBuf);
    return(MkScalar(eStringType,sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarLower(Scalar_t *pList)
{
    Scalar_t *pTop;
    char sBuf[MAX_BUFSIZ];

    if(!pList) {
	DebugHTML(__FILE__,__LINE__,0,"ScalarLower:Scalar is NULL(%x)",pList);
	MsgPush("ScalarLower:Scalar is NULL");
	return(&gsNullScalar);
    }
    if(pList->esType != eListType) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:LOWER:Invalid arg type passed to Function");
	return(&gsNullScalar);
    }
    sBuf[0]=0;
    while( (pTop = (Scalar_t *)DEQ(pList->uVal.pList)) ) {
	char *pTmp = ScalarToString(pTop);
	strcat(sBuf,pTmp);
    }
    ToLower(sBuf);
    return(MkScalar(eStringType,sBuf,1+iStrLen(sBuf)));
}
Scalar_t *
ScalarSubstr(Scalar_t *pList)
{
    char *pTmp;
    int iSize,iOffset,iLength;
    Scalar_t *pString = 0
	    ,*pOffset = 0
	    ,*pLength = 0
	;
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarSubstr:Scalar is NULL(%x)",pList);
	MsgPush("ScalarSubstr:Scalar is NULL");
	return(&gsNullScalar);
    }
    iSize = l_size(pList->uVal.pList);
    if(iSize==2){
	pString=(Scalar_t *)DEQ(pList->uVal.pList);
	pOffset=(Scalar_t *)DEQ(pList->uVal.pList);

	pTmp   = ScalarToString(pString);
	iOffset= (int)ScalarToDouble(pOffset);
	if(iOffset<iStrLen(pTmp)) {
	    pString->esType = eStringType;
	    pString->uVal.pString = (pTmp+iOffset);
	    pString->lSize = 1+iStrLen(pString->uVal.pString);
	    return(pString);
	}
    } else if (iSize==3) {
	pString=(Scalar_t *)DEQ(pList->uVal.pList);
	pOffset=(Scalar_t *)DEQ(pList->uVal.pList);
	pLength=(Scalar_t *)DEQ(pList->uVal.pList);

	pTmp   = ScalarToString(pString);
	iOffset= (int)ScalarToDouble(pOffset);
	iLength= (int)ScalarToDouble(pLength);

	/* Reusing iSize as length of first arg */
	iSize = iStrLen(pTmp);
	if(iOffset<iSize) {
	    pString->uVal.pString = (pTmp + iOffset);
	    if((iOffset+iLength) <iSize)
		*(pTmp + iOffset + iLength) = 0;
	    pString->esType = eStringType;
	    pString->lSize = 1+iStrLen(pString->uVal.pString);
	    return(pString);
	}
    } else {
	DebugHTML(__FILE__,__LINE__,0,"WARN:SUBSTR:Invalid args to Substr");
    }
    return(&gsNullScalar);
}

Scalar_t *
ScalarLength(Scalar_t *pList)
{
    Scalar_t *pTop;
    int iLen=0, lLen=0;
    char sLenBuf[MAX_INTEGER_DIGITS];

    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarLength:Scalar is NULL(%x)",pList);
	MsgPush("ScalarLength:Scalar is NULL");
	return(&gsNullScalar);
    }
    if(pList->esType != eListType) {
	DebugHTML(__FILE__,__LINE__,0
		 ,"WARN:LENGTH:Invalid arg type passed to Function"
		 );
	return(&gsNullScalar);
    }
    iLen=0, lLen=0;
    while( (pTop = (Scalar_t *)DEQ(pList->uVal.pList)) ) {
	/* char *pTmp = ScalarToString(pTop);
	/* iLen += iStrLen(pTmp);
	 */
	lLen += (pTop->lSize>0)?(pTop->lSize) -1:0;
	/* DebugHTML(__FILE__,__LINE__,0,"lSize=%d",lLen);
	 */
    }
    sprintf(sLenBuf,"%d",lLen);
    return(MkScalar(eDoubleType, sLenBuf,1+iStrLen(sLenBuf)));
}

Scalar_t *
ScalarNVL(Scalar_t *pList)
{
    Scalar_t *pTop;

    DebugHTML(__FILE__,__LINE__,3,"Called NVL");

    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarNVL:Scalar is NULL(%x)",pList);
	MsgPush("ScalarNVL:Scalar is NULL");
	return(&gsNullScalar);
    }
    pTop = (Scalar_t *)DEQ(pList->uVal.pList);
    if(IsNullCond(pTop)) {
	DebugHTML(__FILE__,__LINE__,2,"NVL:NULL Condition");
	return((Scalar_t *)DEQ(pList->uVal.pList));
    }
    DebugHTML(__FILE__,__LINE__,2,"NVL:NOT NULL Condition");
    return(pTop);
}

Scalar_t *
ScalarHost(Scalar_t *pList)
{
    Scalar_t *pTop;
    char sBuf[MAX_BUFSIZ];
    int c, iCharCount, iStatus;
    FILE *pF;

#ifdef WIN32
    MsgPush("SQLweb/NT: Doesn't Support HOST");
    return(&gsNullScalar);
#endif

#ifndef WIN32
    {
    DebugHTML(__FILE__,__LINE__,3,"Called Host");

    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarHost:Scalar is NULL(%x)",pList);
	MsgPush("ScalarHost:Scalar is NULL");
	return(&gsNullScalar);
    }
    sBuf[0]=0;
    while( (pTop = (Scalar_t *)DEQ(pList->uVal.pList)) ) {
	char *pTmp = ScalarToString(pTop);
	strcat(sBuf,pTmp);
    }

    DebugHTML(__FILE__,__LINE__,2,"Running: `%s`",sBuf);

    pF = popen(sBuf,"r");
    if(!pF){
	DebugHTML(__FILE__,__LINE__,0,"POpen Failed: (%s)",sBuf);
	MsgPush("Host:Can't popen(%s,\"r\")", sBuf);
	return(&gsNullScalar);
    }

    DebugHTML(__FILE__,__LINE__,3,"Open'd(%x)",pF);
    for( c=fgetc(pF), iCharCount=0
	;c != EOF && iCharCount<MAX_BUFSIZ
	;c=fgetc(pF), iCharCount++ )
    {
	sBuf[iCharCount] = c;
    }
    /*
    /* NULL the string...
     */
    if(iCharCount && sBuf[iCharCount-1]=='\n')
	 sBuf[iCharCount-1]=0;
    else sBuf[iCharCount] = 0;

    DebugHTML(__FILE__,__LINE__,3,"Read %d chars",iCharCount);
    iStatus = pclose(pF);
    DebugHTML(__FILE__,__LINE__,3,"popen:%d:%d",iStatus,c);
    return(MkScalar(eStringType, sBuf,1+iStrLen(sBuf)));
    }
#endif
}

Scalar_t *
ScalarSaveFile(Scalar_t *pList)
{
    Scalar_t *pScalarFileName
	    ,*pScalarData
	    ;
    char *pFileName;
    long lSize;
    int iListSize;
    FILE *pF;

    DebugHTML(__FILE__,__LINE__,3,"Called SaveFile");
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"SaveFile:Scalar is NULL(%x)",pList);
	MsgPush("ScalarSaveFile:Scalar is NULL");
	return(&gsNullScalar);
    }

    /*
    /* Check number of parameters (the USAGE)
     */
    iListSize = l_size(pList->uVal.pList);
    if(iListSize != 2) {
	DebugHTML(__FILE__,__LINE__,0
		 ,"WARN:SAVEFILE:USAGE:SaveFile(filename,data)"
		 );
	MsgPush("ScalarSaveFile:Scalar is NULL");
	return(&gsNullScalar);
    }

    /*
    /* Get Filename 
     */
    pScalarFileName = (Scalar_t *)DEQ(pList->uVal.pList);
    pScalarData = (Scalar_t *)DEQ(pList->uVal.pList);

    pFileName = ScalarToString(pScalarFileName);
    pF=fopen(pFileName,"w");
    if(!pF){
	DebugHTML(__FILE__,__LINE__,0,"SaveFile:Open Failed:%d",errno);
	MsgPush("ScalarSaveFile:Open Failed");
	return(&gsNullScalar);
    }
    lSize = pScalarData->lSize;
    lSize = fwrite(pScalarData->uVal.pString,lSize,1,pF);
    fclose(pF);

    pScalarFileName->esType = eBoolType;
    pScalarFileName->uVal.bBool = lSize==1?eTrue:eFalse;
    pScalarFileName->lSize = 0;
    return(pScalarFileName);
}

Scalar_t *
ScalarToChar(Scalar_t *pList)
{
    char *pFmtStr
	 ,*pInpStr
	 ,sBuf[BUFSIZ]
	 ;
    int iSize;
    Scalar_t *pInput = 0
	    ,*pFormat = 0
	;
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarToChar:Scalar is NULL(%x)",pList);
	MsgPush("ScalarToChar:Scalar is NULL");
	return(&gsNullScalar);
    }
    iSize = l_size(pList->uVal.pList);
    if(iSize!=2){
	DebugHTML(__FILE__,__LINE__,0,"WARN:TO_CHAR:usage error:to_char(input,fmt)");
	return(&gsNullScalar);
    }
    /* Get the Parameters
     */
    pInput =(Scalar_t *)DEQ(pList->uVal.pList);
    pFormat=(Scalar_t *)DEQ(pList->uVal.pList);

    /* Convert Each Parameter to a string
     */
    pInpStr= ScalarToString(pInput);
    pFmtStr= ScalarToString(pFormat);

    /*
    /* Convert input string using format mask
     */
    FormatString(pInpStr,pFmtStr,sBuf);
    return(MkScalar(eStringType, sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarToNumber(Scalar_t *pList)
{
    int  iDotFlag = 0
	,iDigFlag = 0;
    Scalar_t *pScalar;
    char *pInStr;
    char sBuf[MAX_BUFSIZ], *psBuf=sBuf;

    /* fprintf(stderr,"ToNumber(!)\n");fflush(stderr);
     */
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarToNumber:Scalar is NULL(%x)",pList);
	MsgPush("ScalarToNumber:Scalar is NULL");
	return(&gsNullScalar);
    }

    pScalar = DEQ(pList->uVal.pList);
    pInStr = ScalarToString(pScalar);

    for( ; *pInStr; pInStr++) {
	switch(*pInStr){
	    case '-' : 
		if(iDigFlag) break;
	    case '.' : 
		if(iDotFlag) break;
		iDotFlag=1;
	    case '0' :
	    case '1' :
	    case '2' :
	    case '3' :
	    case '4' :
	    case '5' :
	    case '6' :
	    case '7' :
	    case '8' :
	    case '9' :
		iDigFlag=1;
		*psBuf++ = *pInStr;
		break;
	} /* end of switch */
    } /* end of for */
    *psBuf = '\0';
    /* fprintf(stderr,"ToNumber(%s)\n",sBuf);fflush(stderr);
     */
    return(MkScalar(eStringType,sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarLPad(Scalar_t *pList)
{
    int		 iSize
		,iPadSize
		;
    Scalar_t 	 *pSource
		,*pSize
		,*pChar
		;
    char	 *pStrSource
		,*pStrChar = " "
		,sBuf[MAX_BUFSIZ]
		;

    /* fprintf(stderr,"LPad(!)\n");fflush(stderr);
     */
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarLPad:Scalar is NULL(%x)",pList);
	MsgPush("ScalarLPad:Scalar is NULL");
	return(&gsNullScalar);
    }

    iSize = l_size(pList->uVal.pList);
    if(iSize != 2 && iSize !=3) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:LPAD:USAGE:LPad(input,size[,padchar])");
	return(&gsNullScalar);
    }
    pSource = DEQ(pList->uVal.pList);
    pSize   = DEQ(pList->uVal.pList);
    if(iSize == 3) {
	pChar = DEQ(pList->uVal.pList);
    }
    pStrSource = ScalarToString(pSource);
    iPadSize = (int)ScalarToDouble(pSize);
    pStrChar = ScalarToString(pChar);

    if(iPadSize >= MAX_BUFSIZ) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:LPAD:overflow, truncated to %d",MAX_BUFSIZ);
	iPadSize = MAX_BUFSIZ -1;
    }

    memset(sBuf, *pStrChar, iPadSize);
    sBuf[iPadSize] = 0;

    strcpy(&sBuf[iPadSize-iStrLen(pStrSource)],pStrSource);
    return(MkScalar(eStringType,sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarRPad(Scalar_t *pList)
{
    int		 iSize
		,iPadSize
		;
    Scalar_t 	 *pSource
		,*pSize
		,*pChar
		;
    char	 *pStrSource
		,*pStrChar = " "
		,sBuf[MAX_BUFSIZ]
		;

    /* fprintf(stderr,"RPad(!)\n");fflush(stderr);
     */
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarRPad:Scalar is NULL(%x)",pList);
	MsgPush("ScalarRPad:Scalar is NULL");
	return(&gsNullScalar);
    }

    iSize = l_size(pList->uVal.pList);
    if(iSize != 2 && iSize !=3) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:RPAD:USAGE:RPad(input,size[,padchar])");
	return(&gsNullScalar);
    }
    pSource = DEQ(pList->uVal.pList);
    pSize   = DEQ(pList->uVal.pList);
    if(iSize == 3) {
	pChar = DEQ(pList->uVal.pList);
    }
    pStrSource = ScalarToString(pSource);
    iPadSize = (int)ScalarToDouble(pSize);
    pStrChar = ScalarToString(pChar);

    if(iPadSize >= MAX_BUFSIZ) {
	DebugHTML(__FILE__,__LINE__,0
		,"WARN:RPAD:overflow, truncated to %d",MAX_BUFSIZ);
	iPadSize = MAX_BUFSIZ -1;
    }

    memset(sBuf, *pStrChar, iPadSize);
    sBuf[iPadSize] = 0;

    strncpy(sBuf,pStrSource,iStrLen(pStrSource));
    return(MkScalar(eStringType, sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarLTrim(Scalar_t *pList)
{
    int		 iSize
		;
    Scalar_t 	 *pSource
		,*pChar
		;
    char	 *pStrSource
		,*pStrChar = " "
		,sBuf[MAX_BUFSIZ]
		;

    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarTrim:Scalar is NULL(%x)",pList);
	MsgPush("ScalarLTrim:Scalar is NULL");
	return(&gsNullScalar);
    }

    iSize = l_size(pList->uVal.pList);
    if(iSize != 1 && iSize !=2) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:LTRIM:USAGE:LTrim(input[,trimchar])");
	return(&gsNullScalar);
    }
    /*
    /* Get String version of Variable
     */
    pSource = DEQ(pList->uVal.pList);
    pStrSource = ScalarToString(pSource);

    /*
    /* Now Get The "Trimoff Char"
     */
    if(iSize == 2) {
	pChar = DEQ(pList->uVal.pList);
	pStrChar = ScalarToString(pChar);
    }

    while(*pStrSource == *pStrChar)
	pStrSource++;

    if(*pStrSource)
	 strcpy(sBuf,pStrSource);
    else sBuf[0] = 0;

    return(MkScalar(eStringType, sBuf,1+iStrLen(sBuf)));
}

Scalar_t *
ScalarRTrim(Scalar_t *pList)
{
    int		 iSize
		;
    Scalar_t 	 *pSource
		,*pChar
		;
    char	 *pStrSource
		,*pStrChar = " "
		;

    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarRTrim:Scalar is NULL(%x)",pList);
	MsgPush("ScalarRTrim:Scalar is NULL");
	return(&gsNullScalar);
    }

    iSize = l_size(pList->uVal.pList);
    if(iSize != 1 && iSize !=2) {
	DebugHTML(__FILE__,__LINE__,0,"WARN:RTRIM:USAGE:RTrim(input[,padchar])");
	return(&gsNullScalar);
    }
    pSource = DEQ(pList->uVal.pList);
    pStrSource = ScalarToString(pSource);

    if(iSize == 2) {
	pChar = DEQ(pList->uVal.pList);
	pStrChar = ScalarToString(pChar);
    }

    iSize = iStrLen(pStrSource);
    while( *(pStrSource+ (--iSize)) == *pStrChar)
	; /* nothing */
    *(pStrSource + iSize +1) = 0;
    return(MkScalar(eStringType, pStrSource,1+iStrLen(pStrSource)));
}

static void
ToUpper(char *pBuf)
{
    int c;
    for(c= *pBuf;c;c = *++pBuf)
	*pBuf=(char)toupper(c);
    return;
}

static void
ToLower(char *pBuf)
{
    int c;
    for(c = *pBuf;c; c= *++pBuf)
	*pBuf = (char)tolower(c);
    return;
}

static void
FormatString(char *pInpStr,char *pFmtStr,char *pOutBuf)
{
    char *pInDecPt
	,*pFmtDecPt
	;
    int  iInDecSize
	,iFmtPrecision
	,iNewDecSize
	,iNewBufSize
	,iCommaFlag
	,i
	;

    DebugHTML(__FILE__,__LINE__,3,"FormatString(%s,%s,buf)",pInpStr,pFmtStr);

    pInDecPt = (char *)strchr(pInpStr,'.');
    pFmtDecPt= (char *)strchr(pFmtStr,'.');

    iInDecSize   = (pInDecPt) ? pInDecPt - pInpStr : iStrLen(pInpStr);
    iFmtPrecision= (pFmtDecPt)? iStrLen(pFmtDecPt+1) : 0;

    iCommaFlag =  (strchr(pFmtStr,',')) ? 1 : 0;
    iNewDecSize= (iCommaFlag)
		? (iInDecSize + ((iInDecSize-1)/3)) /* Fmt str has commas */
		: iInDecSize			    /* Fmt str NO commas  */
		;
    iNewBufSize= iNewDecSize + ((iFmtPrecision>0)?1:0) + iFmtPrecision;

/*    fprintf(stderr,"NSiz=%d (%d + %d + %d) (fmtprec=%s)\n"
/* 	    , iNewBufSize
/* 	    , iNewDecSize
/* 	    , ((iFmtPrecision>0)?1:0) 
/* 	    , iFmtPrecision
/* 	    , pFmtDecPt+1
/* 	    );
/*    fflush(stderr);
 */

    for(i=0;i<iNewBufSize;i++) {
	/* Place Decimal point when not included in pInpStr
	 */
	if(i==iNewDecSize && !pInDecPt) {
	    *pOutBuf++ = '.';
	}
	/* if a comma goes here
	 */
	else if(iCommaFlag		/* format string has comma */
	    && i>0			/* no leading comma */
	    && iNewDecSize-i>0		/* no comma after decpt */
	    && ((iNewDecSize-i)%4)==0)	/* comma position */
	{
	    *pOutBuf++ = ',';		/* Insert Comma */
	}else {
	    *pOutBuf++ = (*pInpStr) ?*pInpStr++ : '0';
	}
    }
    *pOutBuf = 0;
    return;
}

extern eBoolean_t
AssignCond(Scalar_t *pScalarName
	  ,Scalar_t *pScalarValue
	  )
{
    char *pValue
	,*pName
	;
    if(!pScalarName) {
	DebugHTML(__FILE__,__LINE__,0 ,"WARN:ASSIGN:No Symbol Name");
	MsgPush("WARN:ASSIGN:No Symbol Name");
	return(eFalse);
    }
    pName=pScalarName->pName? pScalarName->pName: ScalarToString(pScalarName);
    DebugHTML(__FILE__,__LINE__,2,"AssignCond(%s)",pName); 
    if(!pScalarValue) {
	DebugHTML(__FILE__,__LINE__,0
		 ,"WARN:ASSIGN:No Symbol Value for %s"
		 ,pName
		 );
	/* return(eFalse);
	 */
	pValue = "";
    } else {
	pValue = ScalarToString(pScalarValue); /* Determine Value */
    }
    SELSym(pName,pValue);
    DebugHTML(__FILE__,__LINE__,2,"AssignCond:Completed");
    return(eTrue);
}

extern Scalar_t *
ScalarGetSym(Scalar_t *pScalar
	    ,char *pRule
	)
{
    char *pBuf, *pName;
    Scalar_t *pNewScalar;
    long lSize;

    if(!pScalar){
	DebugHTML(__FILE__,__LINE__,0,"ScalarGetSym:%s:Scalar is NULL",pRule);
	MsgPush("ScalarGetSym:%s:Scalar is NULL",pRule);
	return(pScalar);
    }
    pName =  pScalar->pName? pScalar->pName: ScalarToString(pScalar);
    if(!pName || iStrLen(pName)==0) {
	DebugHTML(__FILE__,__LINE__,0,"ScalarGetSym:No Name");
	return(pScalar);
    }

    (void)GetRawSymbolValueREF(pName,&pBuf,&lSize);

    DebugHTML(__FILE__,__LINE__,2,"ScalarGetSym(%s,%s):%s:%d"
	    ,pName
	    ,pBuf?pBuf:""
	    ,pRule
	    ,lSize
	    );
    pNewScalar = MkScalar(eStringType,pBuf,lSize);
    pNewScalar->pName = DupBuf(pName);
    return(pNewScalar);
}

Scalar_t *
ScalarGetSymARR(Scalar_t *pScalar,Scalar_t *pArrayOffset)
{
    char *pBuf, *pName;
    int iOffset;
    Scalar_t *pNewScalar;

    if(!pScalar || !pArrayOffset){
	DebugHTML(__FILE__,__LINE__,0,"ScalarARRGetSym(%x,%x):Scalar is NULL"
		,pScalar
		,pArrayOffset
		);
	MsgPush("ScalarARRGetSym:Scalar is NULL");
	return(pScalar);
    }
    pName =  pScalar->pName? pScalar->pName: ScalarToString(pScalar);
    if(!pName || iStrLen(pName)==0) {
	DebugHTML(__FILE__,__LINE__,0,"ScalarGetSym:No Name");
	return(pScalar);
    }
    iOffset = (int)ScalarToDouble(pArrayOffset);
    DebugHTML(__FILE__,__LINE__,2,"ScalarGetSym(%s,%d):%s",pName,iOffset); 
    (void)GetARRSymbolValueREF(pName,iOffset,&pBuf);

    pNewScalar = MkScalar(eStringType,pBuf,1+iStrLen(pBuf));
    pNewScalar->pName = DupBuf(pName);
    return(pNewScalar);
}

Scalar_t *
ScalarGetCookieValue(Scalar_t *pList)
{
    int iListSize;
    long lSize;
    char *pName, *pBuf;
    Scalar_t *pCookieName;

    DebugHTML(__FILE__,__LINE__,3,"Called GetCookieValue()");
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"GetCookieValue:Scalar is NULL(0)");
	MsgPush("GetCookieValue:Scalar is NULL");
	return(&gsNullScalar);
    }

    /* Check number of parameters (the USAGE)
     */
    iListSize = l_size(pList->uVal.pList);
    if(iListSize != 1) {
	DebugHTML(__FILE__,__LINE__,0
		 ,"WARN:GetCookieValue:USAGE:GetCookieValue(CookieName)"
		 );
	return(&gsNullScalar);
    }

    /*
    /* Get CookieName 
     */
    pCookieName=(Scalar_t *)DEQ(pList->uVal.pList);
    pName=ScalarToString(pCookieName);

    lSize=0L;
    (void)GetCookieValueREF(pName,&pBuf,&lSize);
    if(lSize>0){
	return( MkScalar(eStringType,pBuf,1+iStrLen(pBuf)) );
    }
    DebugHTML(__FILE__,__LINE__,1 ,"WARN:GetCookie(%s):No HTTP_COOKIE",pName);
    return(&gsNullScalar);
}

Scalar_t *
ScalarReplace(Scalar_t *pList)
{
    char *pStr, *pSrch, *pRepl, *pResult;
    int iSize;
    Scalar_t *pString = 0
	    ,*pSearch = 0
	    ,*pReplace = 0
	;
    DebugHTML(__FILE__,__LINE__,3,"ScalarReplace()");
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarReplace:Scalar is NULL(%x)",pList);
	MsgPush("ScalarReplace:Scalar is NULL");
	return(&gsNullScalar);
    }
    iSize = l_size(pList->uVal.pList);
    if(iSize==2){
	pString=(Scalar_t *)DEQ(pList->uVal.pList);
	pSearch=(Scalar_t *)DEQ(pList->uVal.pList);

	pStr   = ScalarToString(pString);
	pSrch  = ScalarToString(pSearch);
        DebugHTML(__FILE__,__LINE__,2,"ScalarReplace(%s,%s)",pStr,pSrch);
	pResult= Replace( pStr, pSrch, "" );

	pString->esType = eStringType;
	pString->uVal.pString = pResult;
	pString->lSize = iStrLen(pResult);
	return(pString);

    } else if (iSize==3) {
	pString =(Scalar_t *)DEQ(pList->uVal.pList);
	pSearch =(Scalar_t *)DEQ(pList->uVal.pList);
	pReplace=(Scalar_t *)DEQ(pList->uVal.pList);

	pStr   = ScalarToString(pString);
	pSrch  = ScalarToString(pSearch);
	pRepl  = ScalarToString(pReplace);
        DebugHTML(__FILE__,__LINE__,2,"ScalarReplace(%s,%s,%s)"
		 ,pStr
		 ,pSrch
		 ,pRepl
		 );
	pResult= Replace( pStr, pSrch, pRepl );

	pString->esType = eStringType;
	pString->uVal.pString = pResult;
	pString->lSize = iStrLen(pResult);
	return(pString);

    } else {
	DebugHTML(__FILE__,__LINE__,0,"WARN:SUBSTR:Invalid args to Replace");
    }
    return(&gsNullScalar);
}

static char *
Replace( char *pBuf, char *pSrch, char *pRepl )
{
    char *p, *pE, *po, *pOut;
    long lOutSize;

    lOutSize=GetReplaceSize(pBuf,pSrch,pRepl);
				/* Calc size of resultant string */
    po=pOut=(char*)malloc(lOutSize+1);
				/* Get storage */
    (void)memset(pOut,0,lOutSize);
				/* just in case ... cleanup space */

    for(pE=pBuf,p=strstr(pBuf,pSrch); p; p=strstr(pE,pSrch)) {
	while(pE<p)
	    *po++ = *pE++;	/* copy characters not being replaced */
	strcpy(po,pRepl);	/* copy replace string */
	pE = p + strlen(pSrch);	/* move pointer on input string ahead */
	po = po + strlen(pRepl);/* move pointer on output string ahead */
    }
    while(*pE)			/* process remaining part of string */
	*po++ = *pE++;		/* copy characters */
    *po=0;			/* don't forget the all important NULL */
    return( pOut );		/* OK, otta here! */
}

static long
GetReplaceSize(char *pBuf, char *pSrch, char *pRepl )
{
    char *p, *pE;
    long lStrCount = 0;

    for( p=strstr(pBuf,pSrch); p; p=strstr(pE,pSrch)) {
	pE = p + strlen(pSrch);
	pE++;
	lStrCount++;		/* Count occurances of "pSrch" */
    }
    return (strlen(pBuf)+(lStrCount*(strlen(pRepl)-strlen(pSrch))) );
				/* Calc Size of replaced string */
}

Scalar_t *
ScalarAscii(Scalar_t *pList)
{
    char *pStr;
    Scalar_t *pInt;
    int iSize, iChar;
    DebugHTML(__FILE__,__LINE__,3,"ScalarAscii()");
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarAscii:Scalar is NULL(%x)",pList);
	MsgPush("ScalarAscii:Scalar is NULL");
	return(&gsNullScalar);
    }
    iSize = l_size(pList->uVal.pList);
    if(iSize!=1){
	DebugHTML(__FILE__,__LINE__,0,"WARN:Invalid args to Ascii");
	return(&gsNullScalar);
    }
    pInt=(Scalar_t *)DEQ(pList->uVal.pList);
    pStr=ScalarToString(pInt);

    DebugHTML(__FILE__,__LINE__,2,"ScalarAscii(%s)",pStr);
    iChar = (int) *pStr;

    pInt->esType = eIntType;
    pInt->uVal.iInt = iChar;
    pInt->lSize = 1;
    return(pInt);
}

Scalar_t *
ScalarCHR(Scalar_t *pList)
{
    char *pStr, sChar[2];

    Scalar_t *pString;
    int iSize, iChar;
    DebugHTML(__FILE__,__LINE__,3,"ScalarCHR()");
    if(!pList){
	DebugHTML(__FILE__,__LINE__,0,"ScalarCHR:Scalar is NULL(%x)",pList);
	MsgPush("ScalarCHR:Scalar is NULL");
	return(&gsNullScalar);
    }
    iSize = l_size(pList->uVal.pList);
    if(iSize!=1){
	DebugHTML(__FILE__,__LINE__,0,"WARN:Invalid args to CHR");
	return(&gsNullScalar);
    }
    pString=(Scalar_t *)DEQ(pList->uVal.pList);

    iChar = (int) ScalarToDouble(pString);
    DebugHTML(__FILE__,__LINE__,2,"ScalarCHR(%d)",iChar);

    sChar[0] = (char)iChar;
    sChar[1] = (char)0;

    DebugHTML(__FILE__,__LINE__,2,"ScalarCHR(%s,%c)",sChar,iChar);

    pString->esType = eStringType;
    pString->uVal.pString = DupBuf(sChar);
    pString->lSize =iStrLen(sChar);
    return(pString);
}
