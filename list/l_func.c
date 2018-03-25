/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */
/*
/* L_FUNC.C -- LIST Functions.
/*	The Class specific INSERT, DELETE, and FIND functions.
/*
/*	Functions within this module are:
/*
/*	void	*lnullpfv(),	*ldelq(),	*ldelsrch(),
/*		*ldelbtree();
/*	LNODE	*lnullpfn(),	*lfindq(),	*lfindsrch();
/*	LNODE	*lfascend(),	*lfdesc(),	*lfbtree();
/*	LNODE	*liuascend(),	*liudesc();
/*	int	linsstack(),	linsdesc(),	linsascend(),
/*		linsbtree(),	linsq(),	lnullpfi();
/*
/* */ 


#include <stdio.h>
#include "list.h"

/* LDELQ -- Delete an Element from a QUEUE
/*	All DELETE functions receive the following paramaters:
/*		**first,
/*		**last,
/*		**current
 */
void *
ldelq(LNODE **first, LNODE **last,LNODE ** current)
{
	register LNODE	*tmp;
	register void	*info;
	if( (*first) ) {
		tmp = (*first)->next;
		info= (*first)->info;
		free( (*first) );
		(*first) = tmp;
		if( tmp )	tmp->prev = NULL_LNODE;
		else		(*last)   = NULL_LNODE;
		return( info );
	}
	return( (void *)0 );
}
/* LDELSRCH -- Delete an Element from INORDER, UNSORTED, ASCENDing or
/*	DESCENDing LIST.
/*		
/*	All DELETE functions receive the following paramaters:
/*		**first,
/*		**last,
/*		**current
 */
void *
ldelsrch(LNODE **first, LNODE **last, LNODE **current)
{
	register void	*info;

	if( (*current) == NULL_LNODE )
		return( (void *)0 );
	info= (*current)->info;

/* update last and first pointers
 */
	if( (*current) == (*first) )	(*first) = (*current)->next;
	if( (*current) == (*last) )	(*last)  = (*current)->prev;

/* Unlink element from list
 */
	if( (*current)->prev != NULL_LNODE )
		(*current)->prev->next = (*current)->next;
	if( (*current)->next != NULL_LNODE )
		(*current)->next->prev = (*current)->prev;
	(*current)->next = NULL_LNODE;
	(*current)->prev = NULL_LNODE;

/* Free NODE
 */
	free( (*current) );
	return (info);
}
/* LDELBTREE -- Delete an Element from a BTREE LIST
/*		
/*	All DELETE functions receive the following paramaters:
/*		**first,
/*		**last,
/*		**current
 */
void *
ldelbtree(LNODE **first, LNODE **last, LNODE **current)
{
	register void	*info, *tmp;
	register LNODE	*new;

	if( (*current) == NULL_LNODE )
		return( (void *)0 );

	info=(*current)->info;

	/* Delete if first node
	 */
	if( (*current) == (*first) ) {
		if( (*first)->prev ) {
			/* UPDATE first
			 */
			(*first)=(*first)->prev;
			for(new=(*first); new->next!=NULL_LNODE; new=new->next)
				;
			new->next = (*current)->next;
		}
		else	(*first) = (*first)->next;

		free( (*current) );
		return(info);
	}

	if( (*last) == NULL_LNODE ) {
		return( (void *)0 );
	}
	/* Not the First NODE
	 */
	if( (*last)->prev == (*current) ) {
		if( (*current)->prev ) {
			(*last)->prev = (*current)->prev;
			for(new=(*current)->prev;
				new && new->next!=NULL_LNODE;
				new=new->next)
			    ;
			if(new)
				new->next = (*current)->next;
		} else	(*last)->prev = (*current)->next;
		free( (*current) );
		return( info );
	} else if( (*last)->next == (*current) ) {
		if( (*current)->prev ) {
			(*last)->next = (*current)->prev;
			for(new=(*current)->prev;
				new && new->next!=NULL_LNODE;
				new=new->next)
			    ;
			if(new)
				new->next = (*current)->next;
		} else	(*last)->next = (*current)->next;
		free( (*current) );
		return( info );
	}
/*	else {
/*		printf("Bad Last not parent of current\n");
/*		return( (void *)0 );
/*	}
 */
}
/* LFINDQ -- Find an Element on a QUEUE.
/*
/*	A QUEUE --ALWAYS-- Finds FIRST Element.  Thats what a QUEUE is.
/*
/*	All FIND functions receive the following paramaters:
/*		**first, 
/*		**last,
/*		size,
/*		cmp_f,
/*		cmp_info;
 */
LNODE *
lfindq(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
	return( (*first) );
}
/* LFASCEND -- Find an Element on a Searchable ASCENDING LIST.
/*
/*	The compare function, 'cmp_f', MUST return 0 on a MATCH.
/*	The compare function is called like so for each Element in the LIST
/*	until a MATCH is found or the cmp_f returns >0
/*		cmp_f(info, cmp_i)
/*
/*	All Find functions receive the following paramaters:
/*		**first, 
/*		**last,
/*		size,
/*		cmp_f,
/*		cmp_info;
 */
LNODE *
lfascend(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
	register LNODE	*tmp;
	register short	cmpval;
	for( tmp=(*first); tmp!=NULL_LNODE; tmp=tmp->next)
		if( (cmpval=((*cmp_f)(tmp->info, cmp_i))) == 0)
			return( tmp );
		else if (cmpval>0)	/* No need to search rest */
			break;
	return( NULL_LNODE );
}
/* LFDESC -- Find an Element on a Searchable DESC LIST.
/*
/*	The compare function, 'cmp_f', MUST return 0 on a MATCH.
/*	The compare function is called like so for each Element in the LIST
/*	until a MATCH is found or the cmp_f returns <0
/*		cmp_f(info, cmp_i)
/*
/*	All Find functions receive the following paramaters:
/*		**first, 
/*		**last,
/*		size,
/*		cmp_f,
/*		cmp_info;
 */
LNODE *
lfdesc(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
	register LNODE	*tmp;
	register short	cmpval;
	for( tmp=(*first); tmp!=NULL_LNODE; tmp=tmp->next)
		if( (cmpval=((*cmp_f)(tmp->info, cmp_i))) == 0)
			return( tmp );
		else if (cmpval<0)	/* No need to search rest */
			break;
	return( NULL_LNODE );
}
/* LFINDSRCH -- Find an Element on a Searchable LIST: INORDER, UNSORTED,
/*	ASCEND, DESCEND.
/*
/*	The compare function, 'cmp_f', MUST return 0 on a MATCH.
/*	The compare function is called like so for each Element in the LIST
/*	until a MATCH is found or it reaches the END of the LIST.
/*		cmp_f(info, cmp_i)
/*
/*	All Find functions receive the following paramaters:
/*		**first, 
/*		**last,
/*		size,
/*		cmp_f,
/*		cmp_info;
 */
LNODE *
lfindsrch(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
	register LNODE	*tmp;
	for( tmp=(*first); tmp!=NULL_LNODE; tmp=tmp->next)
		if( ((*cmp_f)(tmp->info, cmp_i)) == 0)
			return( tmp );
	return( NULL_LNODE );
}
/* LFINDBTREE -- Find an Element on a BTREE LIST
/*
/*	The compare function, 'cmp_f', MUST return 0 on a MATCH.
/*	The compare function is called like so for each Element in the LIST
/*	until a MATCH is found or it reaches the END of the LIST.
/*		cmp_f(info, cmp_i)
/*
/*	All Find functions receive the following paramaters:
/*		**first, 
/*		**last,
/*		size,
/*		cmp_f,
/*		cmp_info;
 */
LNODE *
lfbtree(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
	register int	ret;

	if( (*first) == NULL_LNODE)
		return( NULL_LNODE );

	if( (ret=(*cmp_f)((*first)->info, cmp_i)) == 0)
		return( (*first) );
	(*last)=(*first);
	if(ret<0) {
		return(lfbtree(&(*first)->next,last, size, cmp_f, cmp_i));
	} else /* if(ret>0) */ {
		return(lfbtree(&(*first)->prev,last, size, cmp_f, cmp_i));
	}
}
/* LINSSTACK -- Insert an Element onto a STACK.
/*
/*	A STACK --ALWAYS-- Inserts at FIRST Element.  Thats what a STACK is.
/*
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
linsstack( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new;
	if( (new=(LNODE *)malloc( sizeof(LNODE))) == (NULL_LNODE) )
		return( 0 );
	new->weight	= 0;
	new->next	= (*first);
	new->prev	= NULL_LNODE;
	new->info	= info;
	if((*first)==NULL_LNODE)
		(*last)=new;
	else	(*first)->prev = new;	/* link new to front of list */
	(*first) = new;
	return(1);
}
/* LIUDESC -- Insert an Element NO DUPS into a DESCENDING List.
/*
/*	The compare function, 'cmp_f', must return <0 when the INFO
/*	Inserted is 'Less Than' another INFO structure and = when Equal.
/*	The 'cmp_f' must know what a user-malloc'd INFO structure is and
/*	what makes one 'Less Than', 'Equal to', and 'Greater Than' another. 
/*
/*	'cmp_f' gets the new INFO struct as its second parameter.
/*	
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
liudesc( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new, *n;
	register short	val;

	for( n= (*first); n!=NULL_LNODE; n=n->next) {
		if( (val=(*cmp_f)(n->info, info)) < 0 )
			break;
		else if( val==0 )
			return(2);	/* SUCCESS its already there */
	}
	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight = 0;
	new->info = info;
	new->next = n;
	if(n) {
		if(n->prev)	n->prev->next = new;
		new->prev = n->prev;
		n->prev = new;
	} else {
		new->prev = (*last);
		if( (*last) )	(*last)->next = new;
		(*last) = new;
	}
	if( n == (*first))
		(*first) = new;
	return (1);
}
/* LINSDESC -- Insert an Element into a DESCENDING List.
/*
/*	The compare function, 'cmp_f', must return <0 when the INFO
/*	Inserted is 'Less Than' another INFO structure.  The 'cmp_f' must
/*	know what a user-malloc'd INFO structure is and what makes one
/*	'Less Than', 'Equal to', and 'Greater Than' another. 
/*
/*	'cmp_f' gets the new INFO struct as its second parameter.
/*	
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
linsdesc( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new, *n;

	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight = 0;
	new->info = info;
	for( n= (*first); n!=NULL_LNODE; n=n->next)
		if( (*cmp_f)(n->info, info) < 0 )
			break;
	new->next = n;
	if(n) {
		if(n->prev)	n->prev->next = new;
		new->prev = n->prev;
		n->prev = new;
	} else {
		new->prev = (*last);
		if( (*last) )	(*last)->next = new;
		(*last) = new;
	}
	if( n == (*first))
		(*first) = new;
	return (1);
}
/* LIUASCEND -- Insert an Element NO DUPS into an ASCENDING List.
/*
/*	The compare function, 'cmp_f', must return >0 when the INFO
/*	Inserted is 'Greater Than' another INFO structure and = when equal.
/*	The 'cmp_f' must know what a user-malloc'd INFO structure is and
/*	what makes one 'Less Than', 'Equal to', and 'Greater Than' another. 
/*
/*	'cmp_f' gets the new INFO struct as its second parameter.
/*	
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
liuascend( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new, *n;
	register short	val;
	for( n= (*first); n!=NULL_LNODE; n=n->next) {
		if( (val=(*cmp_f)(n->info, info)) > 0 )
			break;
		else if( val==0 )
			return(2);
	}
	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight = 0;
	new->info = info;
	new->next = n;
	if(n) {
		if(n->prev)	n->prev->next = new;
		new->prev = n->prev;
		n->prev = new;
	} else {
		new->prev = (*last);
		if( (*last) )	(*last)->next = new;
		(*last) = new;
	}
	if( n == (*first))
		(*first) = new;
	return (1);
}
/* LINSASCEND -- Insert an Element into an ASCENDING List.
/*
/*	The compare function, 'cmp_f', must return >0 when the INFO
/*	Inserted is 'Greater Than' another INFO structure.  The 'cmp_f' must
/*	know what a user-malloc'd INFO structure is and what makes one
/*	'Less Than', 'Equal to', and 'Greater Than' another. 
/*
/*	'cmp_f' gets the new INFO struct as its second parameter.
/*	
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
linsascend( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new, *n;
	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight = 0;
	new->info = info;
	for( n= (*first); n!=NULL_LNODE; n=n->next) {
		if( (*cmp_f)(n->info, info) > 0 )
			break;
	}
	new->next = n;
	if(n) {
		if(n->prev)	n->prev->next = new;
		new->prev = n->prev;
		n->prev = new;
	} else {
		new->prev = (*last);
		if( (*last) )	(*last)->next = new;
		(*last) = new;
	}
	if( n == (*first))
		(*first) = new;
	return (1);
}
/* LINSQUEUE -- Insert an Element into a QUEUE.
/*
/*	A QUEUE --ALWAYS-- Inserts at LAST Element.  Thats what a QUEUE is.
/*
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
linsq( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register LNODE	*new;
	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight	= 0;
	new->prev 	= (*last);
	if( new->prev )
		new->prev->next = new;
	new->next	= NULL_LNODE;
	new->info	= info;
	(*last)		= new;
	if( (*first) == NULL_LNODE )
		(*first)	= (*last);
	return(1);
}
/* LINSBTREE -- Insert an Element into a BTREE List.
/*
/*	The compare function, 'cmp_f', must return <0 when the INFO
/*	Inserted is 'Less Than' another INFO structure.  The 'cmp_f' must
/*	know what a user-malloc'd INFO structure is and what makes one
/*	'Less Than', 'Equal to', and 'Greater Than' another. 
/*
/*	'cmp_f' gets the new INFO struct as its second parameter.
/*
/*	All Insert functions receive the following paramaters:
/*		*first, 
/*		*last,
/*		cmp_f,
/*		info;
 */
int
linsbtree( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
	register int	ret;

	if((*first) == NULL_LNODE) {
		if( ((*first)=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
			return(0);
		(*first)->weight = 0;
		(*first)->info   = info;
		(*first)->prev   = NULL_LNODE;
		(*first)->next   = NULL_LNODE;
		return (1);
	}
	ret = (*cmp_f)((*first)->info, info);
	if(ret>0) {
		return(linsbtree( &((*first)->prev),last,cmp_f,info));
	}
	if(ret<0) {
		return(linsbtree( &((*first)->next),last,cmp_f,info));
	}
	/* DUPLICATE */
	return(-2);
}
void
rot_prev( LNODE **lnode )
{
	LNODE **tmp;

	tmp = &((*lnode)->next);
	(*lnode)->next = (*tmp)->prev;
	(*tmp)->prev   = *lnode;
	lnode          = tmp;
}

void
rot_next( LNODE **lnode )
{
	LNODE **tmp;

	tmp = &((*lnode)->prev);
	(*lnode)->prev = (*tmp)->next;
	(*tmp)->next   = *lnode;
	lnode          = tmp;
}

/* L SCAN LIST -- Scan a regular LINKED-LIST. Start at first,
/*	walk each next till end.
/*	Call scan_f at each NODE.
/*
/*	Break off scan if any scan_f returns a 0.
 */
int
lscanl(LNODE *first,int size,PFV scan_f)
{
	register LNODE	*tmp;
	for( tmp=first; tmp!=NULL_LNODE; tmp=tmp->next)
		if( ((*scan_f)(tmp->info)) == 0 )
			return(0);
	return(1);
}

/* SCAN BTREE -- Walk a BTREE LINKED-LIST.
/*	Call scan_f at each NODE.
/*
/*	Break off scan if any scan_f returns a 0.
 */
int
lscanbt(LNODE *first,int size,PFV scan_f)
{
	if(first==NULL_LNODE)
		return(1);	/* This is OK, will happen normally
				/* At each leaf.
				 */

	if(lscanbt(first->prev,size,scan_f) == 0)
		return(0);

	if( ((*scan_f)((first)->info)) == 0 )
		return(0);

	if(lscanbt(first->next,size,scan_f) == 0)
		return(0);
	return(1);
}

/*
/* HASH FUNCTIONS 
 */
#define HASHTABLESIZE 211
static int
GetHashKey(char *pKey)
{
    unsigned long g, h = 0;
    while(*pKey){
	h = (h << 4) + toupper(*pKey++);
	g = h & 0xF0000000L;
	if(g) { h ^=g >> 24; }
	h &= ~g;
    }
    return( h % HASHTABLESIZE);
}

int
linshash( LNODE **first, LNODE **last, PFI cmp_f, void *info)
{
    char *pKey; 
    int iHashIndex;
    LNODE **ppLnode;

    if( (*first)== NULL_LNODE) {
	(*first)= (LNODE*)calloc( HASHTABLESIZE, sizeof(LNODE*) );
		/* I know, I know this really points to an array
		/* of those, but this is the defined structure
		/* of the LIST, so....
		 */
    }
    ppLnode = (LNODE**)(*first);

    /* Get HASH KEY
     */
    pKey = (char *)(*cmp_f)(info);
			/* User supplied function returns the character
			/* string that represnts the HASH KEY
			 */
    if(pKey) {
	char *pKey2;
	register LNODE	*new,
			*pTmp;

	iHashIndex = GetHashKey(pKey);

	if( (new=(LNODE *)malloc( sizeof(LNODE) )) == NULL_LNODE )
		return(0);
	new->weight	= 0;
	new->info	= info;

	/*
	/* fprintf(stderr,"Adding:%s\n",pKey);
	/* fflush (stderr);
	 */

	/* Search the "prev" list for match
	 */ 
	for( new->next=NULL_LNODE, pTmp=ppLnode[iHashIndex]
	    ;pTmp
	    ;new->next=pTmp, pTmp=pTmp->prev)
	{
	    pKey2 = (char *)(*cmp_f)(pTmp->info);
	    /*
	    /* Install a Sibling using PUSH semantics
	     */
	    if(strcmp(pKey,pKey2)==0) {
		LNODE	*pTmp2=pTmp;
		while(pTmp2->next) {
		    pTmp2=pTmp2->next;
		}
		pTmp2->next = new;
		new->next = 0;
		new->prev = 0;

	    /*	fprintf(stderr,"S(%d,%s)\n",iHashIndex,pKey);
	    /*	fflush(stderr);
	     */
		return(1);
	    }
	}
	if(new->next)	/* new->next is last ELEMENT on "prev" list */
	     new->next->prev = new;
	else ppLnode[iHashIndex]=new; /* I'm first */
	new->next = NULL_LNODE;	/* I've got no Siblings/Dups */
	new->prev = NULL_LNODE;	/* I'm LAST on the LIST */
	
	/*
	/* fprintf(stderr,"C(%d,%s)\n",iHashIndex,pKey);
	/* fflush(stderr);
	 */

	return(1);
    }
    return(0);
}
void *
ldelhash(LNODE **first, LNODE **last,LNODE ** current)
{
    register LNODE *pTmp = (*current);
    register void *pInfo;

    if(!(*current))
	return( (void*)0 );
    /* If this node has a "sibling"
     */
    if( (*current)->next ) {
	if( (*current)->prev ) 
	    (*current)->prev = (*current)->next;
	(*current)= (*current)->next;
    } else if( (*current)->prev ) {
	(*current)= (*current)->prev;
    }
    pInfo = pTmp->info;
    free( (pTmp) );

    return(pInfo);
}
LNODE *
lfindhash(LNODE **first, LNODE **last, int size, PFI cmp_f, void *cmp_i)
{
    char *pKey, *pKey2; 
    int iHashIndex;
    LNODE **ppLnode, *pTmp;

    if( (*first)== NULL_LNODE) {
	return( NULL_LNODE );
    }
    ppLnode = (LNODE**)(*first);

    /* Get HASH KEY
     */
    pKey = (char *)(*cmp_f)(cmp_i);
			/* User supplied function returns the character
			/* string that represnts the HASH KEY
			 */
    if(pKey) {
	iHashIndex = GetHashKey(pKey);

	for( pTmp=ppLnode[iHashIndex]; pTmp!=NULL_LNODE; pTmp=pTmp->prev) {
	    pKey2 = (char *)(*cmp_f)(pTmp->info);
	    /* Collision Checking
	    /* fprintf(stderr,"L(%d,%s,%s)\n",iHashIndex,pKey,pKey2);
	    /* fflush(stderr);
	     */

	    if(strcasecmp(pKey,pKey2)==0) {
	    /* if(strcmp(pKey,pKey2)==0) 
	    /*	fprintf(stderr,"H(%d,%s)\n",iHashIndex,pKey2);
	    /*	fflush(stderr);
	     */
		return(pTmp);
	    }
	}
	return( NULL_LNODE );
    }
    return( NULL_LNODE);
}
int
lscanhash(LNODE *first,int size,PFV scan_f)
{
    int i;
    LNODE **ppLnode, *pInd, *pColl, *pSib;

    ppLnode = (LNODE**) first;
    for(i=0; i<HASHTABLESIZE;++i){
	pInd = ppLnode[i];

	if(!pInd)
	    continue;

	for(pColl=pInd; pColl; pColl = pColl->prev){
	    if( pColl->info && ((*scan_f)((pColl)->info)) == 0 ) {
		return(0);
	    }
	    for(pSib=pColl->next;pSib;pSib=pSib->next){
		if(pColl->info && ((*scan_f)((pSib)->info)) == 0 ) {
		    return(0);
		}
	    }
	}
    }
    
    return(0);
}

/* Place holder functions 
 */
int
lnullpfi()
{
	return(0);
}
void *
lnullpfv()
{
	return((void *)0);
}
LNODE *
lnullpfn()
{
	return(NULL_LNODE);
}
