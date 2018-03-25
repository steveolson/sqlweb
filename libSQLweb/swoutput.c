/* swoutput.c - Output routines for SQLweb
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
/* #include <sys/types.h>
 */
#include <ctype.h>
#include <string.h>
#include "sqlweb.h"
#include "userexit.h"

/*
/* Internal Functions....
 */
static eBoolean_t LoadPageList(LIST *lPageGenNbrList);
static eBoolean_t CookPage(PAGE *pPage);
static eBoolean_t CookPIA(SYMBOL *pPIA);
static eBoolean_t OutputPage(PAGE *pPage);
static eBoolean_t OutputPI(PI *pPI);
static eBoolean_t OutputPIA(SYMBOL *pPIA);
static eBoolean_t GetPIFieldNameREF(PI *pPI,char **pOutFieldName);
static eBoolean_t GetPIValue(PI *pPI, char *pOutValue);
static eBoolean_t GetPIValueREF(PI *pPI, char **pOutValue);
static eBoolean_t GetPITagType(PI *pPI, char **pOutType);
static eBoolean_t SetPIValue(PI *pPI, char *pValue);
static eBoolean_t SetPIAValue(PI *pPI,char *pName, char *pOutValue);
static void RawOutputHTML(char *p);

static eBoolean_t DumpPI(PI *pPI);
static eBoolean_t DumpPIA(SYMBOL *pPIA);


/*
/* Global Variables used by all modules
/* but owned by this one!
 */
eBoolean_t gbCookFlag;
PAGE *gpPage;

/*
/* Global (local to this file) Variables...
 */
static LIST *galPI[MAX_TAG_LEVELS]
		    /* Array for storing the current PI List for
		    /*  each scope level
		     */
    ;
static PI   *gaPI[MAX_TAG_LEVELS]
    ;
eBoolean_t gbBufferOutput = eFalse;

eBoolean_t
swoutput(PAGE *pPage)
{
    DebugHTML(__FILE__,__LINE__,2,"swoutput(%x,%s)"
			,pPage
			,ISCOOKED ? "TRUE":"FALSE");
    /*
    /* Create the Global PAGE, if necessary,
     */
    /* Use the PAGE passed in
     */
    gpPage = pPage;

    /*
    /* Cook the raw page into something juicy...
     */
    if(ISCOOKED) {
	RETeFalse(CookPage(gpPage)
		 ,"Failed to expand SQLweb Page"
		 );
	if(ISeTrue(gbBufferOutput)) {
	    /* Time for desert....
	     */
	    if(giParseLevel==3) {
		DumpPage(gpPage);
		return(eTrue);
	    }
	    RETeFalse(OutputPage(gpPage),"OutputPage Failed");
	}
    } else {
	RETeFalse(OutputPage(gpPage),"OutputPage Failed");
    }
    /* OutputHTML("\n");
     */
    FlushHTMLBuf();
    return(eTrue);
}

static eBoolean_t
OutputPage(PAGE *pPage)
{
    DebugHTML(__FILE__,__LINE__,3,"OutputPage(%x)",pPage);
    if(!pPage)
	return(eTrue);
    PrintHTMLHeader("text/html");
    return( l_scan(pPage->lPI,(PFI)OutputPI) );
}

static eBoolean_t
OutputPI(PI *pPI)
{
    /* output tag
     */
    if(!pPI)
	return(eTrue);

    DebugHTML(__FILE__,__LINE__,6,"OutputPI(%s)",pPI->pTagName);

    if(isNotHidden(pPI)) {
	OutputHTML("<%s",pPI->pTagName);
	/*
	/* output TagAttributes
	 */
	RETeFalse(l_scan(pPI->lPIA,(PFI)OutputPIA)
		 ,"l_scan(PIA) OutputPIA Failed"
		 );
	/* output end-tag
	 */
	OutputHTML(">");
    }
    /* output (normal) contents
     */
    if(pPI && pPI->pPiContents && *(pPI->pPiContents)) {
        RawOutputHTML(pPI->pPiContents);
    }
    /* output lPI
     */
    RETeFalse( l_scan(pPI->lPI,(PFI)OutputPI)
	     ,"Recursive call failed"
	     );
    /*
    /* Output Trailer
     */
    if(!isEmpty(pPI) && isNotHidden(pPI)) {
	OutputHTML("</%s>",pPI->pTagName);
    }

    FlushHTMLBuf();
    return(eTrue);
}

static eBoolean_t
CookPIA(SYMBOL *pPIA)
{
    char *pBuf ;
    if(!pPIA)
	return(eTrue);

    DebugHTML(__FILE__,__LINE__,5,"CookPIA(%s,%x)",pPIA->pName,pPIA->pValue);

    /* Always expand the NAME= part of each attrib
     */
    RETeFalse2(ExpandString(pPIA->pName,&pBuf)
	      ,"ExpandString(%s) Failed"
	      ,pPIA->pName
	      );
    FreeBuf(pPIA->pName);
    pPIA->pName = pBuf;

    /*
    /* Only expand VALUE part when it's NOT EXPR= or SQL=
    /*  ... because the parser will deal with the : variables
     */
    if( !(is_casematch(pBuf,"EXPR") || is_casematch(pBuf,"SQL"))) {
	RETeFalse2(ExpandString(pPIA->pValue,&pBuf)
		  ,"ExpandString(%s) Failed"
		  ,pPIA->pValue
		  );
	FreeBuf(pPIA->pValue);
	pPIA->pValue = pBuf;
    } else {
    /*
    /* Only remove charcodes from SQL and EXPR attributes
    /*  ... because the parser doesn't deal with the &gt; syntax
     */
	RemoveCharCodes(pBuf);
    }
    return(eTrue);
}

static eBoolean_t
OutputPIA(SYMBOL *pPIA)
{
    return ( iPrintHTMLSymbol(pPIA) );
}

static eBoolean_t
CookPage(PAGE *pPage)
{
    if(!pPage)
	return(eTrue);
    DebugHTML(__FILE__,__LINE__,3,"CookPage(%x)",pPage);
    return( l_scan(pPage->lPI,(PFI)CookPI) );
}

eBoolean_t
CookPI(PI *pPI)
{
    char *pFieldName
    	,*pValue
	;
    TAG_HANDLE *pTagHandle;
    eBoolean_t bExpr = eTrue;

    if(pPI == NULL){
	MsgPush("Internal error: CookPI received NULL");
	return(eTrue);
/*	return(eFalse);
 */
    }
    DebugHTML(__FILE__,__LINE__,5,"CookPI(%x):%s (line %d)"
	     ,pPI
	     ,pPI->pTagName
	     ,pPI->iLineNbr
	     );

    for(pTagHandle= &gaTagHandlers[0]; pTagHandle->pTagName;pTagHandle++){
	if(is_casematch(pTagHandle->pTagName,pPI->pTagName)){
	    char *pBuf;
	    DebugHTML(__FILE__,__LINE__,2,"Handling:%s (line %d)"
		     ,pPI->pTagName
		     ,pPI->iLineNbr
		     );
	    /*
	    /* AUTOFAIL should be set for backward compatability...
	    /* May be set in the "INI" file or in "old" pages
	     */
	    if(ISeTrue(GetSymbolValueREF("SQLWEB_AUTOFAIL",&pBuf))
		&& is_casematch("TRUE",pBuf))
	    {
		RETeFalse3((pTagHandle->fHandler)(pPI,&bExpr)
			  ,"Tag Handler Failed:%s (line %d)"
			  ,pPI->pTagName
			  ,pPI->iLineNbr
			  );
	    } else if(ISeFalse((pTagHandle->fHandler)(pPI,&bExpr))) {
		DebugHTML(__FILE__,__LINE__,0
			,"ERROR: Tag Handler failed on %s (line %d)"
			,pPI->pTagName
			,pPI->iLineNbr
			);
		SELSym("SQLWEB_CODE","1");
		SELSym("SQLWEB_MESSAGE","Handler Failed");
		/* return(eFalse);
		 */
		return(eTrue);
	    }
	    SELSym("SQLWEB_CODE","0");
	    SELSym("SQLWEB_MESSAGE","Normal Successful Completion");

	    /* return(eTrue);
	     */
	}
    }
    if(ISeFalse(bExpr)){
	/*
	 */
	DebugHTML(__FILE__,__LINE__,3 ,"fHandler says FALSE:otta here!");
	/* ************************************************************
	/* ************************************************************
	/* BTW, don't reference pPI now, it has been FREE'd by fHandler
	 */

	/* if(ISeTrue(gbBufferOutput)) {
	/*     pPI->lPI = l_create("QUEUE");	/* Clobber list */
	/*     pPI->pPiContents = DupBuf(0);	/* Clobber contents */
	/* } else {
	/*     FreePI(pPI);
	/* }
	 */
	return(eTrue);
    }

    /*
    /* Expand Contents
     */
    RETeFalse2(ExpandContents(pPI)
             ,"ExpandContents(%s) Failed"
             ,pPI->pPiContents
             );

    /*
    /* Expand Attributes....
     */
    RETeFalse(l_scan(pPI->lPIA,(PFI)CookPIA)
	     ,"l_scan(PIA) CookPIA Failed"
	     );
    /*
    /* Process (INPUT/TEXTAREA)/OPTION Tags
     */
    if((( is_casematch(pPI->pTagName,"INPUT")
	||is_casematch(pPI->pTagName,"TEXTAREA"))
	   && (ISeTrue(GetPIFieldNameREF(pPI,&pFieldName)) && *pFieldName)
	   && ISeTrue(GetSymbolValueREF(pFieldName,&pValue)))
	||(is_casematch(pPI->pTagName,"OPTION")
	   && (ISeTrue(GetPIFieldNameREF(pPI,&pFieldName)) && *pFieldName)
	   && ISeTrue(GetPIValueREF(pPI,&pValue))
	   && ISeTrue(IsSymbolValue(pFieldName,pValue))))
    {
	if(ISeTrue(SetPIValue(pPI,pValue))) {
	    DebugHTML(__FILE__,__LINE__,5,"CookPI:SetPIValue(%s)OK",pFieldName);
	} else {
	    DebugHTML(__FILE__,__LINE__,1
		     ,"CookPI:W:SetPIValue(%s) Failed"
		     ,pFieldName
		     );
	}
    }
    if(ISeTrue(gbBufferOutput)){
	return(l_scan(pPI->lPI,(PFI)CookPI));
    }

    /********************************************
    /* From here on we are only Non-Buffered Output ...
     */

    /*
    /* Output <TAG and Attributes>
     */
    if(isNotHidden(pPI)) {
	OutputHTML("<%s",pPI->pTagName);
	RETeFalse(l_scan(pPI->lPIA,(PFI)OutputPIA)
		 ,"l_scan(PIA) OutputPIA Failed"
		 );
	OutputHTML(">");
    }
    /*
    /* Output TAG Contents
     */
    if(pPI && pPI->pPiContents && *(pPI->pPiContents))
	RawOutputHTML(pPI->pPiContents);

    /*
    /* COOK and Output Subordinate TAGS, the children
     */
    RETeFalse(l_scan(pPI->lPI,(PFI)CookPI)
	     ,"l_scan(pPI->lPI,CookPI) Failed"
	     );

    /*
    /* Output the ENDING TAG </TAG>
     */
    if(!isEmpty(pPI) && isNotHidden(pPI)) {
	OutputHTML("</%s>",pPI->pTagName);
    }
/*
 */
    FreePI(pPI);

    /* Don't forget to Flush!
     */
    FlushHTMLBuf();
    return(eTrue);
}

static eBoolean_t
GetPIFieldNameREF
    (PI *pPI
    ,char **pOutFieldName
    )
{
    DebugHTML(__FILE__,__LINE__,6,"GetPIFieldNameREF(%x,%s,?)",pPI,pPI->pTagName);
    if(is_casematch(pPI->pTagName,"INPUT")
       || is_casematch(pPI->pTagName,"TEXTAREA")
       || is_casematch(pPI->pTagName,"SELECT"))
    {
	if(ISeTrue(GetPIAValueREF(pPI,"NAME",pOutFieldName))) {
	    return(eTrue);
	}
    }
    else if(is_casematch(pPI->pTagName,"OPTION")){
	/* For options, since they are nested below SELECTs must
	/* use the PARENT's i.e. pPIContext' Attirb list to determine
	/* the NAME
	/* DebugHTML(__FILE__,__LINE__,5,"dump Core!(%x,%x)"
	/* 		,pPI
	/* 		,pPI->pPIContext);
	/* FlushHTMLBuf();
	 */
	PI *pPI2;
	for(pPI2=pPI->pPIContext;pPI2;pPI2=pPI2->pPIContext) {
	    if(ISeTrue(GetPIAValueREF(pPI2,"NAME",pOutFieldName))){
		return(eTrue);
	    }
	}
    }
    *pOutFieldName=0;
    return(eFalse);
}

/* static eBoolean_t
/* GetPIValue
/*     (PI *pPI
/*     ,char *pOutValue
/*     )
/* {
/*     /* Basically, look for a VALUE attribute
/*      */
/*     DebugHTML(__FILE__,__LINE__,5,"GetPIValue(%s)",pPI->pTagName);
/*     if(ISeTrue(GetPIAValue(pPI,"VALUE",pOutValue))){
/* 	/* Got IT! */
/* 	return(eTrue);
/*     }
/*     /* Only OPTIONS use the Contents as the backup value holder...
/*      */
/*     if(is_casematch(pPI->pTagName,"OPTION") && pPI->pPiContents){
/* 	DebugHTML(__FILE__,__LINE__,6,"GetPIValue(%s)",pPI->pPiContents);
/* 	strcpy(pOutValue,pPI->pPiContents);
/* 	return(eTrue);
/*     }
/*     
/*     /*
/*     /* Couln't find the value...
/*      */
/*     *pOutValue=0;
/*     return(eFalse);
/* }
/* */
static eBoolean_t
GetPIValueREF
    (PI *pPI
    ,char **pOutValue
    )
{
    /* Basically, look for a VALUE attribute
     */
    DebugHTML(__FILE__,__LINE__,5,"GetPIValueREF(%s)",pPI->pTagName);
    if(ISeTrue(GetPIAValueREF(pPI,"VALUE",pOutValue))){
	/* Got IT! */
	return(eTrue);
    }
    /* Only OPTIONS use the Contents as the backup value holder...
     */
    if(is_casematch(pPI->pTagName,"OPTION") && pPI->pPiContents){
	DebugHTML(__FILE__,__LINE__,6,"GetPIValueREF(%s)",pPI->pPiContents);
	(*pOutValue)=pPI->pPiContents;
	return(eTrue);
    }
    
    /*
    /* Couln't find the value...
     */
    (*pOutValue)=DupBuf(0);
    return(eFalse);
}

static eBoolean_t
GetPITagType
    (PI *pPI
    ,char **pOutType
    )
{
    if(ISeTrue(GetPIAValueREF(pPI,"TYPE",pOutType))) {
	/* Got it! */
	return(eTrue);
    }
    /* Default is TEXT
     */
    (*pOutType) = "TEXT";
    return(eTrue);
}

static eBoolean_t
SetPIValue
    (PI *pPI
    ,char *pValue
    )
{
    char *pType
	,*pValue2
	;

    DebugHTML(__FILE__,__LINE__,5,"SetPIValue(%s,%s)",pPI->pTagName,pValue);

    if(is_casematch(pPI->pTagName,"INPUT")) {
	RETeFalse(GetPITagType(pPI,&pType)
		 ,"GetPITagType failed"
		 );
	/* TYPE in(TEXT,PASSWORD)
	/* set VALUE Attribute to pValue
	 */
	if(    is_casematch(pType,"TEXT")
	    || is_casematch(pType,"PASSWORD")
	    || is_casematch(pType,"HIDDEN"))
	{
	    if(!pValue || !(*pValue)){
		/* Delete VALUE Attrib, since it's value is NULL
		/* If it works, it works!
		 */
		SYMBOL *p;
		p=l_delete(pPI->lPIA,l_find,iCmpSymbolpName,"VALUE");
		if(p){
		    FreePIA(p);
		}
	    } else {
		DebugHTML(__FILE__,__LINE__,5,"SettingV(%s,%s)",pType,pValue);
		return( SetPIAValue(pPI,"VALUE",pValue));
	    }
	}
	/* TYPE in(CHECKBOX,RADIO)
	/* Set CHECKED Attribute
	 */
	if(is_casematch(pType,"RADIO") || is_casematch(pType,"CHECKBOX")){
	    if(ISeFalse(GetPIValueREF(pPI,&pValue2))){
		pValue= "on"; /* Value is set to "on" when
			     /* there is no VALUE attrib */
	    }
	    /*
	    /* On a MATCH, SET/Create the CHECKED ATTRIBUTE
	     */
	    if(is_match(pValue2,pValue)){
		return( SetPIAValue(pPI,"CHECKED",""));
	    /*
	    /* No Match -- make sure the CHECKED ATTRIBUTE is Gone!
	     */
	    } else {
		SYMBOL *p;
		p=l_delete(pPI->lPIA,l_find,iCmpSymbolpName,"CHECKED");
		if(p) {
		    FreePIA(p);
		}
	    }
	    return(eTrue);
	}
    }
    if(is_casematch(pPI->pTagName,"TEXTAREA")){
	while(*pValue == '\n') ++pValue;
		/* Mosaic tends to stick leading '\n' is there
		 */
	FreeBuf(pPI->pPiContents);
	pPI->pPiContents = DupBuf(pValue);
	return(eTrue);
	/* strcpy(pOutValue,pPI->pPiContents);
	 */
    }
    /*
    /* OPTION TAG that has a VALUE
     */
    if(is_casematch(pPI->pTagName,"OPTION")
	&& ISeTrue(GetPIValueREF(pPI,&pValue2)))
    {
	/*
	/* IF Symbol VALUE Matches this OPTION
	/* Set the SELECTED ATTRIBUTE
	 */
	if( is_match(pValue2,pValue)) {
	    if(ISeFalse( SetPIAValue(pPI,"SELECTED",""))){
		MsgPush("SetPIAValue failed on OPTION");
		return(eFalse);
	    } else {
		DebugHTML(__FILE__,__LINE__,5,"Set OPTION:%s",pValue);
		return(eTrue);
	    }
	/*
	/* IF the VALUES Don't match make sure the 
	/* SELECTED ATTRIBUTE is DELETED
	 */
	} else {
	    SYMBOL *p;
	    /* If they don't match, erase the SELECTED attribute 
	     */
	    p=l_delete(pPI->lPIA,l_find,iCmpSymbolpName,"SELECTED");
	    if(p){
		FreePIA(p);
	    }
	}
    }
    return(eTrue);
}

/* eBoolean_t
/* GetPIAValue
/*     (PI *pPI
/*     ,char *pName
/*     ,char *pOutValue
/*     )
/* {
/*     SYMBOL *pSym;
/*     DebugHTML(__FILE__,__LINE__,5,"GetPIAValue(%x,%s,...)",pPI,pName);
/* 
/*     if(!pPI) {
/* 	*pOutValue=0;
/* 	return(eFalse);
/*     }
/* 
/*     pSym=l_find(pPI->lPIA,iCmpSymbolpName,pName);
/*     if(!pSym){
/* 	*pOutValue=0;
/* 	return(eFalse);
/*     }
/*     strcpy(pOutValue,pSym->pValue);
/*     return(eTrue);
/* }
 */
eBoolean_t
GetPIAValueREF
    (PI *pPI
    ,char *pName
    ,char **pOutValue
    )
{
    SYMBOL *pSym;
    DebugHTML(__FILE__,__LINE__,5,"GetPIAValueREF(%x,%s,...)",pPI,pName);

    if(!pPI) {
	(*pOutValue)=DupBuf(0);
	return(eFalse);
    }

    pSym=l_find(pPI->lPIA,iCmpSymbolpName,pName);
    if(!pSym){
	(*pOutValue)=DupBuf(0);
	return(eFalse);
    }
    (*pOutValue) = pSym->pValue;
    return(eTrue);
}

static eBoolean_t
SetPIAValue
    (PI *pPI
    ,char *pName
    ,char *pValue
    )
{
    SYMBOL *pSym;

    DebugHTML(__FILE__,__LINE__,6,"setPIAValue(%s,%s)",pName,pValue);

    pSym=l_find(pPI->lPIA,iCmpSymbolpName,pName);
    if(!pSym){
	pSym = NewPIA();
	if(!pSym){
	    MsgPush("NewPIA failed");
	    return(eFalse);
	}
	/* (void)memset((pSym),0,sizeof(SYMBOL));
	 */
	pSym->iType=0;	/* Not used, best init it, though */
	pSym->pName=DupBuf(pName);
	pSym->pValue=DupBuf(pValue);
	return( ENQ(pPI->lPIA,pSym) );
    }
    /* toss origional value
     */
    FreeBuf(pSym->pValue);
    pSym->pValue = DupBuf(pValue);
    return(eTrue);
}

static void
RawOutputHTML(char *p)
{
    FlushHTMLBuf();
    fprintf(stdout,"%s",p);
    fflush (stdout);
    return;
}

eBoolean_t
DumpPage(PAGE *pPage)
{
    eBoolean_t bRet;
    bRet=l_scan(pPage->lPI,(PFI)DumpPI);
    fprintf(stdout,"\n");
    return(bRet);
}
static eBoolean_t
DumpPI(PI *pPI)
{
    if(!pPI)
	return(eTrue);

    fprintf(stdout,"\n%*s%s (line=%d) (depth=%d) "
	   ,(pPI->iLevel)*2 ," "
	   ,pPI->pTagName
	   ,pPI->iLineNbr
	   ,pPI->iLevel
	   );
    (void)l_scan(pPI->lPIA,(PFI)DumpPIA);

    if(l_size(pPI->lPI)>0) {
	RETeFalse( l_scan(pPI->lPI,(PFI)DumpPI)
		 ,"Recursive call failed"
		 );

	fprintf(stdout,"\n%*s/%s"
		,(pPI->iLevel)*2 ," "
		,pPI->pTagName
		);
    }
    return(eTrue);
}
static eBoolean_t
DumpPIA(SYMBOL *pPIA)
{
    char sBuf[BUFSIZ];

    (void)memset(sBuf,0,BUFSIZ);
    if(pPIA->pValue)
	strncpy(sBuf,pPIA->pValue,10);
    fprintf(stdout,"(%s=%s)",pPIA->pName,sBuf);
    return(eTrue);
}
