/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
/*
/* L_FLAST.C -- Find Last Element in LIST
/*	set current to last, return current->info.
/* */ 

#include "list.h"

void *
l_flast(LIST *ll )
{
	if(ll==NULL_LIST)
	    return ( (void *)0 );

	if( ll->last ) {
		ll->current = ll->last;
		return ( ll->current->info );
	}
	ll->current = NULL_LNODE;
	return ( (void *) 0 );
}
