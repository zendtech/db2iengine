/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: aix71D bos/usr/include/malloc.h 1$ */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 1985,1994              */
/*                                                                        */
/* Pvalue: p3 */
/* Licensed Materials - Property of IBM                                   */
/*                                                                        */
/* US Government Users Restricted Rights - Use, duplication or            */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.      */
/*                                                                        */
/* Origin: 27 */
/*                                                                        */
/* $Header: @(#) AIX71D_area/1 bos/usr/include/malloc.h, libcgen, aix71D, 1115A_71D 2011-03-29T02:27:28-05:00$ */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/*
 * COMPONENT_NAME: (LIBCGEN) Standard C Library General Functions
 *
 * FUNCTIONS: 
 *
 * ORIGINS: 27
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1994
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */
#ifndef _H_MALLOC
#define _H_MALLOC

/*
	Constants defining mallopt operations
*/
#define M_MXFAST	1	/* set size of blocks to be fast */
#define M_NLBLKS	2	/* set number of block in a holding block */
#define M_GRAIN		3	/* set number of sizes mapped to one, for
				   small blocks */
#define M_KEEP		4	/* retain contents of block after a free until
				   another allocation */
#define	M_DISCLAIM	5	/* disclaim free'd memory */
#define	M_MALIGN	6	/* set default allocation alignment */
/*
	structure filled by mallinfo
*/
struct mallinfo  {
   unsigned long arena;		/* total space in arena */
   int ordblks;			/* number of ordinary blocks */
   int smblks;			/* number of small blocks */
   int hblks;			/* number of holding blocks */
   int hblkhd;			/* space in holding block headers */
   unsigned long usmblks;	/* space in small blocks in use */
   unsigned long fsmblks;	/* space in free small blocks */
   unsigned long uordblks;	/* space in ordinary blocks in use */
   unsigned long fordblks;	/* space in free ordinary blocks */
   int keepcost;		/* cost of enabling keep option */
#ifdef SUNINFO
   int mxfast;			/* max size of small block */
   int nblks;			/* number of small blocks in holding block */
   int grain;			/* small block rounding factor */
   unsigned long uordbytes;	/* space allocated in ordinary blocks */
   int allocated;		/* number of ordinary blocks allocated */
   unsigned long treeoverhead;	/* bytes used in maintaining in free tree */
#endif
};

/*
        struct filled by mallinfo_heap
*/
struct mallinfo_heap {
   unsigned long root_sz;       /* size of biggest free chunk in arena */
   unsigned long acquired;      /* total acquired storage including  actual data
                                   overhead */
   unsigned long blks_alloc;    /* number of blocks alocated including the free tree */
   unsigned long netalloc;      /* heap space already malloced */
   unsigned long bytes_free;    /* total bytes in free tree */
};


#ifdef   _NO_PROTO
extern int mallopt();
extern struct mallinfo mallinfo();
extern struct mallinfo_heap mallinfo_heap();
#else  /*_NO_PROTO */
extern int mallopt(int, int);
extern struct mallinfo mallinfo(void);
extern struct mallinfo_heap mallinfo_heap(int);
#endif /*_NO_PROTO */

/*************************************************************************
 * Malloc Log
 *************************************************************************/
#define DEFAULT_RECS_PER_HEAP   4096
#define MAX_RECS_PER_HEAP       65535
#define DEFAULT_STACK_DEPTH     4
#define MAX_STACK_DEPTH         64  

#ifndef MALLOC_LOG_STACKDEPTH
#define MALLOC_LOG_STACKDEPTH DEFAULT_STACK_DEPTH
#endif 

#include <sys/types.h> /* for size_t and uintptr_t */
struct malloc_log {
   size_t size;		/* size of the allocation */
   size_t cnt;		/* number of allocations that match this particular
			 * size and stack traceback */
   uintptr_t callers [MALLOC_LOG_STACKDEPTH];
			/* stack traceback of the allocation */
};

size_t			get_malloc_log 		(void *, void *, size_t);
struct malloc_log * 	get_malloc_log_live 	(void *);
void			reset_malloc_log 	(void * addr);


#endif /* _H_MALLOC */
