/* ifwords.c - declares symbols for EXPR parsing
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

#include "ifwords.h"	/* Defines Reserved_t Type */
#include "scalar.h"	/* Defines TOKEN and FUNC,and pScalarFunc "handlers" */
#include "ifparse.h"	/* Defines the Tokens */

Reserved_t gaReservedWordTable[] = {
     { "IF",      IF,      TOKEN, 0}
    ,{ "BETWEEN", BETWEEN, TOKEN, 0}
    ,{ "LIKE",    LIKE,    TOKEN, 0}
    ,{ "IS",      IS,      TOKEN, 0}
    ,{ "IN",      IN,      TOKEN, 0}
    ,{ "NULL",    NULLX,   TOKEN, 0}
    ,{ "TRUE",    TRUEX,   TOKEN, 0}
    ,{ "FALSE",   FALSEX,  TOKEN, 0}
    ,{ "AND",     AND,     TOKEN, 0}
    ,{ "OR",      OR,      TOKEN, 0}
    ,{ "NOT",     NOT,     TOKEN, 0}
    ,{ "TO_SCALAR",TO_SCALAR,TOKEN,0}
    ,{ "SUBSTR",  FUNCNAME,FUNC,  ScalarSubstr}
    ,{ "LENGTH",  FUNCNAME,FUNC,  ScalarLength}
    ,{ "UPPER",   FUNCNAME,FUNC,  ScalarUpper}
    ,{ "LOWER",   FUNCNAME,FUNC,  ScalarLower}
    ,{ "TO_CHAR", FUNCNAME,FUNC,  ScalarToChar}
    ,{ "TO_NUMBER",FUNCNAME,FUNC, ScalarToNumber}
    ,{ "TO_NUM",  FUNCNAME,FUNC,  ScalarToNumber}
    ,{ "LPAD",    FUNCNAME,FUNC,  ScalarLPad}
    ,{ "RPAD",    FUNCNAME,FUNC,  ScalarRPad}
    ,{ "LTRIM",   FUNCNAME,FUNC,  ScalarLTrim}
    ,{ "RTRIM",   FUNCNAME,FUNC,  ScalarRTrim}
    ,{ "NVL",     FUNCNAME,FUNC,  ScalarNVL}
    ,{ "HOST",    FUNCNAME,FUNC,  ScalarHost}
    ,{ "SAVEFILE",FUNCNAME,FUNC,  ScalarSaveFile}
    ,{ "GETCOOKIE",FUNCNAME,FUNC, ScalarGetCookieValue}
    ,{ "REPLACE" ,FUNCNAME,FUNC,  ScalarReplace}
    ,{ "ASCII"   ,FUNCNAME,FUNC,  ScalarAscii}
    ,{ "CHR"     ,FUNCNAME,FUNC,  ScalarCHR}
    ,{ 0,         0,       0,     0}
};

