//==============================================================================
//
// xrsc.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-02 - Initial Version.
//==============================================================================
//
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <X11/Xmd.h>

#include "xrsc.h"
#include "x_printf.h"

#include <X11/Xproto.h>


BOOL DBG_XRSC_TypeError = xFalse;


#if 0
# define HEAVY_DEBUG

static CARD32 DEBUG_SUM (p_XRSCPOOL pool)
{
	CARD32 sum = pool->mask_bits;
	int    i   = sum;
	do { sum += (CARD32)pool->item[i]; } while (i--);
	
	return sum;
}

#define DEBUG_CHK(pool,func,id) \
	if (pool->DEBUG_chk != DEBUG_SUM (pool)) { \
		x_printf ("\n###" #func "(%p,Id:%X) pool destroyed!\n\n", pool, id); \
		exit(1); \
	}

static char __mem[1024*1024*2], * __ptr = __mem;

static void * _Xrsc_malloc (size_t size)
{
	void * m = (void*)__ptr;
	__ptr += (size +3) & ~3uL;
	
	return m;
}
#define malloc(s) _Xrsc_malloc(s)
#define free(m)   {}

void _Xrsc_Free (void * m);
void _Xrsc_Free (void * m)
{
	char * p = m;
	
	if (p >= __mem  &&  p < __mem + sizeof(__mem)) {
		x_printf("\n\nPLONK %lX\n", p - __mem);
		exit(1);
	}
}

#define DEBUG_SET(pool) \
	pool->DEBUG_chk = DEBUG_SUM(pool)

#else
# define DEBUG_CHK(pool,func,id)
# define DEBUG_SET(pool)
#endif


//==============================================================================
void
_xrsc_pool_init (p_XRSCPOOL pool, size_t size)
{
	size -= sizeof(struct s_XRSCPOOL) - sizeof(p_XRSC);
	
#ifdef HEAVY_DEBUG
	if (size & 3) {
		x_printf("\nsize = %li & 3\n", size);
		exit(1);
	}
#endif
	
	memset (&pool->item, 0, size);
	pool->mask_bits = (size /4) -1;
	DEBUG_SET (pool);
}


//==============================================================================
void
_xrsc_insert (p_XRSCPOOL pool, p_XRSC r)
{
	p_XRSC * p = & pool->item[r->Id & pool->mask_bits];
	
	DEBUG_CHK (pool, _xrsc_insert, r->Id);
	
#ifdef HEAVY_DEBUG
	if (r->DEBUG_tag) {
		x_printf ("### _xrsc_insert(%p,Id:%X) tag not cleared!\n\n", pool, r->Id);
		exit(1);
	}
	r->DEBUG_tag = pool;
#endif HEAVY_DEBUG
	
	r->NextXRSC = *p;
	*p          = r;
	
	DEBUG_SET (pool);
}


//==============================================================================
p_XRSC
_xrsc_create (p_XRSCPOOL pool, CARD32 id, size_t size)
{
	p_XRSC r = malloc (size);
	if (r) {
		r->NextXRSC = NULL;
		r->Id       = id;
		// the flag bitfield stays undefined!
		
		_xrsc_insert (pool, r);
	
	} else {
		x_printf ("_xrsc_create(%lX,%li): memory exhaustetd.\n", id, size);
	}
	return r;
}


//==============================================================================
BOOL
_xrsc_remove (p_XRSCPOOL pool, p_XRSC r)
{
	p_XRSC * p = & pool->item[r->Id & pool->mask_bits];
	
	DEBUG_CHK (pool, _xrsc_remove, r->Id);
	
#ifdef HEAVY_DEBUG
	if (!r->DEBUG_tag) {
		x_printf ("### _xrsc_remove(%p,Id:%X) tag is cleared!\n\n", pool, r->Id);
		exit(1);
	}
#endif HEAVY_DEBUG

	while (*p) {
#ifdef HEAVY_DEBUG
		if ((*p)->DEBUG_tag != pool) {
			x_printf ("### _xrsc_remove(%p,Id:%X) invalid tag %p in Id:%X!\n\n",
			         pool, r->Id, (*p)->DEBUG_tag, (*p)->Id);
			exit(1);
		}
#endif HEAVY_DEBUG

		if (*p == r) {
			*p          = r->NextXRSC;
			r->NextXRSC = NULL;
#ifdef HEAVY_DEBUG
			r->DEBUG_tag = NULL;
#endif HEAVY_DEBUG
			
			DEBUG_SET (pool);
			
			return xTrue;
		}
		p = &(*p)->NextXRSC;
	}
#ifdef HEAVY_DEBUG
	x_printf ("### _xrsc_remove(%p,Id:%X) not in pool!\n\n", pool, r->Id);
	exit(1);
#endif HEAVY_DEBUG
	
	return xFalse;
}


//==============================================================================
BOOL
_xrsc_delete (p_XRSCPOOL pool, p_XRSC r)
{
	BOOL ok = _xrsc_remove (pool, r);
	
	if (ok) free (r);
	
	return ok;
}


//==============================================================================
p_XRSC
_xrsc_search (const struct s_XRSCPOOL * pool, CARD32 id)
{
	p_XRSC r = pool->item[id & pool->mask_bits];
	
	DEBUG_CHK (pool, _xrsc_search, (unsigned int)id);
	
	while (r) {
#ifdef HEAVY_DEBUG
		if (r->DEBUG_tag != pool) {
			x_printf ("### _xrsc_search(%p,Id:%lX) invalid tag %p in Id:%X!\n\n",
			         pool, id, r->DEBUG_tag, r->Id);
			exit(1);
		}
#endif HEAVY_DEBUG
		if (r->Id == id) break;
		r = r->NextXRSC;
	}
	return r;
}
