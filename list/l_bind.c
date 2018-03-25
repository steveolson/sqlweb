/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */
/* L_BIND.C -- List Bind.  Binds insert, delete, and find functions to a LIST.
/*	
/*	The LFUNC (LIST FUNCTION) Table defines what insert, delete,
/*	and find functions make up a LIST class.  Available LIST classes are:
/*		QUEUE,		STACK,
/*		ASCEND,		DESCEND,
/*		UNSORTED,	INORDER.
/* */

#include "list.h"

/* Struct to hold insert, delete, find, and scan functions by NAME.
 */
typedef struct {
	char	*name;
	PFI	ins_f;
	PFV	del_f;
	PFN	find_f;
	PFI	scan_f;
} LFUNC;

/* MUST declare all functions
 */
extern void	*lnullpfv(),	*ldelq(),	*ldelsrch(),	*ldelbtree()
		,*ldelhash()
		;
extern LNODE	*lnullpfn(),	*lfindq(),	*lfindsrch(),	*lfbtree()
		,*lfdesc(),	*lfascend()
		,*lfindhash()
		;
		
extern int	 lnullpfi(),	 linsstack(),	 linsdesc(),	 linsbtree()
		,linsascend(),	 linsq(),	 liuascend()
		,lscanl(),   	 lscanbt(), 	 liudesc()
		,lscanhash(), linshash()
		;


/* Table to hold Insert, Delete, Find, and Scan Functions.
 */
static LFUNC lfunctab [] = {
/*	 NAME		INSERT FUNC	DELETE FUNC	FIND FUNC  WALK FUNC
/*	 ====		===========	===========	=========  =========
 */	{"HASH",	linshash,	ldelhash,	lfindhash, lscanhash},
	{"hash",	linshash,	ldelhash,	lfindhash, lscanhash},
	{"BTREE",	linsbtree,	ldelbtree,	lfbtree,   lscanbt },
	{"btree",	linsbtree,	ldelbtree,	lfbtree,   lscanbt },
	{"ASCEND",	linsascend,	ldelsrch,	lfascend,  lscanl },
	{"ascend",	linsascend,	ldelsrch,	lfascend,  lscanl },
	{"DESCEND",	linsdesc,	ldelsrch,	lfdesc,	   lscanl },
	{"descend",	linsdesc,	ldelsrch,	lfdesc,	   lscanl },
	{"UASCEND",	liuascend,	ldelsrch,	lfascend,  lscanl },
	{"uascend",	liuascend,	ldelsrch,	lfascend,  lscanl },
	{"UDESCEND",	liudesc,	ldelsrch,	lfdesc,	   lscanl },
	{"udescend",	liudesc,	ldelsrch,	lfdesc,	   lscanl },
	{"QUEUE",	linsq,		ldelq,		lfindq,	   lscanl },
	{"queue",	linsq,		ldelq,		lfindq,	   lscanl },
	{"STACK",	linsstack,	ldelq,		lfindq,	   lscanl },
	{"stack",	linsstack,	ldelq,		lfindq,	   lscanl },
	{"UNSORTED",	linsq,		ldelsrch,	lfindsrch, lscanl },
	{"unsorted",	linsq,		ldelsrch,	lfindsrch, lscanl },
	{"INORDER",	linsq,		ldelsrch,	lfindsrch, lscanl },
	{"inorder",	linsq,		ldelsrch,	lfindsrch, lscanl },

	/* Sentinal Do Not Delete */
	{ (char *)0,	NULL_PFI,	NULL_PFV,	NULL_PFN,  NULL_PFI }
};

/* Function to locate an LFUNC in the above table given a name, such as "QUEUE".
/*	called by lgetinsf, lgetdelf, lgetfindf.
 */
LFUNC *
getlfunc(char *name)
{
	register LFUNC	*lf;
	for( lf=lfunctab; lf->name != (char *)0 ; lf++)
		if( strcmp(lf->name,name)==0 )
			return( lf );
	return( (LFUNC *)0 );
}

/* Function to find the INSERT function given a name
 */
PFI
lgetinsf(char *name )
{
	register LFUNC	*lf;
	if( (lf=getlfunc(name)) )
		return( lf->ins_f );
	return( NULL_PFI );
}
/* Function to find the DELETE function given a name
 */
PFV
lgetdelf(char *name )
{
	register LFUNC	*lf;
	if( (lf=getlfunc(name)) )
		return( lf->del_f );
	return( NULL_PFV );
}
/* Function to find the FIND function given a name
 */
PFN
lgetfindf(char *name )
{
	register LFUNC	*lf;
	if( (lf=getlfunc(name)) )
		return( lf->find_f );
	return( NULL_PFN );
}
/* Function to find the SCAN function given a name
 */
PFI
lgetscanf( char *name )
{
	register LFUNC	*lf;
	if( (lf=getlfunc(name)) )
		return( lf->scan_f );
	return( NULL_PFI );
}
