/* boolean.h - BOOLEAN definitions and MACROS
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
#ifndef _BOOLEAN_H_
#define _BOOLEAN_H_

/*
/* The "Enumerated" Boolean Type, "eBoolean_t"
 */
typedef enum {
	 eFalse	/* should map to Zero, but don't assume it!*/
	,eTrue	/* Should map to 1, but don't assume it! */
} eBoolean_t;

/*
/* stdarg (vararg) style Error Logging Routine..
 */
#define ERR(m)		  MsgPush(m)
#define ERR2(m,a)	  MsgPush(m,a)
#define ERR3(m,a,b)	  MsgPush(m,a,b)
#define ERR4(m,a,b,c)	  MsgPush(m,a,b,c)
#define ERR5(m,a,b,c,d)	  MsgPush(m,a,b,c,d)
#define ERR6(m,a,b,c,d,e) MsgPush(m,a,b,c,d,e)

#define ISeFalse(x)	((x)==eFalse)
#define ISeTrue(x)	(!ISeFalse(x))

#define RETeFalse(x,errm)	if(ISeFalse(x)){ERR(errm);return(eFalse);}
#define RETeFalse2(x,errm,a)	if(ISeFalse(x)){ERR2(errm,a);return(eFalse);}
#define RETeFalse3(x,errm,a,b)	if(ISeFalse(x)){ERR3(errm,a,b);return(eFalse);}
#define RETeFalse4(x,errm,a,b,c) if(ISeFalse(x)){\
				    ERR4(errm,a,b,c);return(eFalse);}
#define RETeFalse5(x,errm,a,b,c,d) if(ISeFalse(x)){\
				    ERR5(errm,a,b,c,d);return(eFalse);}
#define RETeFalse6(x,errm,a,b,c,d,e) if(ISeFalse(x)){\
				    ERR6(errm,a,b,c,d,e);return(eFalse);}


#endif /* _BOOLEAN_H_ */
