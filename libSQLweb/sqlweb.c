/* sqlweb.c - shell level interface contains the main() function
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

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sqlweb.h"

/*
/* Internal Functions
 */
static eBoolean_t SQLwebProc();

/*
/* Global Variables...
/*	These are initialized here, but are overridden in swmain.pc
/*	as part of command-line processing..... these are the defaults.
 */
SQLWEB_LDA gLDA;

char *gpFileName=0
    ,*gpProgram=0
    ,*gpIniFile=0
    ;
eBoolean_t
     gbiFlag=eFalse	/* Ini File */
    ,gbpFlag=eFalse	/* Parse level (0=cook;1=source dump;2=tag dump)*/
    ,gbfFlag=eFalse	/* File-to-process flag */
    ,gbbFlag=eFalse	/* Buffer output flag */
    ;
int 
     giParseLevel=0	/* Parse Level */
    ;

/*
/* SQLweb-- transaction control layer, based on return code
/* of SQLwebProc() commit or rollback.
 */
eBoolean_t
sqlweb()
{
    eBoolean_t bSQLwebRet;

    /* 
    /* Run SQLweb!
     */
    bSQLwebRet = SQLwebProc();

    if(ISeFalse(bSQLwebRet)) {
	PrintHTMLErrStack();
	(void) DbDisconnect(&gLDA,eFalse);
		/* Don't care about discon status
		/*  we're already exiting with err 
		 */
	return(eFalse);
    } else {
	RETeFalse(DbDisconnect(&gLDA,eTrue)
		 ,"Disconnect Failed"
		 );
    }
    return(bSQLwebRet);
}

/*
/* SQLwebProc -- most of the high-level processing in
/* SQLweb, multiple failure points. 
 */
static eBoolean_t
SQLwebProc()
{
    char *pBuf, *pBuf2
	,sBuf[BUFSIZ];

    /*
    /* Load the SYMBOL Table
     */
    RETeFalse(swinput(),"Internal error: swinput failed");

    /*
    /* Implement LISCENSE KEY (coming soon)
     */

    /* Set buffering */
    if(ISeTrue(gbbFlag)
	|| (ISeTrue(GetSymbolValueREF("SQLWEB_BUFFER_OUTPUT",&pBuf))
	    && is_casematch(pBuf,"TRUE")))
    {
	gbBufferOutput = eTrue;
    }

    /*
    /* Now that input is loaded... 
    /* set the DEBUG LEVEL
     */
    if(ISeTrue(GetSymbolValueREF("SQLWEB_DEBUG_LEVEL",&pBuf))
	&& ISCOOKED)
    {
	DebugHTMLSet(atoi(pBuf));
    }

    if(ISeTrue(GetSymbolValueREF("SQLWEB_SHOW_SYMBOLS",&pBuf))
	&& ISCOOKED
	&& is_casematch(pBuf,"TRUE"))
    {
	ShowSymbolTable();
    }

    /*
    /* Connect to Database, perhaps...
     */
    if((ISeFalse(GetSymbolValueREF("SQLWEB_DELAY_CONNECT",&pBuf)) && ISCOOKED)
	|| (is_casematch(pBuf,"FALSE")))
    {
	RETeFalse(DbConnect(0,&gLDA),"Failed to CONNECT to Database");
    }

    /*
    /* Check for PATH_TRANSLATED
     */
    if(    ISeTrue(GetSymbolValueREF("PATH_TRANSLATED",&pBuf))
	&& gpFileName==0
	/* && access(pBuf,R_OK)==0 */ )
	    /* File is readable
	     */
    {
	gpFileName=strdup(pBuf);
    }
    /*
    /* Check for PATH_INFO, if no filename....
     */
    if( !gpFileName){
	if(ISeTrue(GetSymbolValueREF("PATH_INFO",&pBuf))
	    && ISeTrue(GetSymbolValueREF("SQLWEB_DOC_ROOT",&pBuf2)))
	{
	    sprintf(sBuf,"%s%s",pBuf2,pBuf);
	    gpFileName=strdup(sBuf);
	}
    }

    if(gpFileName) {
	/* A FILE
	/* Build a PAGE from an HTML file 
	 */
	PAGE *pPage;
	struct stat stBuf;
	char *pBasename;
	int iLen = iStrLen(gpFileName);

	/* Step on trailing '/' **chop**
	 */
	if( *(gpFileName+iLen-1)=='/'){
	    *(gpFileName+iLen-1)=0;
	}

	/*
	/* Verify file type...
	 */
	stat(gpFileName,&stBuf);
	if(S_ISDIR(stBuf.st_mode)){
	    if(ISeTrue(GetSymbolValueREF("SQLWEB_DEFAULT_PAGE",&pBuf2))) {
		sprintf(sBuf,"%s/%s",gpFileName,pBuf2);
		gpFileName = strdup(sBuf);
	    } else {
		MsgPush("Can't open directory: Missing SQLWEB_DEFAUL_PAGE");
		return(eFalse);
	    }
	}
	pBasename = strrchr(gpFileName,'/');
	RETeFalse2(LoadHTML(gpFileName,&pPage)
		  ,"Failed to load: %s"
		  ,pBasename? ++pBasename :gpFileName
		  );
	if(l_size(pPage->lPI)==0) {
	    MsgPush("File Contains no TAGS");
	    return(eFalse);
	}
	if(ISCOOKED) {
	    RETeFalse2(swoutput(pPage)
		      ,"Output module Failed on: %s"
		      ,pBasename? ++pBasename :gpFileName
		      );
	} else {
	    (void)DumpPage(pPage);
	}
    } else {
	MsgPush("Can't find SQLweb file to process.");
	return(eFalse);
    }
    return(eTrue);
}
