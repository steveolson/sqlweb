/*  sqlweb.h - main sqlweb header file
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

#ifndef _SQLWEB_H_
#define _SQLWEB_H_

/*
/* This includes all other needed includes for SQLweb
 */
#ifdef USE_LIBMALLOC
#include <malloc.h>
#endif

#include "list.h"
#include "boolean.h"
#include "sqlwebdb.h"
#include "scalar.h"

#define MAX_METHOD 31
#define MAX_CHAR_MNEMONIC 128
#define MAX_CONTENT_TYPE 256
#define MAX_TAG_LEVELS 500
#define MAX_TOK_SIZE 255
#define MAX_TOKVAL_SIZE 32768
#define MAX_HTMLBUF_SIZE 65536
#define MAX_BUFSIZ MAX_HTMLBUF_SIZE
#define MAX_INTEGER_DIGITS 45 /* MAX number of digits for an integer */

/*
/* Defines...
 */

/*
/* Symbol iTypes
 */
#define ENV_SYMBOL	1
#define INI_SYMBOL	2
#define FRM_SYMBOL	4
#define SEL_SYMBOL	8
#define BND_SYMBOL	16
#define PIA_SYMBOL	32
#define RAW_SYMBOL	64

typedef struct SymTab {
    int	 iType			/* Symbol Type ENV_SYMBOL, INI_SYMBOL, ... */
	;
    char *pName			/* Symbol Name */
	,*pValue		/* Symbol Value */
	;
    eScalar_t esDataType;	/* Datatype of Symbol */
    long lSize;			/* Size of Data storred */
} SYMBOL;

/*
/* Typedefs and such...
 */

/*
/* An HTML PAGE 
 */
typedef struct HTML_PAGES {
    LIST *lPI;		/* 1st Level PAGE_ITEMS, usually HTML */
    char *pFileText;	/* origional file */
} PAGE;

/*
/* A HTML TAG 
 */
typedef struct HTML_TAGS {
    char *pTagName
	,*pTagEmptyInd
	,*pTagDesc
	,*pTagAfterInd
	,*pTagLinkDesc
	;
}TAG;

/*
/* The Famous PAGE_ITEM
 */
typedef struct HTML_PAGE_ITEMS {
    int iLevel ;
    char  *pTagName
	,*pPiContents
	;
    struct HTML_PAGE_ITEMS *pPIContext;
    PAGE *pPage;
    TAG *pTag;
    LIST *lPIA		/* Attribute List */
	,*lPI		/* PageItem  List */
	;
    int iLineNbr;	/* Line number of tag starting of tag */
} PI;

/*
/* Some useful macros...
 */
/* #define is_match(a,b)	((strcmp(a,b))==0)
 */
#define is_match(a,b)	ISeTrue(bStrCaseMatch(a,b))
/* #define is_casematch(a,b) ((strcasecmp(a,b))==0)
 */
#define is_casematch(a,b) ISeTrue(bStrCaseMatch(a,b))
#define isEmpty(x) (x->pTag && *((x)->pTag->pTagEmptyInd)=='Y')
#define HIDIND(x) (*((x)->pTag->pTagAfterInd))
#define isHidden(x)    ((HIDIND(x)=='A') || (HIDIND(x)=='Y' &&  ISCOOKED))
#define isNotHidden(x) ((HIDIND(x)=='N') || (HIDIND(x)=='Y' && !ISCOOKED))

#define BNDSym(n,v)	RETeFalse2(AddSymbol(BND_SYMBOL,n,v,eTrue,iStrLen(v)+1)\
				  ,"AddSymbol(BND,%s) Failed." \
				  ,n)
#define SELSym(n,v)	RETeFalse2(AddSymbol(SEL_SYMBOL,n,v,eTrue,iStrLen(v)+1)\
				  ,"AddSymbol(SEL,%s) Failed." \
				  ,n)
#define INISym(n,v)	RETeFalse2(AddSymbol(INI_SYMBOL,n,v,eTrue,iStrLen(v)+1)\
				  ,"AddSymbol(INI,%s) Failed." \
				  ,n)
#define FRMSym(n,v)	RETeFalse2(AddSymbol(FRM_SYMBOL,n,v,eFalse,iStrLen(v)+1)\
				  ,"AddSymbol(FRM,%s) Failed." \
				  ,n)
#define ENVSym(n,v)	RETeFalse2(AddSymbol(ENV_SYMBOL,n,v,eTrue,iStrLen(v)+1)\
				  ,"AddSymbol(ENV,%s) Failed." \
				  ,n)
#define RAWSym(n,v,l)	RETeFalse2(AddSymbol(RAW_SYMBOL,n,v,eFalse,l) \
				  ,"AddSymbol(RAW,%s) Failed." \
				  ,n)

/*
/* Global Variables used by ALL modules.
 */
extern char
     *gpFileName
    ,*gpIniFile
    ,*gpProgram
    ;
extern int
     giParseLevel
    ;
extern eBoolean_t
     gbiFlag
    ,gbpFlag
    ,gbfFlag
    ,gbbFlag
    ,gbBufferOutput
    ;
extern PAGE *gpPage;	/* defined in swoutput.c */

extern char
     *gpIfExpr
    ,*gpNullSelect
    ;

extern SQLWEB_LDA gLDA;	/* defined/allocated in swdb.c */
extern LIST *glCookie;	/* Cookie List */

#define ISCOOKED		(giParseLevel==0||giParseLevel==3)

/*
/* External, Generally available functions
 */
extern eBoolean_t sqlweb();
extern eBoolean_t swinput();
extern eBoolean_t swlogic(LIST *lPageList);
extern eBoolean_t swgpage(char *pSQL,LIST *lPageGenNbrList);
extern eBoolean_t swoutput(PAGE *pPage);

extern void       MsgPush(const char *pFmt,...);
extern eBoolean_t MsgPop(char *pOutMsgBuf);

extern eBoolean_t OutputHTML(const char *pFmt,...);
extern eBoolean_t FlushHTMLBuf();
extern eBoolean_t PrintHTMLHeader(char *pContentType);
extern eBoolean_t FlushHTMLBuf();
extern eBoolean_t PrintHTMLErrStack();

/* extern eBoolean_t RemoveCharCodes(char *pIn, char *pOut);
 */
extern eBoolean_t RemoveCharCodes(char *pIn);
extern eBoolean_t GetTAGByName(char *pTagName,TAG **gpTAG);
extern eBoolean_t LoadHTML(char *pFilename
		   ,PAGE **pPage
		   );
extern eBoolean_t ShowSymbolTable();
extern eBoolean_t iPrintHTMLSymbol(SYMBOL *pSym);
extern int        iCmpSymbolpName(SYMBOL *pSym, char *pName);
extern int        iCmpSymbolNames(SYMBOL *pSym1,SYMBOL *pSym2);
#ifdef USEHASHTABLE
extern char *     GetSymbolName(SYMBOL *pSym);
#else
extern int	  GetSymbolName(SYMBOL *pSym,SYMBOL *p2);
#endif
extern eBoolean_t BuildSymbolTable(char *pBuf);
extern eBoolean_t GetSymbolValue(char *pName,char *pOutValue);
extern eBoolean_t GetSymbolValueREF(char *pName,char **pOutPtr);
extern eBoolean_t GetRawSymbolValueREF(char *pName,char **pOutPtr,long *lSize);
extern eBoolean_t GetARRSymbolValueREF(char *pName,int iIndex,char **pOutPtr);
extern eBoolean_t IsSymbolValue(char *pName,char *pValue);
extern eBoolean_t AddSymbol(int iType
			   ,char *pName
			   ,char *pValue
			   ,eBoolean_t bReplace
			   ,long lSize
			   );

extern eBoolean_t DebugHTMLSet(int iDebugLevel);
extern int        DebugHTMLGet();
extern eBoolean_t DebugHTML(char *pFileName
			   ,int iLine
			   ,int iDebugLevel
			   ,char *pFmt
			   ,...
			   );
extern eBoolean_t CookPI(PI *pPI);
extern eBoolean_t GetPIAValue(PI *pPI,char *pName, char *pOutValue);
extern eBoolean_t GetPIAValueREF(PI *pPI,char *pName, char **pOutValue);
extern eBoolean_t LoadPage(char *pPageGenNbr);
extern eBoolean_t ExpandString(char *pIn, char **pOut);
extern eBoolean_t ExpandContents(PI *pPI);
extern eBoolean_t LoadTag(char *pTagData);
extern void       LogMsg(const char *pFmt,...);
extern char *     DupBuf(const char *pBuf);
extern void *     DupMem(const char *pBuf, long lSize);
extern void       FreeBuf(char *pBuf);
extern eBoolean_t ParseIf(char *pExpr, eBoolean_t *pbResult);
extern PI *       NewPI();
extern eBoolean_t FreePI(PI *pPI);	/* Free PI (not children) */
extern eBoolean_t FreePIr(PI *pPI);	/* Free PI recursively */
extern eBoolean_t FreePIA(SYMBOL *pPIA);
extern SYMBOL *   NewPIA();
extern eBoolean_t LoadTEXT(char *pFileName,char *pFileType, PI *pPI);
extern eBoolean_t DumpPage(PAGE *pPage);
extern int	  iStrLen(char *pString);
extern int iStrCmp(char *s,char *t);
extern int iStrCaseCmp(char *s, char *t);
extern eBoolean_t bStrMatch(char *s,char *t);
extern eBoolean_t bStrNMatch(char *s,char *t,int n);
extern eBoolean_t bStrCaseMatch(char *s,char *t);
extern eBoolean_t bStrNCaseMatch(char *s,char *t,int n);
extern eBoolean_t GetCookieValueREF(char *pName, char **pBuf, long *lSize);

#endif /* _SQLWEB_H_ */
