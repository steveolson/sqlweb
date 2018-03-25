/* scalar.h - header for scalar routines
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
#ifndef _SCALAR_H_
#define _SCALAR_H_

#include <stdio.h>
#include <ctype.h>
#include "boolean.h"
#include "list.h"

/*
/* The valid TYPES for the Scalar_t
 */
typedef enum {
	 eStringType	/* Character Strings */
	,eIntType	/* Integers */
	,eDoubleType	/* Real Numbers */
	,eListType	/* Linked-Lists */
	,eBoolType	/* Boolean Value (True or False) */
	,eRawType	/* Raw Type memory buffer with length */
} eScalar_t;

typedef struct {
    eScalar_t	esType;	/* a valid eStackType */
    union {
	char	*pString;/* character string */
	int	iInt;	/* int */
	double	dDouble;/* real number */
	LIST	*pList;	/* Linked-List of Scalar_t's */
	eBoolean_t bBool;/* Boolean */
	void	*pRaw; /* Raw Memory Buffer (not null terminated) */
    } uVal;		/* UNION */
    char *pName;
    long lSize;
} Scalar_t;

typedef Scalar_t *(*PFPS_t)(Scalar_t *); /* Pointer to function that accepts
					/* a pointer to a Scalar_t and
					/* returns a pointer to a Scalar_t
					 */

/*
/* Function Protytypes
 */
extern eBoolean_t BetweenCond(Scalar_t *p, Scalar_t *plo, Scalar_t *phi);
extern eBoolean_t LikeCond(Scalar_t *p1, Scalar_t *p2);
extern eBoolean_t IsNullCond(Scalar_t *p);
extern eBoolean_t InCond(Scalar_t *pVal, Scalar_t *pList);
extern eBoolean_t CompareCond(Scalar_t *pOp,Scalar_t *pLeft,Scalar_t *pRight);

extern Scalar_t *AddScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *CatScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *SubScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *MultScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *DivScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *ModScalar(Scalar_t *p1, Scalar_t *p2);
extern Scalar_t *USubScalar(Scalar_t *p1);
extern Scalar_t *CallFunction(Scalar_t *pName, Scalar_t *pList);
extern Scalar_t *AddScalarToList(Scalar_t *pList,Scalar_t *p1);

/*
/* Built-in Functions
 */
extern Scalar_t * ScalarUpper(Scalar_t *pList);
extern Scalar_t * ScalarLower(Scalar_t *pList);
extern Scalar_t * ScalarSubstr(Scalar_t *pList);
extern Scalar_t * ScalarLength(Scalar_t *pList);
extern Scalar_t * ScalarToChar(Scalar_t *pList);
extern Scalar_t * ScalarToNumber(Scalar_t *pList);
extern Scalar_t * ScalarLPad(Scalar_t *pList);
extern Scalar_t * ScalarRPad(Scalar_t *pList);
extern Scalar_t * ScalarLTrim(Scalar_t *pList);
extern Scalar_t * ScalarRTrim(Scalar_t *pList);
extern Scalar_t * ScalarNVL(Scalar_t *pList);
extern Scalar_t * ScalarHost(Scalar_t *pList);
extern Scalar_t * ScalarGetSym(Scalar_t *pScalar,char *pRule);
extern Scalar_t * ScalarGetSymARR(Scalar_t *pScalar,Scalar_t *pArrayOffset);
extern Scalar_t * ScalarSaveFile(Scalar_t *pList);
extern Scalar_t * ScalarGetCookieValue(Scalar_t *pList);
extern Scalar_t * ScalarReplace(Scalar_t *pList);
extern Scalar_t * ScalarAscii(Scalar_t *pList);
extern Scalar_t * ScalarCHR(Scalar_t *pList);

extern eBoolean_t AssignCond(Scalar_t *pScalarName, Scalar_t *pScalarValue);
/*
/* Called by yylex to create a scalar when found
 */
extern Scalar_t *MkScalar(eScalar_t esType, void *pValue, long lSize);
extern Scalar_t *MkScalarToken(int iToken);
extern Scalar_t *ToScalar(eBoolean_t bBool);
extern eBoolean_t IsScalar(Scalar_t *pScalar);
extern eBoolean_t OutputScalar(Scalar_t *pScalar);

#endif /* _SCALAR_H_ */
