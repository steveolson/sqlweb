/* ldhtml.c - loads and HTML file
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
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

/*
/* Local Includes 
 */
#include "sqlweb.h"

/*
/* TOKENS
 */
typedef enum {
     OpenTag_t
    ,OpenEndTag_t
    ,OpenComment_t
    ,EndTag_t
    ,AttAssign_t
    ,String_t
    ,EndOfFile_t
} eToken_t;

/*
/* STATES
 */
typedef enum {
     Init_s
    ,InBetween_s
    ,InTag_s
    ,InAtt_s
    ,InAttAssign_s
    ,InValue_s
    ,InContents_s
    ,InComment_s
    ,InEndTag_s
    ,InEndTag2_s
    ,Err_s
    ,EndOfFile_s
} eParseState_t;

/*
/* Global Variables, basically, in a STATE MACHINE everything
/* is global!
 */
static char gsStringBuf[MAX_BUFSIZ];		/* String from "TOKENIZER" */
static char *gpStringBuf;			/* Current Location in memory-
						/* mapped file */
static eParseState_t gsParseState =Init_s;	/* The STATE */
static eToken_t gtToken;			/* The TOKEN */
static TAG *gpTAG;				/* The TAG */
static SYMBOL *gpSym;				/* The Attribute */
static PI *gpPI;				/* The PageItem */
static PAGE *gpPAGE;				/* The PAGE */
static LIST *galPI[MAX_TAG_LEVELS];		/* The PageItem List Array */
static PI   *gaPI[MAX_TAG_LEVELS];		/* The PI Array (context) */
static int giLevel				/* The Level */
	  ,giErr = 0				/* Error counter */
	  ,giLineNbr = 1			/* Line Counter */
	;
static eBoolean_t
	   gbFatal = eFalse			/* Fatal Errors */
	;

static TAG gNULLTag = 
    {"NULL"	/* pTagName */
    ,"Y"	/* pTagEmptyInd */
    ,""		/* pTagDesc */
    ,"A"	/* pTagAfterInd(HIDDEN) */
    ,""		/* pTagLinkDesc */
    };

#define EXACT_TOKENS 7
#define EXACT_STATES 12
/* #define MAX_PARSE_ERRORS 25
 */

/*
/* State Transition Functions
 */
eParseState_t	 f011(),f012(),f013(),f014(),f015(),f016()
		,f021(),f022(),f023(),f024(),f025(),f026()
		,f031(),f032(),f033(),f034(),f035(),f036()
		,f041(),f042(),f043(),f044(),f045(),f046()
		,f051(),f052(),f053(),f054(),f055(),f056()
		,f061(),f062(),f063(),f064(),f065(),f066()
		,f071(),f072(),f073(),f074(),f075(),f076()
		,f081(),f082(),f083(),f084(),f085(),f086()
		,f091(),f092(),f093(),f094(),f095(),f096()
		,f101(),f102(),f103(),f104(),f105(),f106()
		,f111(),f112(),f113(),f114(),f115(),f116()
		,f_eof()/* end of file */
	;

/*
/* The STATE MACHINE!
 */
typedef eParseState_t (*PFS_t)(); /* Pointer to Function returning STATE */
PFS_t gaTransTab [ EXACT_STATES ] [ EXACT_TOKENS ]  = {
/*		   Open-  Open-   Open-           Att-
/*		   Tag    EndTag  Comment  EndTag Assign    STRING EOF
/*		   ===    ======  =======  ====== ======    ====== ===
/* Init_s	*/ f011  ,f012   ,f013    ,f014  ,f015     ,f016  ,f_eof
/* InBetween_s	*/,f021  ,f022   ,f023    ,f024  ,f025     ,f026  ,f_eof
/* InTag_s	*/,f031  ,f032   ,f033    ,f034  ,f035     ,f036  ,f_eof
/* InAtt_s	*/,f041  ,f042   ,f043    ,f044  ,f045     ,f046  ,f_eof
/* InAttAssign_s*/,f051  ,f052   ,f053    ,f054  ,f055     ,f056  ,f_eof
/* InValue_s	*/,f061  ,f062   ,f063    ,f064  ,f065     ,f066  ,f_eof
/* InContents_s	*/,f071  ,f072   ,f073    ,f074  ,f075     ,f076  ,f_eof
/* InComment_s	*/,f081  ,f082   ,f083    ,f084  ,f085     ,f086  ,f_eof
/* InEndTag_s	*/,f091  ,f092   ,f093    ,f094  ,f095     ,f096  ,f_eof
/* InEndTag2_s	*/,f101  ,f102   ,f103    ,f104  ,f105     ,f106  ,f_eof
/* Err_s	*/,f111  ,f112   ,f113    ,f114  ,f115     ,f116  ,f_eof
};

static eBoolean_t ParseStream();
static eToken_t GetNextToken();
static eBoolean_t mk_PI(char *pTagName, char *pPiContents);
static void ParseErr(const char *pFmt, ...);

/*
/* Routines for processing input characters
 */
static int EatSpaces(int c);
static int GetC();
static void unGetC(int c);

/*
/*  Here we go.....
 */

/* The main interface routine.  This function takes the SQLweb HTML
/* file and builds the in-memory PAGE / PI Tree that is represented
/* by it.
 */
eBoolean_t
LoadHTML(char *pFilename
	,PAGE **pout_Page)
{
    eBoolean_t bParseRet;
    PI PI;		/* temporary PI for loading File Text */
    char sBuf[BUFSIZ];	/* for dumping ERRStack */

    RETeFalse2(LoadTEXT(pFilename,"HTML",&PI)
	     ,"LoadHTML Failed on %s"
	     ,pFilename
	     );

    (*pout_Page) = (PAGE*)malloc(sizeof(PAGE));
    if(!(*pout_Page)){
	ParseErr("malloc failed in parser");
	return(eFalse);
    }
    (void)memset((*pout_Page),0,sizeof(PAGE));

    (*pout_Page)->lPI = galPI[0] = l_create("QUEUE"); /* Level 0:Page List*/
    (*pout_Page)->pFileText = gpStringBuf = PI.pPiContents;

    bParseRet = ParseStream();


    /* This method, pop's off ALL Parse Errors
    /* and if we are NOT COOKING the page it
    /* displays the errors as HTML Comments
    /* are silently ignored.....
     */
    while( MsgPop(sBuf) ) {
	if(ISCOOKED) {
	    DebugHTML(__FILE__,__LINE__,0,"%s",sBuf);
	} else {
	    fprintf(stderr,"%s\n",sBuf);
	}
    }
    return(bParseRet);
}

/*
/* The file parser.  runs "The State Machine"
 */
static eBoolean_t
ParseStream()
{
    giErr=0;		/* Global Error Count */
    gbFatal=eFalse;	/* Fatal Error Flag */
    giLevel=0;		/* Global PageItem Level */
    giLineNbr=1;	/* Line Number */
    gsParseState = Init_s;
    do {
	gtToken = GetNextToken();
	gsParseState = (gaTransTab[gsParseState][gtToken])();
    } while( gsParseState != EndOfFile_s);

    return( ISeTrue(gbFatal) ? eFalse : eTrue );
}

/*
/* The Tokenizer
 */
static eToken_t
GetNextToken()
{
    int c;
    char *pBuf=gsStringBuf;
    eBoolean_t bQuote=eFalse;

    /*
    /* Get the first character
     */
    c=GetC();

    /*
    /* Initialize the global STRING BUFFER 
     */
    *pBuf=0;

    if(c==EOF){
	return(EndOfFile_t);
    }

    if(gsParseState==InAttAssign_s && c=='=') {
	return( AttAssign_t );
    }

    if(c == '<'){
	c=GetC();

	if(c=='!') {
	    return(OpenComment_t);
	} if(c=='/') {
	    return(OpenEndTag_t);
	} 
	unGetC(c);
	return(OpenTag_t);
    }
    if(c == '>') {
	return(EndTag_t);
    }

    for( ;c!=EOF && (pBuf-gsStringBuf)<sizeof(gsStringBuf);c=GetC()){
	/*
	/* Deal with Quotes (excluding InContents_s and InComments_s)
	 */
	if(c == '"'
	    && (  gsParseState==InAtt_s
		||gsParseState==InAttAssign_s
		||gsParseState==InValue_s))
	{
	     bQuote = ISeTrue(bQuote) ? eFalse :eTrue;
		    /* Toggle Quote Flag
		     */
	     continue;	/* And eat it! */
	}

	if(c == '\\' && ISeTrue(bQuote)) {
	    int c2 = GetC();
	    if(c2=='"'){
		*pBuf++ = (char) c2;
		continue;
	    }
	    unGetC(c2);
	}

	/* Deal with < and > 'tings
	 */
	if((c == '<' || c == '>')
	    && ISeFalse(bQuote))
	{
	    unGetC(c);
	    *pBuf = 0;
	    return(String_t);
	}

	/*
	/* The Attribute Assignment
	 */
	if(c == '=' && ISeFalse(bQuote)
	    && (gsParseState==InAtt_s || gsParseState==InAttAssign_s))
	{
	    unGetC(c);
	    *pBuf = 0;
	    return(String_t);
	}

	/*
	/* White space separator within <> pairs...
	 */
	if(isspace(c) 
	    && ISeFalse(bQuote)
	    &&( gsParseState==InTag_s
	     || gsParseState==InAtt_s
	     || gsParseState==InAttAssign_s
	     || gsParseState==InValue_s))
	{
	    *pBuf=0;
	    c=EatSpaces(c);
	    unGetC(c);
	    if( pBuf==gsStringBuf )
		continue;
	    return(String_t);
	}

	/* Fillin the Buffer....
	 */
	*pBuf++ = (char) c;
    }
    if( pBuf==gsStringBuf )
	return(EndOfFile_t);

    *pBuf=0;
    unGetC(c);
    return(String_t);
}

/*
/* Eat Spaces between TAGS
 */
static int
EatSpaces(int c)
{
    do{ c=GetC();
      } while(isspace(c));
    return(c);
}

eParseState_t f_eof(){ return(EndOfFile_s); }

/*
/* The Init_s State
 */
eParseState_t f011(){return(InTag_s);}
eParseState_t f012(){return(Init_s);}
eParseState_t f013(){return(InComment_s);}
eParseState_t f014(){return(Init_s);}
eParseState_t f015(){return(Init_s);}
eParseState_t f016(){return(Init_s);}

/*
/* The InBetween State
 */
eParseState_t f021(){return(InTag_s);}
eParseState_t f022(){return(InEndTag_s);}
eParseState_t f023(){return(InComment_s);}
eParseState_t f024(){ParseErr("Encountered unmatched 'gt'");return(Err_s); }
eParseState_t f025(){ParseErr("Internal error got '=' inBTWN");return(Err_s);}
eParseState_t f026()
{
/*
/*    int iWLen = strspn(gsStringBuf," \t\n\r");
/*    if(iWLen=iStrLen(gsStringBuf))
/*	return(InBetween_s);
 */

    if(ISeFalse(mk_PI("NULL",gsStringBuf))) {
	ParseErr("FATAL Error:mk_PI(NULL,%%s) failed");
	gbFatal=eTrue;
	return(Err_s);
    }
    return(InBetween_s);
}

/*
/* The InTag State
 */
eParseState_t f031(){ParseErr("Got 'lt' within a tag")   ;return(Err_s);}
eParseState_t f032(){ParseErr("Got 'lt+/' within a tag") ;return(Err_s);}
eParseState_t f033(){ParseErr("Got 'lt+!' within a tag") ;return(Err_s);}
eParseState_t f034(){ParseErr("Got 'lt' before tag name");return(Err_s);}
eParseState_t f035(){ParseErr("Got '=' before tag name") ;return(Err_s);}
eParseState_t f036()
{
    if(ISeFalse(mk_PI(gsStringBuf,0))) {
	ParseErr("ERR:InTag:mk_PI(%s,null) failed",gsStringBuf);
	gbFatal=eTrue;
	return(Err_s);
    }
    return(InAtt_s);
}

/*
/* The InAtt State
 */
eParseState_t f041(){ParseErr("Got 'lt' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
eParseState_t f042(){ParseErr("Got 'lt+/' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
eParseState_t f043(){ParseErr("Got 'lt+!' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
/*eParseState_t f044(){if(!isEmpty((gpPI))) return(InContents_s);
/*		     else		  return(InBetween_s);
/*		    }
 */
eParseState_t f044(){if(!isEmpty((gpPI))) {
			/* giLevel++;
			/* galPI[giLevel] = gpPI->lPI;
			 */
			return(InBetween_s);
		    }
		     else return(InContents_s);
		    }
eParseState_t f045(){ParseErr("Got '=' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
eParseState_t f046()
{
    gpSym = NewPIA();
    if(!gpSym){
	ParseErr("NewPIA Failed in parser");
	gbFatal=eTrue;
	return(Err_s);
    }
    /* (void)memset(gpSym,0,sizeof(SYMBOL));
     */
    gpSym->iType = PIA_SYMBOL;
    gpSym->pName = DupBuf(gsStringBuf);
    gpSym->pValue= 0;

    ENQ(gpPI->lPIA,gpSym);
    return(InAttAssign_s);
}

/*
/* The InAttAssign State
 */
eParseState_t f051(){ParseErr("Got 'lt' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
eParseState_t f052(){ParseErr("Got 'lt+/' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
eParseState_t f053(){ParseErr("Got 'lt+!' within tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s); 
		    }
/* eParseState_t f054(){if(!isEmpty((gpPI))) return(InContents_s);
/* 		     else		  return(InBetween_s);
/* 		    }
 */
eParseState_t f054(){if(!isEmpty((gpPI))) {
			/* giLevel++;
			/* galPI[giLevel] = gpPI->lPI;
			 */
			return(InBetween_s);
		    }
		     else return(InContents_s);
		    }
eParseState_t f055(){return(InValue_s);}
eParseState_t f056()
{
    gpSym = NewPIA();
    if(!gpSym){
	ParseErr("NewPIA Failed to parser");
	gbFatal=eTrue;
	return(Err_s);
    }
    (void)memset(gpSym,0,sizeof(SYMBOL));
    gpSym->iType = PIA_SYMBOL;
    gpSym->pName = DupBuf(gsStringBuf);
    gpSym->pValue= 0;

    ENQ(gpPI->lPIA,gpSym);
    return(InAttAssign_s);
}

/*
/* The InValue State
 */
eParseState_t f061(){ParseErr("Got 'lt' within tag, \"%s\"" ,gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f062(){ParseErr("Got 'lt+/' within tag, \"%s\"" ,gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f063(){ParseErr("Got 'lt+!' within tag, \"%s\"" ,gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f064(){ParseErr("Got 'gt' within attribute assign of tag, \"%s\""
			    ,gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f065(){ParseErr("Got '= =' within tag, \"%s\"" ,gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f066(){
		    gpSym->pValue = DupBuf(gsStringBuf);
		    return(InAtt_s);
		    }

/*
/* The InContents State
 */
eParseState_t f071(){return(InTag_s); }
eParseState_t f072(){return(InEndTag_s); }
eParseState_t f073(){return(InComment_s); }
eParseState_t f074(){ParseErr("Got 'gt' after tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f075(){ParseErr("Got '=' after tag, \"%s\"",gpTAG->pTagName);
		    return(Err_s);
		    }
eParseState_t f076(){
		    gpPI->pPiContents = DupBuf(gsStringBuf);
		    return(InBetween_s); 
		    }

/*
/* The InComment State
 */
eParseState_t f081(){return(InComment_s); }
eParseState_t f082(){return(InComment_s); }
eParseState_t f083(){return(InComment_s); }
eParseState_t f084(){return(InBetween_s); }
eParseState_t f085(){return(InComment_s); }
eParseState_t f086(){
    int iSize = iStrLen(gsStringBuf);

    gsStringBuf[iSize+3]=0;	/* NULL */
    gsStringBuf[iSize+2]='>';	/* Closing of Tag '>' */
    while( --iSize>=0 )		/* The Contents */
	gsStringBuf[iSize+2] = gsStringBuf[iSize];
    gsStringBuf[1] = '!';	/* Comment identifier */
    gsStringBuf[0] = '<';	/* Starting TAG symbol */

    if(*gsStringBuf && ISeFalse(mk_PI("!",gsStringBuf))) {
	ParseErr("ERR:InComment:Failed to make COMMENT");
	gbFatal=eTrue;
	return(Err_s);
    }
    return(InComment_s);
} /* was InEndTag2 */

/*
/* The InEndTag State, just got '</' now looking for TAG-NAME
 */
eParseState_t f091(){ParseErr("missing tag name in trailer");return(Err_s); }
eParseState_t f092(){ParseErr("missing tag name in trailer");return(Err_s); }
eParseState_t f093(){ParseErr("missing tag name in trailer");return(Err_s); }
eParseState_t f094(){ParseErr("missing tag name in trailer");return(Err_s); }
eParseState_t f095(){ParseErr("missing tag name in trailer");return(Err_s); }
eParseState_t f096()
{
    int iLevel = giLevel;
    TAG *pTAG;

    /* Make sure Tag was non-empty...
     */
    if(ISeFalse(GetTAGByName(gsStringBuf,&pTAG))) {
    /*
    /* May want to use NULL, here???
     */
	MsgPush("Unknown tag, \"%s\"", gsStringBuf);
	return(Err_s);
    }
    if(is_casematch(pTAG->pTagEmptyInd,"Y")) {
	/* Empty TAG with end tag, WARNING...
	 */
	MsgPush("WARN(%s):Empty Tag w/END-TAG ignored, \"%s\" near line %d"
		,pTAG->pTagEmptyInd
		,gsStringBuf
		,giLineNbr
	        ,gaPI[(giLevel>0)?giLevel-1:0]->pTagName
	        ,(giLevel>0)?giLevel-1:0
		);
	return(InEndTag2_s);
    }
    
    /*
    /* OK, Tag was non-empty, let's find it on the stack
     */
    while(--giLevel>=0
	&& !is_casematch(gaPI[giLevel]->pTagName,gsStringBuf))
    {
	/*
	DebugHTML(__FILE__,__LINE__,1
	,"WARN(%s): Possible missing END-TAG, \"%s\" near line %d"
	       ,gaPI[giLevel]->pTag->pTagEmptyInd
	       ,gaPI[giLevel]->pTagName
	       ,giLineNbr
	       );
	 */
	MsgPush("WARN(%s): Possible missing END-TAG, \"%s\" near line %d"
	       ,gaPI[giLevel]->pTag->pTagEmptyInd
	       ,gaPI[giLevel]->pTagName
	       ,giLineNbr
	       );
    }
    if(giLevel<0) {
	MsgPush("Failed to find matching TAG for END-TAG, \"%s\" at line %d"
	       ,gsStringBuf
	       ,giLineNbr
	       );
	/* giLevel = iLevel-1;
	 */
	giLevel = iLevel;	/* Couldn't find it; don't pop */
	return(Err_s);
    }
    return(InEndTag2_s);
}

/*
/* The InEndTag2 State (looking for '>')
 */
eParseState_t f101(){ParseErr("Failed to find 'gt'");return(Err_s);}
eParseState_t f102(){ParseErr("Failed to find 'gt'");return(Err_s);}
eParseState_t f103(){ParseErr("Failed to find 'gt'");return(Err_s);}
eParseState_t f104(){return(InBetween_s);}
eParseState_t f105(){ParseErr("Failed to find 'gt'");return(Err_s);}
eParseState_t f106(){ParseErr("Failed to find 'gt'");return(Err_s);}

/*
/* The inErr State
 */
eParseState_t f111(){return(ISeTrue(gbFatal)?EndOfFile_s:InTag_s);}
eParseState_t f112(){return(ISeTrue(gbFatal)?EndOfFile_s:InEndTag_s);}
eParseState_t f113(){return(ISeTrue(gbFatal)?EndOfFile_s:InComment_s);}
eParseState_t f114(){return(ISeTrue(gbFatal)?EndOfFile_s:InBetween_s);}
eParseState_t f115(){return(ISeTrue(gbFatal)?EndOfFile_s:Err_s);}
eParseState_t f116(){return(ISeTrue(gbFatal)?EndOfFile_s:InBetween_s);}

static eBoolean_t
mk_PI(char *pTagName
     ,char *pPiContents
     )
{
    DebugHTML(__FILE__,__LINE__,4
		,"mk_PI(%s,%s) line=%d"
		,pTagName
		,pPiContents?pPiContents:"**Null**"
		,giLineNbr
		);
    (gpPI) = NewPI();
    if(!gpPI) {
	ParseErr("NewPI Failed in parser");
	gbFatal=eTrue;
	return(Err_s);
    }

    (gpPI)->iLevel = giLevel;
    (gpPI)->pTagName = DupBuf(pTagName);

    (gpPI)->pPage = gpPAGE;
    (gpPI)->pTag = gpTAG;
    (gpPI)->lPI = l_create("QUEUE");
    (gpPI)->lPIA= l_create("INORDER");
    (gpPI)->iLineNbr = giLineNbr;


    if(is_casematch(pTagName,"NULL")){
	gpTAG = &gNULLTag;
    } else {
	if(ISeFalse(GetTAGByName(pTagName,&gpTAG))) {
	/*
	/* May want to use NULL, here???
	 */
	    ParseErr("Unknown tag, \"%s\"", gsStringBuf);
	    return(eFalse);
	}
    }
    (gpPI)->pTag = gpTAG;
    ENQ(galPI[giLevel], gpPI);	/* Put PI in correct PI List */

    gaPI[giLevel] = gpPI;	/* Store pPI in PI Array */
    gpPI->pPIContext = giLevel>0 ? gaPI[giLevel-1]: 0;
				/* Build the CONTEXT pointer */

    (gpPI)->pPiContents = DupBuf(pPiContents);

    if(!isEmpty((gpPI))) {
	giLevel++;
	galPI[giLevel] = gpPI->lPI;
    }
    return(eTrue);
}

/*
/* Report a Parse Error
 */
static void
ParseErr
    (
    const char *pFmt
    ,...
    )
{
    va_list pParms;
    char sBuf[BUFSIZ];

    /*
    /* Build PrintF style buffer
     */
    va_start(pParms,pFmt);
    vsprintf(sBuf,pFmt,pParms);

    /* Add Line Number information
     */
    MsgPush("ParseErr line: %.4d: %s",giLineNbr,sBuf);
}

static int
GetC()
{
    int c;
    c= (int) *gpStringBuf++;
    if(c=='\n') giLineNbr++;
    if(c=='\0') return(EOF);
    return(c);
}

static void
unGetC(int c)
{
    *(--gpStringBuf) = (char)c;
}
