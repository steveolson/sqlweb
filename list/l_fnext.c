/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
/*
/* L_FNEXT.C -- Find Next Element in LIST
/*	set current = current->next; return current->info.
/* */ 

#include "list.h"

void *
l_fnext(LIST *ll )
{
	if(ll==NULL_LIST)
	    return ( (void *)0 );

	if( ll->current ){
		ll->current = ll->current->next;
		if( ll->current )
			return( ll->current->info );
	}
	return ( (void *)0 );
}
