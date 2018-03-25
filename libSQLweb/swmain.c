/* swmain.c - shell level interface for sqlweb contains main function.
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

#define GETOPTEOF	(-1)
#define GTERR(S,C)	MsgPush("%s%c\n",(S),(C))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlweb.h"

/*
/* Global Variables
 */

/*
/* Internal functions
 */
static int sw_getopt(int argc, char **argv, char *opts);

static int	optind=1;
static char    *optarg;
static int	sp=1;

static char *USAGE = "USAGE:\n\
CGI Call:\n\
    sqlweb\n\
	where PATH_TRANSLATED is used to determine SQLweb Page\n\
	according to HTTP\n\
SQLweb Page Utility:\n\
    sqlweb [-b] [-iIniFile] [-p ParseLevel] html_file\n\
	where,\n\
	    -b	Buffer output \n\
	    -p	set parse level (0=expand page;1=show source;2=dump tags)\n\
	    -i  Sets IniFile to the desired initialization file\n\
\n";

int
main(int    argc
    ,char **argv
    ,char **arge
    )
{
    int  iErrFlag=0
	,c;
/*
/*    eBoolean_t
/*	 bCFlag=eTrue	/* Cook Page */
/*	;
 */
    /*ctSetup();
     */

    gpProgram = strrchr(argv[0],'/');
    if(gpProgram) gpProgram++;
    else	  gpProgram=argv[0];

    while((c=sw_getopt(argc, argv, "bei:f:p:")) != -1) {
	switch (c) {
	    /* FLAGS
	     */
	    case 'b': gbbFlag=eTrue;				break;
	    case 'e': gbpFlag=eTrue;giParseLevel=1;		break;
	    /* Option Args
	     */
	    case 'i': gbiFlag=eTrue;gpIniFile=optarg;		break;
	    case 'p': gbpFlag=eTrue;giParseLevel=atoi(optarg);	break;
	    case 'f': gbfFlag=eTrue;gpFileName=optarg;		break;
			/* For Backward compatability
			/* Not listed in usage...
			 */
	    /* Syntax Errors */
	    case '?':
		iErrFlag++;
		break;
	}
    }
    if(iErrFlag){
	fprintf(stderr,USAGE);
	exit(2);
    }

    /*
    /* Report Args
     */
    if(optind<argc) {
	gpFileName = argv[optind];
    }

    return(ISeTrue(sqlweb()) ? 0: 1);
}

static int
sw_getopt(int argc, char **argv, char *opts)
{
	register int c;
	register char *cp;

	if (sp == 1)
		if (optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(GETOPTEOF);
		else if (bStrMatch(argv[optind], "--")) {
			optind++;
			return(GETOPTEOF);
		}
	c = argv[optind][sp];
	if (c == ':' || (cp=strchr(opts, c)) == 0) {
		GTERR("illegal option -- ", c);
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		optarg = 0;
		return('?');
	}
	if (*++cp == ':') {
#ifdef FASCIST
		if (sp != 1) {
			GTERR("option must not be grouped -- ", c );
			optind++;
			sp = 1;
			optarg = 0;
			return('?');
		} else
#endif
		if (argv[optind][sp+1] != '\0') {
#ifdef FASCIST
			GTERR("option must be followed by whitespace -- ", c );
			optind++;
			sp = 1;
			optarg = 0;
			return('?');
#else
			optarg = &argv[optind++][sp+1];
#endif
		} else if (++optind >= argc) {
			GTERR("option requires an argument -- ", c);
			sp = 1;
			optarg = 0;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if (argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = 0;
	}
	return(c);
}
