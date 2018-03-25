/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */
/*
/* L_FFIRST.C -- Find First Element in LIST.
/*	set current to first, return current->info.
/* */

#include "list.h"

void *
l_ffirst( LIST *ll )
{
	if(ll==NULL_LIST)
	    return ( (void *)0 );

	if( ll->first ) {
		ll->current = ll->first;
		return ( ll->current->info );
	}
	ll->current = NULL_LNODE;
	return ( (void *) 0 );
}
