/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
/*
/* L_FPREV.C -- Find PREV Element in LIST
/*	set current = current->prev; return current->info.
/* */ 

#include "list.h"

void *
l_fprev(LIST *ll )
{
	if(ll==NULL_LIST)
	    return ( (void *)0 );

	if( ll->current ){
		ll->current = ll->current->prev;
		if( ll->current )
			return( ll->current->info );
	}
	return ( (void *)0 );
}
