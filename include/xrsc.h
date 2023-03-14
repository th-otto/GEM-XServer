//==============================================================================
//
// xrsc.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-10-02 - Initial Version.
//==============================================================================
//
#ifndef __XRSC_H__
# define __XRSC_H__


#define XrscPOOLSIZE( pool )      ((pool).xrsc.mask_bits +1)
#define XrscPOOLITEM( pool, n )   (pool).type.item[n]


#define XRSC( T, flag ) \
	p_##T  NextXRSC; \
	BOOL   flag : 1; \
	CARD32 Id   :31  \
//	;void * DEBUG_tag


typedef struct s_XRSC *   p_XRSC;

struct s_XRSC {
	XRSC( XRSC, flag );
};


#define struct_XRSCPOOL( s, T, n ) struct s { \
/*	CARD32 DEBUG_chk;*/  \
	CARD32 mask_bits;    \
	p_##T  item[1uL<<n]; \
}

typedef struct_XRSCPOOL( s_XRSCPOOL, XRSC, 0 ) * p_XRSCPOOL;

#define XRSCPOOL( T, name,n ) \
	union {                        \
		struct s_XRSCPOOL     xrsc; \
		struct_XRSCPOOL(,T,n) type; \
	} name


#define CastXRSC(r)   ((p_XRSC)&r->NextXRSC)

#define XrscCreate( T, id, pool, size...) \
                                   ((T*)_xrsc_create (&(pool).xrsc,id, \
                                                         sizeof(T) + (size +0)))
#define Xrsc(       T, id, pool )  ((T*)_xrsc_search (&(pool).xrsc,id))
#define XrscInsert( pool, r )            _xrsc_insert(&(pool).xrsc,CastXRSC(r))
#define XrscRemove( pool, r )            _xrsc_remove(&(pool).xrsc,CastXRSC(r))
#define XrscDelete( pool, r )            _xrsc_delete(&(pool).xrsc,CastXRSC(r))

#define XrscPoolInit( pool )         _xrsc_pool_init (&(pool).xrsc,sizeof(pool))


p_XRSC _xrsc_search (const struct s_XRSCPOOL * , CARD32 id);
p_XRSC _xrsc_create (p_XRSCPOOL , CARD32 id, size_t size);
void   _xrsc_insert (p_XRSCPOOL , p_XRSC);
BOOL   _xrsc_remove (p_XRSCPOOL , p_XRSC);
BOOL   _xrsc_delete (p_XRSCPOOL , p_XRSC);

void _xrsc_pool_init (p_XRSCPOOL , size_t size);

extern BOOL DBG_XRSC_TypeError;


#endif __XRSC_H__
