%{
/* ifparse.y - EXPR Parser
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
#include "scalar.h"
#include "boolean.h"
#include "ifwords.h"

/* 
/* The Global Variables for the IfParser
 */
static eBoolean_t gbIfResult;		/* The Answer */
static char       *gpIfExpr;		/* The Expression to Parse */
static int        giRestart = 0;	/* Start a new expression */

%}
%union {
    eBoolean_t bVal;	/* Boolean Value eTrue or eFalse */
    Scalar_t *pScalar;	/* Scalar Variable */
}

%token	IF AND OR NOT BETWEEN LIKE IS NULLX TRUEX FALSEX IN ASSIGN CAT
%token	COMPARISON LT GT LE GE EQ NEQ
%token	STRING INTNUM APPROXNUM FUNCNAME LITERAL TO_SCALAR
%type	<bVal>    IfExp BoolExp BoolCond /* Expr */
%type	<bVal>    BetweenCond LikeCond IsNullCond InCond CompareCond AssignCond
%type	<bVal>    IsTrueCond IsFalseCond
%type	<pScalar> ScalarExp ScalarCommaList SymbolRef SimpleExp
%type	<pScalar> COMPARISON LT GT LE GE EQ NEQ
%type	<pScalar> STRING INTNUM APPROXNUM FUNCNAME LITERAL
%left	AND OR
%nonassoc NOT
%left	BETWEEN LIKE IS IN COMPARISON /* LT GT LE GE EQ NEQ */
%left	'+' '-' CAT
%left	'*' '/' '%'
%nonassoc UMINUS ':'
%%
/*
/* STARTING point an IF-Expression
 */
IfExp:	  BoolExp ';' 		{gbIfResult=$1;}
	| IF BoolExp ';' 	{gbIfResult=$2;}
	;
/*
/* Boolean Expression-- True or False
 */
BoolExp:  BoolExp AND BoolExp	{$$= (ISeTrue($1) && ISeTrue($3));}
	| BoolExp  OR BoolExp	{$$= (ISeTrue($1) || ISeTrue($3));}
	| NOT BoolExp		{$$= !ISeTrue($2);}
	| '(' BoolExp ')'	{$$= ISeTrue($2);}
	| BoolCond		{$$= ISeTrue($1);}
	;
/* 
/* Predicates that return Boolean Values
 */
BoolCond:	CompareCond	{$$=$1;}
	|	BetweenCond	{$$=$1;}
	|	LikeCond	{$$=$1;}
	|	IsNullCond	{$$=$1;}
	|	IsTrueCond	{$$=$1;}
	|	IsFalseCond	{$$=$1;}
	|	InCond		{$$=$1;}
	|	AssignCond	{$$=$1;}
	|	ScalarExp	{$$=OutputScalar($1);}
	;
/*
/* The Specific Boolean Conditions
 */
BetweenCond:	ScalarExp NOT BETWEEN ScalarExp AND ScalarExp
				{$$=!BetweenCond($1,$4,$6);}
	|	ScalarExp     BETWEEN ScalarExp AND ScalarExp
				{$$= BetweenCond($1,$3,$5);}
	;
LikeCond:	ScalarExp NOT LIKE ScalarExp	{$$=!LikeCond($1,$4);}
	|	ScalarExp LIKE ScalarExp	{$$= LikeCond($1,$3);}
	;
IsNullCond:	ScalarExp IS NOT NULLX		{$$=!IsNullCond($1);}
	|	ScalarExp IS NULLX		{$$= IsNullCond($1);}
	;
IsTrueCond:	ScalarExp IS NOT TRUEX		{$$=!IsScalar($1);}
	|	ScalarExp IS TRUEX		{$$= IsScalar($1);}
	;
IsFalseCond:	ScalarExp IS NOT FALSEX		{$$= IsScalar($1);}
	|	ScalarExp IS FALSEX		{$$=!IsScalar($1);}
	;
InCond:		ScalarExp NOT IN '(' ScalarCommaList ')'
						{$$=!InCond($1,$5);}
	|	ScalarExp IN '(' ScalarCommaList ')'
						{$$= InCond($1,$4);}
	;
CompareCond:	ScalarExp COMPARISON ScalarExp	{$$=CompareCond($2,$1,$3);}
	;
AssignCond:	SymbolRef ASSIGN ScalarExp	{$$=AssignCond($1,$3);}
	;
/*
/* scalar expressions
 */
ScalarExp:	ScalarExp '+' ScalarExp		{$$=AddScalar($1,$3);}
	|	ScalarExp CAT ScalarExp		{$$=CatScalar($1,$3);}
	|	ScalarExp '-' ScalarExp		{$$=SubScalar($1,$3);}
	|	ScalarExp '*' ScalarExp		{$$=MultScalar($1,$3);}
	|	ScalarExp '/' ScalarExp		{$$=DivScalar($1,$3);}
	|	ScalarExp '%' ScalarExp		{$$=ModScalar($1,$3);}
	|	'+' ScalarExp %prec UMINUS	{$$=$2;}
	|	'-' ScalarExp %prec UMINUS	{$$=USubScalar($2);}
	|	FUNCNAME '(' ScalarCommaList ')'{$$=CallFunction($1,$3);}
						/* All function get called
						/* with a LIST* of Scalar_t
						 */
	|	'(' ScalarExp ')'		{$$=$2;}
	|	SimpleExp			{$$=$1;}
	|	SymbolRef			{$$=$1;}
	;
SimpleExp:	STRING				{$$=$1;} /* Quoted str 'xtr' */
	|	INTNUM				{$$=$1;} /* 242 */
	|	APPROXNUM			{$$=$1;} /* 234.223 */
	|	TO_SCALAR '(' BoolExp ')'	{$$=ToScalar($3);}
	;
SymbolRef:	':' ScalarExp 			{$$=ScalarGetSym($2,"cE");}
	|	':' LITERAL			{$$=ScalarGetSym($2,"cL");}
	|	':' LITERAL '[' ScalarExp ']'	{$$=ScalarGetSymARR($2,$4);}
/*	|	LITERAL 			{$$=ScalarGetSym($1,"xL");}
 */
	;

ScalarCommaList:
		/* nothing */	{$$=(Scalar_t*)0;}
	|	ScalarExp	{$$=MkScalar(eListType,$1,$1->lSize);}
	|	ScalarCommaList ',' ScalarExp
				{$$=AddScalarToList($1,$3);}
	;
%%

int
yylex()
{
    static char *fp;
    static char ssBuf[ BUFSIZ ];	/* Needs to stay around */
					/* so I can return pointer to... */
    int    i;

    if(giRestart == 0) {
	fp = gpIfExpr;
	giRestart = 1;
    }

    /* Skip White Space
     */
    while( isspace( *fp ) )
	fp++;

    /*
    /* Deal with the End-Of-String
     */
    if( *fp == 0) {
	if( giRestart == -1 ) {
	    giRestart=0;/* Second time... */
	    return(0);	/* That's all */
	}
	giRestart= -1;	/* Sets for End above */
	return(';');	/* Returns Sentinal Token */
    }

    /* fprintf(stderr,"yylex:%s\n",fp);
    /* fflush(stderr);
     */

    /*
    /* Process known single and double character character tokens
     */
    switch(*fp) {
	/*
	/* COMPARISON's
	 */
	case '<':
	    if(*(fp+1) == '='){
		fp+=2;
		yylval.pScalar = MkScalarToken(LE);
		return COMPARISON;
	    }
	    if(*(fp+1) == '>'){
		fp+=2;
		yylval.pScalar = MkScalarToken(NEQ);
		return COMPARISON;
	    } else {
		fp++;
		yylval.pScalar = MkScalarToken(LT);
		return COMPARISON;
	    }
	case '>':
	    if(*(fp+1) == '='){
		fp+=2;
		yylval.pScalar = MkScalarToken(GE);
		return COMPARISON;
	    } else{
		fp++;
		yylval.pScalar = MkScalarToken(GT);
		return COMPARISON;
	    }
	case '!':		
	    if(*(fp+1) == '=')	{
		fp+=2;
		yylval.pScalar = MkScalarToken(NEQ);
		return COMPARISON;
	    } else {
		fp++;
		yylval.pScalar = MkScalarToken('!');
		return COMPARISON;
	    }
	case '=':
	    fp++;
	    yylval.pScalar = MkScalarToken(EQ);
	    return COMPARISON;
	case ':':
	    if(*(fp+1) == '=') {
		yylval.pScalar = MkScalarToken(ASSIGN);
		fp+=2;
		return(ASSIGN);
	    } else {
		yylval.pScalar = MkScalarToken(':');
		fp++;
		return(':');
	    }
	case '|':
	    if(*(fp+1) == '|') {
		yylval.pScalar = MkScalarToken(CAT);
		fp+=2;
		return(CAT);
	    } else {
		yylval.pScalar = MkScalarToken('|');
		fp++;
		return('|');
	    }
    }

    /*
    /* Literal strings are Single Quoted
     */
    if( *fp == '\'' ) {
	for(i=0, ++fp; *fp; fp++,i++) {
	    if(*fp=='\'' && *(fp+1) != '\'')
		break;
	    ssBuf[i] = *fp;
	}
	ssBuf[i]='\0';
	if(*fp) ++fp;
	yylval.pScalar = MkScalar(eStringType,ssBuf,1+iStrLen(ssBuf));
	return(STRING);
    }

    /*
    /* Numbers start with a digit or '.'
     */
    if(*fp =='.' || isdigit(*fp)) {
	for(i=0; *fp =='.' || isdigit(*fp); fp++,i++) {
	    ssBuf[i] = *fp;
	}
	ssBuf[i]='\0';
	/*
	/* Here's where we can differentiate between INTS and
	/* REALS, but don't need to...
	 */
	yylval.pScalar = MkScalar(eDoubleType,ssBuf,0);
	return(APPROXNUM);
    }

    /*
    /* Variables/Builtins/Reserved words start with alpha
     */
    if( isalpha(*fp) || *fp == '_' ) {
	char *pBuf;
	Reserved_t *pResWord;

	for(i=0; *fp == '_' || isalnum(*fp) ; fp++,i++)
	    ssBuf[i]= *fp;
	ssBuf[i]='\0';

	for(pResWord=gaReservedWordTable; pResWord->pName; pResWord++){
	    if(bStrCaseMatch(pResWord->pName,ssBuf)) {
		/* Got one!
		 */
		if(pResWord->iType == TOKEN)
		    return(pResWord->iToken);
		yylval.pScalar = MkScalar(eStringType,ssBuf,1+iStrLen(ssBuf));
		return(pResWord->iToken);
	    }
	}
	/*
	/* Look up variable name
	 */
	yylval.pScalar = MkScalar(eStringType,ssBuf,1+iStrLen(ssBuf));
	return(LITERAL);
    }

    return(*fp++);
}

void
yyerror(char *s)
{
    giRestart=0;
    /*
    /* fprintf(stderr,"yyerror:%s\n",s);
     */
    MsgPush("ifparse error:%s\n",s);
    return;
}

eBoolean_t 
ParseIf(char *pExpr, eBoolean_t *pbResult)
{
    giRestart=0;	/* Restart at beginning of string */
    gbIfResult=eFalse;	/* Init Result to FALSE, feel lucky punk? */
    gpIfExpr=pExpr;	/* Set the Global Expression for yylex(); */
    if(yyparse()) {		/* Parse it! */
	MsgPush("Parser:Failed on \"%s\"",pExpr);
	*pbResult = gbIfResult;
	return(eFalse);
    }
    *pbResult = gbIfResult;
    return(eTrue);
}

#ifdef MAIN_TEST
int
main(int argc, char **argv)
{
    if( ISeTrue(ParseIf(argv[1]) ) ) {
	fprintf(stderr,"Its TRUE!\n");
    } else {
	fprintf(stderr,"FALSE\n");
    }
    exit(0);
}
int strcasecmp(char *s, char *t){ return iStrCaseCmp(s,t); }
#endif
