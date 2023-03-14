//==============================================================================
//
// Prop_ICCC.c -- 'Inter Client Communication Convention' related functions.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-06-30 - Initial Version.
//==============================================================================
//
#include "Property_P.h"
#include "pixmap.h"
#include "wmgr.h"
#include "ICCC.h"

#include <stdlib.h>
#include <stdio.h>


//==============================================================================
void
PropDelete (PROPERTIES ** pPool)
{
	PROPERTIES * pool = *pPool;
	
	if (pool) {
		int i;
		for (i = 0; i < XrscPOOLSIZE (pool->Pool); ++i) {
			p_PROPERTY p;
			while ((p = XrscPOOLITEM (pool->Pool, i))) XrscDelete (pool->Pool, p);
		}
		if (pool->IconPmap) PmapFree (pool->IconPmap, NULL);
		if (pool->IconMask) PmapFree (pool->IconMask, NULL);
		free (pool);
		*pPool = NULL;
	}
}


//------------------------------------------------------------------------------
void
_Prop_ICCC (WINDOW * wind, PROPERTY * prop, BOOL changed)
{
	PROPERTIES * pool = wind->Properties;
	BOOL         iccc = xTrue;
	
	#define CONTAINS( p, s, m) (p->Length >= offsetof(s, m) \
	                                       + sizeof (((s*)0uL)->m))
	switch (prop->Id) {
		
		case XA_WM_COMMAND: if (prop->Type == XA_STRING) {
			WmgrClntUpdate (ClntFind (wind->Id), prop->Data);
		}	break;
		
		case XA_WM_NAME: if (prop->Type == XA_STRING) {
			if (changed) {
				WmgrWindName (wind, prop->Data, xFalse);
				pool->WindName = prop->Data;
		} else {
				WmgrWindName (wind, "", xFalse);
				pool->WindName = NULL;
			}
		}	break;
		
		case XA_WM_ICON_NAME: if (prop->Type == XA_STRING) {
			if (changed) {
				WmgrWindName (wind, prop->Data, xTrue);
				pool->IconName = prop->Data;
		} else {
				WmgrWindName (wind, "", xTrue);
				pool->IconName = NULL;
			}
		}	break;
		
		case XA_WM_HINTS: if (prop->Type == XA_WM_HINTS) {
			WmHints * hints  = (WmHints*)prop->Data;
			PIXMAP  * pmap   = pool->IconPmap;
			PIXMAP  * mask   = pool->IconMask;
			BOOL      notify = xFalse;
			if (changed && (hints->flags & IconPixmapHint)
				         && CONTAINS (prop, WmHints, icon_pixmap)) {
				pmap = PmapFind (hints->icon_pixmap);
			} else {
				pmap = NULL;
			}
			if (pmap != pool->IconPmap) {
				if (pool->IconPmap) PmapFree  (pool->IconPmap, NULL);
				pool->IconPmap    = PmapShare (pmap);
				notify            = xTrue;
			}
			if (changed && (hints->flags & IconMaskHint)
				         && CONTAINS (prop, WmHints, icon_mask)) {
				mask = PmapFind (hints->icon_mask);
			} else {
				mask = NULL;
			}
			if (mask != pool->IconMask) {
				if (pool->IconMask) PmapFree  (pool->IconMask, NULL);
				pool->IconMask    = PmapShare (mask);
				notify            = xTrue;
			}
			if (notify) WmgrWindIcon (wind);
		}	break;
		
		case XA_WM_NORMAL_HINTS: if (prop->Type == XA_WM_SIZE_HINTS) {
			if (changed) {
				SizeHints * hints  = (SizeHints*)prop->Data;
				if ((hints->flags & PBaseSize)
				    && CONTAINS (prop, SizeHints, base_height)) {
					pool->Base.Size.x = hints->base_width;
					pool->Base.Size.y = hints->base_height;
				} else {
					pool->Base.valid  = xFalse;
				}
				if ((hints->flags & PMinSize)
				    && CONTAINS (prop, SizeHints, min_height)) {
					pool->Min.Size.x = hints->min_width;
					pool->Min.Size.y = hints->min_height;
					pool->FixedSize  = xTrue;
				} else {
					pool->Min.valid  = xFalse;
					pool->FixedSize  = xFalse;
				}
				if ((hints->flags & PMaxSize)
				    && CONTAINS (prop, SizeHints, max_height)) {
					pool->Max.Size.x = hints->max_width;
					pool->Max.Size.y = hints->max_height;
				} else {
					pool->Max. valid = xFalse;
					pool->FixedSize  = xFalse;
				}
				if ((hints->flags & PResizeInc)
				    && CONTAINS (prop, SizeHints, inc_height)) {
					pool->Inc.Step.x = hints->inc_width;
					pool->Inc.Step.y = hints->inc_height;
				} else {
					pool->Inc.valid  = xFalse;
				}
			} else {
				pool->Base.valid = xFalse;
				pool->Min.valid  = xFalse;
				pool->Max.valid  = xFalse;
				pool->Inc.valid  = xFalse;
			}
		}	break;
		
		case WM_PROTOCOLS: if (prop->Type == XA_ATOM) {
			pool->ProtoDelWind = xFalse;
			if (changed) {
				Atom * atom = (Atom*)prop->Data;
				int    num  = prop->Length /4;
				while (num--) {
					switch (*atom) {
						case WM_DELETE_WINDOW: pool->ProtoDelWind = xTrue; break;
						
						default: {
							CLIENT * clnt = ClntFind (wind->Id);
							PRINT (,"-_Prop_ICCC(W:%X): unknown WM_PROTOCOL ",
							        wind->Id);
							if (AtomValid(*atom)) {
								printf ("'%s'.\n", ATOM_Table[*atom]->Name);
							} else {
								printf ("A:%lX.\n", *atom);
							}
						}
					}
					atom++;
				}
			}
		}	break;
		
		default:
			iccc = xFalse;
	}
	#undef CONTAINS
	
	if (changed) prop->ICCC = iccc;
}
