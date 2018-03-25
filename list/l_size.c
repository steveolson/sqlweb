/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
/* 
/* L_SIZE.C -- Return number of Elements in the LIST.
/*
/* */ 

#include "list.h"

int
l_size(LIST *ll)
{
	if(ll)	return(ll->size);
	return(0);
}
