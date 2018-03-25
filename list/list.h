/**********************************************************************
/* Copyright (C) 1990 - 1999	Steve A. Olson
/* 				11617 Quarterfield Dr
/*				Ellicott City, MD, 21042
/* All rights reserved.
/**********************************************************************
/* */ 
#ifndef LINKED_LIST_HEADER
#define LINKED_LIST_HEADER 1
/* */ 
/*
/*	List.h	Generic Linked-List Header File
/*
/* */ 


/*
/*	The LNODE structure is two-way LINKED-LIST that holds
/*	user-malloced info.  LNODE is the LINKED-LIST pointed to and
/*	manipulated by LIST (defined below).
 */
typedef struct l_node {
	int		weight;	/* not used */
	struct l_node	*next,	/* Next     LNODE in LINKED-LIST */
			*prev;	/* Previous LNODE in LINKED-LIST */
	void		*info;	/* User Malloc'd */
} LNODE;

/* Definitions for easier declarations later
 */
typedef	int	(*PFI)();	/* Pointer to Function returning int       */
typedef	void	*(*PFV)();	/* Pointer to Function returning (void *)  */
typedef LNODE	*(*PFN)();	/* Pointer to Function returning (LNODE *) */

/* The LIST Struct is the controlling node for each LINKED-LIST.
 */
typedef struct {
	int	size;		/* Number of elements in LIST */
	PFI	f_insert;	/* Pointer to INSERT function */
	PFV	f_delete;	/* Pointer to DELETE function */
	PFN	f_find;		/* Pointer to FIND   function */
	PFI	f_scan;  	/* Pointer to WALK   function */
	LNODE	*first,		/* Pointer to First Node in LIST */
		*last,		/* Pointer to Last  Node in LIST */
		*current;	/* Pointer to Last 'found' Node  */
} LIST;

/* Convenience Macros
 */
#define NULL_LIST	(LIST *)0
#define NULL_LNODE	(LNODE *)0
#define NULL_INFO	(void *)0
#define NULL_PFI	(PFI)0
#define NULL_PFV	(PFV)0
#define NULL_PFN	(PFN)0


/* l_display -- Calls function 'f' for each element in LIST 'll'
/*	l_display was written as debugging function to print entire list 'll'
/*	using display funtion 'f' 
 */
#define l_display(ll,f)	l_scan(ll,f)

/* Macros for Stact Manipulation
 */
typedef	LIST		STACK;
#define PUSH(ll,i)	l_insert(ll,NULL_PFI,i)
#define POP(ll)		l_delete(ll,NULL_PFV,NULL_PFI,NULL_INFO)

/* Macros for QUEUE Manipulation
 */
typedef	LIST		QUEUE;
#define ENQ(ll,i)	l_insert(ll,NULL_PFI,i)
#define DEQ(ll)		l_delete(ll,NULL_PFV,NULL_PFI,NULL_INFO)

#ifdef __STDC__
extern	LIST	 *l_create(char *pName)
	;
extern	void	 *l_delete(LIST *ll,PFV find_f,PFI cmp_f,void *cmp_i)
		,*l_find(LIST *ll, PFI cmp_f, void *cmp_i)
		,*l_ffirst( LIST *ll )
		,*l_fnext(LIST *ll )
		,*l_fprev(LIST *ll)
		,*l_flast(LIST *ll)
	;
extern	int	 l_destroy(LIST *ll)
		,l_insert(LIST *ll, PFI cmp_f, void *info)
		,l_size(LIST *ll)
		,l_scan(LIST *ll, PFI scan_f)
	;
#else
extern	LIST	*l_create();
extern	void	*l_delete(),	*l_find(),	*l_ffirst(),	*l_fnext(),
		*l_fprev(),	*l_flast();
extern	int	 l_destroy(),	 l_insert(),	 l_size(),	 l_scan();
#endif

#endif /* LINKED_LIST_HEADER */
