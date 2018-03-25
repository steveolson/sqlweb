/*  ifwords.h - defines types for EXPR Parser
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

#ifndef _IFWORDS_H_
#define _IFWORDS_H_
#include "scalar.h"

/*
/* Reserved words
 */
#define FUNC  0
#define TOKEN 1

typedef struct {
    char *pName;
    int   iToken
	 ,iType;
    PFPS_t pScalarFunc;
} Reserved_t;


extern Reserved_t gaReservedWordTable [ ] ;

#endif /* _IFWORDS_H_ */
