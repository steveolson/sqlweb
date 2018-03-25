/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
/*	The LINKED-LIST API.
/*
/*	The main LINKED-LIST routines.  This file contains most of the API
/*		functions -- some are in seperate modules so that target
/*		executables are as small as possible.
/*
/*	Contained in this module are the following functions:
/*		l_create()
/*		l_destroy()
/*		l_insert()
/*		l_delete()
/*		l_find()
/* */ 


#include "list.h"
#include "stdlib.h"

/* L_CREATE -- Create a LIST.
/*	Create takes one paramater -- the name of the LIST CLASS, the
/*	name is used to lookup the INSERT, DELETE, and FIND functions
/*	from the LIST FUNCTION TABLE.
 */
LIST *
l_create(char *name)
{
	register LIST	*ll;
	PFI lgetinsf();
	PFV lgetdelf();
	PFN lgetfindf();
	PFI lgetscanf();

	if( (ll=(LIST *)malloc(sizeof(LIST))) == NULL_LIST )
		return( NULL_LIST );
	if(	((ll->f_insert = lgetinsf(name)) == NULL_PFI ) ||
		((ll->f_delete = lgetdelf(name)) == NULL_PFV ) ||
		((ll->f_scan   = lgetscanf(name))== NULL_PFI ) ||
		((ll->f_find   = lgetfindf(name))== NULL_PFN )
	) {
		free( ll );
		return( NULL_LIST );
	}

	ll->size	= 0;
	ll->first	= NULL_LNODE;
	ll->last	= NULL_LNODE;
	ll->current	= NULL_LNODE;
	return(ll);
}


/* L_DESTROY -- Destroy a LIST.
/*
/*	Destroy takes one paramater -- the LIST to be destroyed.
 */
int
l_destroy(LIST *ll)
{
	if(ll==NULL_LIST)
		return ( 0 );
	while(ll->first) {
		ll->current=ll->first;
		ll->first=ll->first->next;
		free(ll->current);
	}
	free(ll);
	return(1);	/* returns TRUE */
}

/* L_INSERT -- Insert an Element into the LIST.
/*	Insert takes three paramaters
/*		LIST, the compare function, and the INFO to be INSERTED.
/*
/*	The compare function is used for ASCEND and DESCEND LIST CLASSES
/*	so that the INFO gets inserted in the proper order.
/*
/*	The compare function must take two parameters both the same type as
/*	INFO.  INFO already in LIST is given as the FIRST parameter and the
/*	NEW INFO is always the SECOND parameter. 
/*
/*	The compare function must know how to compare the two INFO and what
/*	constitutes one being 'Greater Than', 'Less Than', and 'Equal To'
/*	another.  If the First Parameter is 'Greater Than' and Second the
/* 	compare function should return an INTEGER Greater Than 0.  If the
/*	First Parameter is 'Less Than' the Second it should return an INTEGER
/*	Less Than 0.  If the two are Equal the Compare function should
/*	return 0.
 */
int
l_insert(LIST *ll, PFI cmp_f, void *info)
{
	register int	val;

	if(ll==NULL_LIST)
		return ( 0 );

	if( (val=((*ll->f_insert)(&ll->first, &ll->last, cmp_f, info)))==1)
		ll->size++;
	return( val );
}

/* L_DELETE -- Delete an Element LIST.
/*
/*	Delete takes 4 arguments:
/*		LIST, find function, compare function, and compare INFO.
/*	Valid 'find function' are: NULL_PFV, l_find, l_ffirst, l_fnext,
/*		and l_fprev.  (NULL_PFV is used for QUEUE and STACK LISTS).
/*		The 'find function' sets ll->current.
/*
/*	l_delete returns a pointer to the Deleted INFO so that the USER
/*	can manually free it.
/*
/*	The 'Compare function' is the same as INSERT.
 */
void *
l_delete(LIST *ll,PFV find_f,PFI cmp_f,void *cmp_i)
{
	register void	*info;

	if(ll==NULL_LIST)
		return ( (void *)0 );

	if( find_f==NULL_PFV || ((*find_f)(ll,cmp_f,cmp_i)) ) {
		if((info=((*ll->f_delete)(&ll->first,&ll->last,&ll->current))))
			ll->size--;
		return(info);
	}
	return( (void *)0 );
}

/* L_FIND -- Find an Element in the LIST.
/*	Find takes three paramater.
/*		LIST, compare function, and compare information.
/*	Compare function is same as INSERT.
 */
void *
l_find(LIST *ll, PFI cmp_f, void *cmp_i)
{
    if(ll==NULL_LIST)
	return ( (void *)0 );

    if((ll->current=(*ll->f_find)(&ll->first,&ll->last,ll->size,cmp_f,cmp_i)))
	return ( ll->current->info );
    return ( (void *)0 );
}
/* L_SCAN -- SCAN entire LIST and invoke scan_f for every node.
/*	SCAN takes two paramaters.
/*		LIST and scan function.
 */
int
l_scan(LIST *ll, PFI scan_f)
{
	if(ll==NULL_LIST)
	    return(0);
	return (*ll->f_scan)(ll->first,ll->size,scan_f);
}
