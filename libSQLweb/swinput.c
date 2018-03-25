/* swinput.c - input processor
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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "sqlweb.h"

extern char **environ;

LIST *glCookie;

/* Internal (private) Functions
 */
static eBoolean_t ParseMultiPartFormData(char *pContent,long lInSize);
static eBoolean_t getHeaderItem (char *pHeader
				,char *pAttrName
				,char **pNameValue);


eBoolean_t
swinput()
{
    char **pEnv
	,sContent[MAX_BUFSIZ]
	,sCookie[MAX_BUFSIZ]
	,sContentLength[MAX_TOKVAL_SIZE]
	,sBuf[MAX_TOK_SIZE + MAX_TOKVAL_SIZE]
	,*pContent
	,*pContentType
	,*pValue
	,cSectionID
	;
    long
	 lContentLength = 0
	;
    FILE *pIniFile=0;

    /*
    /* Load Entire ENV into SymbolTable
     */
    for(pEnv=environ;*pEnv;++pEnv){
	register char
		 *pName
		,*pValue
		;

	pName = (char*)strcpy(sContent,*pEnv);
	pValue = strchr(pName,'=');
	if(pValue) {
	    *(pValue)=0;
	    ++pValue;
	} else {
	    pValue = 0;
	}

	ENVSym(DupBuf(pName),DupBuf(pValue));
    }

    /*
    /* Then the INI File
    /* First Choice(s) -i inifile or $SQLWEB_INI
     */
    pValue= gpIniFile ?gpIniFile :getenv("SQLWEB_INI");
    if(pValue){
	pIniFile=fopen(pValue,"r");
    }
    if(!pIniFile){
	char sIniFilename[BUFSIZ];
	/* Second Choice, ./sqlweb.ini
	 */
	sprintf(sIniFilename,"./%s.ini", gpProgram);
	pIniFile=fopen(sIniFilename,"r");
	if(!pIniFile){
	    /* Third Choice, The global INI_PATH (/etc/sqlweb.ini)
	     */
	    sprintf(sIniFilename,"/etc/%s.ini", gpProgram);
	    pIniFile=fopen(sIniFilename,"r");
	    if(!pIniFile){
		/* Third swing, Yeeerrrrr OUT!
		 */
		MsgPush("Failed to Open(%s),errno=%d",sIniFilename,errno);
		return(eFalse);
	    }
	}
    }
    cSectionID = 'S';
    while(fgets(sBuf,MAX_TOK_SIZE+MAX_TOKVAL_SIZE,pIniFile) != 0){
	/*
	/* Comments start with # or ; in position 1
	 */
	if(*sBuf=='#' || *sBuf==';' || iStrLen(sBuf)<2)
	    continue;

	/*
	/* Section Header
	 */
	if(*sBuf=='[') {
	    cSectionID = sBuf[1]==' '?sBuf[2]:sBuf[1];
	    continue;
	}

	/*
	/* Don't forget to CHOP newline...
	 */
	sBuf[ iStrLen(sBuf)-1 ] = 0;	/* CHOP */

	/*
	/* Add The Exception Tags into the TagList
	 */
	if(cSectionID=='T') {
	    if(ISeFalse(LoadTag(sBuf))) {
		MsgPush("Failed to Create TAG");
	    }
	    continue;
	}

	/*
	/* Put EnvTags into the Env
	 */
	if(cSectionID=='E'){
	    putenv(DupBuf(sBuf));
	}
	/*
	/* Put Symbol and EnvTags into SymbolTable
	 */
	if(cSectionID=='E' || cSectionID=='S') {
	    /* look for '=' sign...
	     */
	    pValue = strchr(sBuf,'=');
	    if(pValue) {
		*(pValue)=0;	/* Null sBuf, symbol name */
		++pValue;	/* Symbol Value.. */

		/* Add the INI Symbol to SYMBOL Table
		 */
		INISym(DupBuf(sBuf),DupBuf(pValue));
	    }
	}
    }

    /*
    /* Implement the POST METHOD-- I've modified this to try to read until
    /*  the input is all received, previously only attempted a single read
    /*  This should alievate swinput errors...
     */
    /* Read the Input -- if it's there.
     */
    if(ISeTrue(GetSymbolValue("CONTENT_LENGTH",sContentLength)) ){
	long lBytesRead
	    ,lOneReadCount
	    ;

	lContentLength = atoi(sContentLength);

	pContent = malloc(lContentLength+1);
	if(!pContent){
	    /* We got a problem.....
	     */
	    MsgPush("Failed to malloc %d bytes" ,lContentLength);
	    return(eFalse);
	}
	*pContent=0;
	/* sContent[0]=0;
	 */

	if( lContentLength >0 ) {
	    for( lBytesRead=0, lOneReadCount=1
		;lBytesRead<lContentLength && lOneReadCount>0
		;lOneReadCount=read(0
				   ,pContent+lBytesRead
				   ,lContentLength-lBytesRead)
		 ,lBytesRead += lOneReadCount)
	    {
		; /* The null for loop */
	    }
	    /* Testing indicates it's possible to over shoot... 
	    /* if(lBytesRead != lContentLength)
	     */
	    if(lBytesRead < lContentLength) {
	 	/* We got a problem.....
	 	 */
	 	MsgPush("Read only %d bytes of %d Content"
	 	    ,lBytesRead
	 	    ,lContentLength
	 	    );
	 	return(eFalse);
	    }

	    /* Using contentLength because it's possible to read
	    /* More that the specified chars, so since we only test
	    /* for underflow, it's best to use the passed-in value
	    /* to null the string.
	     */
	    /* sContent[lContentLength]=0;
	     */
	    *(pContent+lContentLength)=0;
	}
    }
    /* Parse POSTED FORM.
     */
    if(lContentLength>0) {
	ENVSym(DupBuf("CONTENTS"),DupBuf(pContent));

	if(ISeTrue(GetSymbolValueREF("CONTENT_TYPE",&pContentType))
	     && (bStrNMatch(pContentType,"multipart/form-data",19)))
	{
	    if( ISeFalse(ParseMultiPartFormData(pContent,lContentLength)) ){
		MsgPush("Failed to Parse multipart/form-data");
		return(eFalse);
	    }
	} else {
	    if( ISeFalse(BuildSymbolTable(pContent)) ){
		MsgPush("Failed to build SymbolTable from CONTENT");
		return(eFalse);
	    }
	}
    }

    /*
    /* Look for QUERY_STRING
     */
    if(ISeTrue(GetSymbolValue("QUERY_STRING",sContent)) ){
	/* First, check for ISMAP....
	 */
	if( !(strchr(sContent,'=')) ) {
	    char *pX, *pY;
	    pX = sContent;
	    pY=strchr(sContent,',');
	    if(pY) {
		*pY = 0; /* replace "," with NULL */
		pY++;	/* Move to one past the "," */
		/*
		/* Ok, lets add (X,Y) the the Symbol Table
		 */
		ENVSym("X_CLICK",DupBuf(pX));
		ENVSym("Y_CLICK",DupBuf(pY));
	    }
	} else if( ISeFalse(BuildSymbolTable(sContent)) ){
	    MsgPush("Failed to build SymbolTable from QUERY_STRING");
	    return(eFalse);
	}
    }
    if(ISeTrue(GetSymbolValue("HTTP_COOKIE",sCookie)) ){
	char *pCookieName, *pCookieValue, *pCookieEnd;
	SYMBOL *pSym;
	for( pCookieName=sCookie;
	     pCookieName;
	     pCookieName=pCookieEnd )
	{
	    if((pCookieValue=strchr(pCookieName,'=')))
	    {
		*pCookieValue++=0;     /* NULLs end of pCookieName */
		pCookieEnd=strchr(pCookieValue,';');
		if(pCookieEnd) {
		    *pCookieEnd++=0; /* NULL pCookieValue */
		}

		/* OK, we've got a COOKIE; let's stuff it on
		/* the GLOBAL COOKIE LIST glCookie
		 */
		if(glCookie==NULL_LIST){
		    glCookie=l_create("INORDER");
		    if(!glCookie) {
			MsgPush("l_create failed in Cookie ...");
			return(eFalse);
		    }
		}
		pSym = NewPIA();
		if(!pSym){
		    MsgPush("swinput: Cookie, NewPIA failed");
		    return(eFalse);
		}
		pSym->iType=0;	/* Not used, best init it, though */
		pSym->pName=DupBuf(pCookieName);
		pSym->pValue=DupBuf(pCookieValue);
		pSym->lSize=strlen(pCookieValue);
		ENQ(glCookie,pSym);
		continue;
	    }
	    /* ... potential invalid cookie ... 
	     */
	    break;
	}
    }
    return(eTrue);
}

/*
/* Parse a Multipart/form-data buffer
/* first line is the "delimiter"
/* then a series of "parts" each made up of a Header and Body
/* Header begins with a "content-disposition" header and
/* optionally more headers like content-type
/* The Body follows a blank line after the Headers and continues
/* to the next "delimiter".  The last NEWLINE should be stripped
/* off of the end of the field.
 */
static eBoolean_t
ParseMultiPartFormData(char *pIn, long lInSize)
{
    char *pBoundry = pIn
	,*pName    = 0
	,*pUnixEOL = "\n"
	,*pUnixEOH = "\n\n"
	,*pDosEOL  = "\r\n"
	,*pDosEOH  = "\r\n\r\n"
	,*pEOL     = pUnixEOL
	,*pEOH     = pUnixEOH
	;
    FILE *pF, *fopen();

    /* First line is multi-part delimiter
     */
    for(pBoundry=pIn;*pIn;++pIn) {
	if(bStrNMatch(pDosEOL,pIn,iStrLen(pDosEOL))) {
	    pEOL = pDosEOL;
	    pEOH = pDosEOH;
	    break;
	}
	if(bStrNMatch(pUnixEOL,pIn,iStrLen(pUnixEOL))) {
	    pEOL = pUnixEOL;
	    pEOH = pUnixEOH;
	    break;
	}
    }
    if(*pIn==0){
	MsgPush("Failed to find Border in ParseMultiPartFormData");
	return(eFalse);
    }
    *pIn=0;			/* NULL the Delimiter String */
    pIn += iStrLen(pEOL);	/* Set pointer */

#ifdef DEBUG
    pF=fopen("/tmp/sqlweb.log","a");
    fprintf(pF,"********************************************\n");
    fprintf(pF,"DELIM=^%s^\n",pBoundry);
    fflush(pF);
#endif

    while(*pIn && pIn-pBoundry<lInSize) {
	char *pStart
	    ,*pEnd
	    ,*pBody
	    ;
	/*
	/* Let's build the Parts, pBody is used below as a
	/* temporary variable for walking through binary data
	/* in case the file is a image/jpeg file.
	/* This loop sets pEnd which marks the end of the Body
	 */
	pBody = pStart = pIn;
	while( (long)(pBody-pBoundry) <lInSize) {
	    pEnd = (char*)strstr(pBody,pBoundry);
	    if(pEnd)
		break;
	    pBody += iStrLen(pBody)+1;
	    while((*pBody)!=(*pBoundry) && (long)(pBody-pBoundry)<lInSize)
		++pBody;
	}
	if(!pEnd) {
	    MsgPush("No Boundry in Multipart/form-data");
	    return(eFalse);
	}
	pEnd = pEnd-iStrLen(pEOL);
	*pEnd=0;

	/*
	/* Let's get the Header
	 */
	pBody = (char*)strstr(pStart,pEOH);
	if(!pBody){
	    MsgPush("No Separator after Headers in MultiPart/Form-data [%s]"
		   ,pStart
		   );
	    return(eFalse);
	}
	*pBody=0;	/* NULL header */
	pBody += iStrLen(pEOH);

#ifdef DEBUG
	fprintf(pF,"pHeader[%d]=^%s^\n",(pEnd-pBody),pStart);
	/*
	/* fprintf(pF,"pValue=^%s^\n",pBody);
	 */
#endif
	if(ISeTrue(getHeaderItem(pStart,"name=",&pName))) {
	    char *pFileName;
	    RAWSym(DupBuf(pName)
		  ,(char *)DupMem(pBody,(pEnd-pBody)+1)
		  ,(pEnd-pBody)+1 /* +1 adds the NULL */
		  );
	    if(ISeTrue(getHeaderItem(pStart,"filename=",&pFileName))) {
		char sBuf[MAX_TOK_SIZE];
		sprintf(sBuf,"%s_filename",pName);
		FRMSym(DupBuf(sBuf),DupBuf(pFileName));
		/*
		/* Since it's a FILE, let's try to get the TYPE
		 */
		if(ISeTrue(getHeaderItem(pStart,"Content-Type:",&pFileName))){
		    char sBuf[MAX_TOK_SIZE];
		    sprintf(sBuf,"%s_content_type",pName);
		    FRMSym(DupBuf(sBuf),DupBuf(pFileName));
		}
	    } else {
		;
#ifdef DEBUG
		/* Since it's NOT a FILE, let's have a look see
		 */
		fprintf(pF,"pValue=^%s^\n",pBody);
#endif
	    }
	} else {
	    MsgPush("No Name in Headers in MultiPart/Form-data");
	    return(eFalse);
	}

	/* Advance pIn */
	pIn = pEnd + iStrLen(pEOL) + iStrLen(pBoundry);
	if( *pIn=='-' && *(pIn+1)=='-') {
	    return(eTrue);
	}
	pIn += iStrLen(pEOL);
    }
    fclose(pF);
    return(eTrue);
}

static eBoolean_t
getHeaderItem(char *pHeader, char *pAttrName, char **pNameValue)
{
    char *pStart, *pEnd
	,sBuf[MAX_TOK_SIZE]
	;
    int i;

    *pNameValue = 0;
    pStart = strstr(pHeader,pAttrName);
    if(!pStart) {
	/* MsgPush("Missing Attribute \"%s\" in Headers in MultiPart/Form-data"
	/*	,pAttrName
	/*	);
	 */
	return(eFalse);
    }
    for(pStart+=iStrLen(pAttrName);*pStart && !isalnum(*pStart);pStart++)
	;
    for( i=0,pEnd=pStart
	;*pEnd &&i<MAX_TOK_SIZE
		&& *pEnd!='"' && *pEnd!=';' && *pEnd!='\n' && *pEnd!='\r'
	;pEnd++,i++)
    {
	sBuf[i]= *pEnd;
    }
    sBuf[i]=0;
    *pNameValue = DupBuf(sBuf);
    return(eTrue);
}
