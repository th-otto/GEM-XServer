//==============================================================================
//
// Property.c -- Implementation of struct 'PROPERTY' related functions.
//
// Even  if the server normally doesn't interprete property contents, some of it
// will be reported to the built-in window manager if changed or created.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-06-19 - Initial Version.
//==============================================================================
//
#include "Property_P.h"
#include "tools.h"
#include "event.h"

#include <stdlib.h>


#define _Prop_Find(pool, name) \
                  (pool ? Xrsc(PROPERTY, name, pool->Pool) : NULL)


//==============================================================================
void *
PropValue (const PROPERTIES * pool, Atom name, Atom type, size_t min_len)
{
	void     * data = NULL;
	PROPERTY * prop;
	
	if ((prop = _Prop_Find (pool, name))
	    && (type == None || type    == prop->Type)
	    && (!min_len     || min_len <= prop->Length)) {
		data = prop->Data;
	}
	return data;
}


//==============================================================================
BOOL
PropHasAtom (const PROPERTIES * pool, Atom name, Atom which)
{
	BOOL       has  = xFalse;
	PROPERTY * prop;
	
	if ((prop = _Prop_Find (pool, name))  &&  prop->Type == XA_ATOM) {
		Atom * list = (Atom*)prop->Data;
		int    num  = prop->Length /4;
		int    i;
		for (i = 0; i < num; ++i) {
			if (list[i] == which) {
				has = xTrue;
				break;
			}
		}
	}
	return has;
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ChangeProperty (CLIENT * clnt, xChangePropertyReq * q)
{
	// Alters or creates a property of the given window.
	//
	// Window window:   location for the property
	// Atom   property: to be altered (or created)
	// Atom   type:     uninterpreted, but must match for existing
	// CARD8  mode:     Replace|Prepend|Append
	// CARD8  format:   8/16/32-bit per unit
	// CARD32 nUnits:   length of stuff following, depends on format
	// void   (q +1):   data to be applied
	//...........................................................................
	
	WINDOW     * wind = WindFind (q->window);
	PROPERTIES * pool = NULL;
	PROPERTY   * prop = NULL, * have;
	int factor = (q->format == 8  ? 1 : // number of bytes per unit
	              q->format == 16 ? 2 :
	              q->format == 32 ? 4 : 0);
	
	if (!wind) {
		Bad(Window, q->window, ChangeProperty,"():\n          not %s.",
		                       (DBG_XRSC_TypeError ? "a window" : "found"));
		
	} else if (!AtomValid(q->property)) {
		Bad(Atom, q->property, ChangeProperty,"(W:%lX):\n"
		                        "          invalid property.", q->window);
	
	} else if (!AtomValid(q->type)) {
		Bad(Atom, q->type, ChangeProperty,"(W:%lX,'%s'):\n"
		                   "          undefined type.",
		                   q->window, ATOM_Table[q->property]->Name);
	
	} else if (!factor) {
		Bad(Value, q->format, ChangeProperty,"(W:%lX,'%s'):\n"
		                      "          illegal format.",
		                      q->window, ATOM_Table[q->property]->Name);
	
	} else if (!wind->Properties && !(pool = malloc (sizeof (PROPERTIES)))) {
		Bad(Alloc,, ChangeProperty,"(W:%lX,A:%lu): pool struct.",
		            q->window, q->property);
		
	} else if ((have = _Prop_Find (wind->Properties, q->property)) &&
	            q->mode != PropModeReplace                         &&
	           (q->type != have->Type  ||  q->format != have->Format)) {
		Bad(Match,, ChangeProperty,"(W:%lX,'%s'):\n"
		            "          type = A:%lu/A:%lu, format = %i/%i.",
		            q->window, ATOM_Table[q->property]->Name,
		            q->type, have->Type, q->format, have->Format);
	
	} else { //..................................................................
		
		size_t have_size = (have ? have->Length : 0);
		CARD8  mode      = (have_size ? q->mode : PropModeReplace);
		size_t size      = q->nUnits * factor;
		size_t need_size = size + (mode == PropModeReplace ? 0 : have_size);
		size_t xtra      = (factor == 1 ? 1 :0);
		                   // reserve an extra char space for a trailing '\0'
		
		#	ifdef TRACE
			PRINT (ChangeProperty, "-(W:%lX): '%s'(%s,%i*%li,%i)",
	      		 q->window,
	      		 ATOM_Table[q->property]->Name, ATOM_Table[q->type]->Name,
	      		 factor, q->nUnits, q->mode);
			if (q->type == XA_STRING) {
				PRINT (,"+\n          '%*s'", (int)q->nUnits, (char*)(q +1));
			} else {
				PRINT (,"+");
			}
		#	endif TRACE
		
		if (pool) {
			XrscPoolInit (pool->Pool);
			pool->WindName     = NULL;
			pool->Base.valid   = xFalse;
			pool->Min.valid    = xFalse;
			pool->Max.valid    = xFalse;
			pool->Inc.valid    = xFalse;
			pool->IconName     = NULL;
			pool->IconPmap     = NULL;
			pool->IconMask     = NULL;
			pool->FixedSize    = xFalse;
			pool->ProtoDelWind = xFalse;
			wind->Properties = pool;
		
		} else {
			pool = wind->Properties;
			if (have) {
				XrscRemove (pool->Pool, have);
			}
		}
		
		prop = XrscCreate (PROPERTY, q->property, pool->Pool, need_size + xtra);
		if (!prop) {
			Bad(Alloc,, ChangeProperty,"(W:%lX,A:%lX):\n"
			            "          %lu bytes.", q->window, q->property, need_size);
			if (have) {
				XrscInsert (pool->Pool, have);
			}
		
		} else {
			char * data = prop->Data;
			
			if (mode == PropModePrepend) {
				memcpy (data + size, have->Data, have->Length);
			
			} else if (mode == PropModeAppend) {
				memcpy (data, have->Data, have->Length);
				data += have->Length;
			}
			
			if (size) {
				if (!clnt->DoSwap  ||  q->format == 8) {
					memcpy (data, (q +1), size);
				
				} else if (q->format == 16) {
					CARD16 * src = (CARD16*)(q +1);
					CARD16 * dst = (CARD16*)data;
					size_t   num = q->nUnits;
					while (num--) {
						*(dst++) = Swap16(*(src++));
					}
				
				} else { // q->format == 32
					CARD32 * src = (CARD32*)(q +1);
					CARD32 * dst = (CARD32*)data;
					size_t   num = q->nUnits;
					while (num--) {
						*(dst++) = Swap32(*(src++));
					}
				}
			}
			if (xtra) {
				prop->Data[need_size] = '\0';
			}
			prop->ICCC   = (!have || have->ICCC);
			prop->Type   = q->type;
			prop->Format = q->format;
			prop->Length = need_size;
			
			if (wind->u.List.AllMasks & PropertyChangeMask) {
				EvntPropertyNotify (wind, prop->Id, PropertyNewValue);
			}
			if (prop->ICCC  &&  wind->Handle > 0) {
				_Prop_ICCC (wind, prop, xTrue);
			}
			if (have) {
				free (have);
			}
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_DeleteProperty (CLIENT * clnt, xDeletePropertyReq * q)
{
	// Deletes an (existing) property from the given window.
	//
	// Window window:   location for the property
	// Atom   property: to be altered (or created)
	//...........................................................................
	
	WINDOW * wind = WindFind (q->window);
	
	if (!wind) {
		Bad(Window, q->window, DeleteProperty,"():\n          not %s.",
		                       (DBG_XRSC_TypeError ? "a window" : "found"));
		
	} else if (!AtomValid(q->property)) {
		Bad(Atom, q->property, DeleteProperty,"(W:%lX)", q->window);
	
	} else { //..................................................................
		
		PROPERTY * prop = _Prop_Find (wind->Properties, q->property);
		
		DEBUG (DeleteProperty," '%s'(%s) from W:%lX",
		       ATOM_Table[q->property]->Name,
		       (prop ? ATOM_Table[prop->Type]->Name : "n/a"), q->window);
		
		if (prop) {
			if (wind->u.List.AllMasks & PropertyChangeMask) {
				EvntPropertyNotify (wind, prop->Id, PropertyDelete);
			}
			if (prop->ICCC  &&  wind->Handle > 0) {
				_Prop_ICCC (wind, prop, xFalse);
			}
			XrscDelete (wind->Properties->Pool, prop);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetProperty (CLIENT * clnt, xGetPropertyReq * q)
{
	// Returns the full or partial content of a property.
	//
	// Window window:     location of the property
	// Atom   property:   source to get content from
	// Atom   type:       AnyPropertyType or must match
	// CARD32 longOffset: offset in 32bit units, indipendent of property format
	// CARD32 longLength: length to read in 32bit units
	// BOOL   delete:     remove property if all date were read
	//
	// Reply:
	// Atom   propertyType: None, if property doesn't exist
	// CARD8  format:       0|8|16|32
	// CARD32 bytesAfter:   length of data behind the part read
	// CARD32 nItems:       # of 8, 16, or 32-bit entities in reply
	// (char*)(r +1):       content
	//...........................................................................
	
	WINDOW   * wind = WindFind (q->window);
	CARD32     offs = q->longOffset *4;
	PROPERTY * prop;
	
	if (!wind) {
		Bad(Window, q->window, GetProperty,"():\n          not %s.",
			                    (DBG_XRSC_TypeError ? "a window" : "found"));
		
	} else if (!AtomValid(q->property)) {
		Bad(Atom, q->property ,GetProperty,"(W:%lX)", q->window);
	
	} else if ((prop = _Prop_Find (wind->Properties, q->property)) &&
	           offs > prop->Length) {
		Bad(Value, q->longOffset, GetProperty,"(W:%lX,A:%lu)\n"
		                          "          offset %lu > property length %u.",
		                          q->window, q->property, offs, prop->Length);
	
	} else { //..................................................................
		
		ClntReplyPtr (GetProperty, r,);
		size_t len  = 0;
		
		if (!prop) {
			r->propertyType = None;
			r->format       = 0;
			r->bytesAfter   = 0;
			r->nItems       = 0;
		
		} else if (q->type != AnyPropertyType  &&  q->type != prop->Type ) {
			r->propertyType = prop->Type;
			r->format       = prop->Format;
			r->bytesAfter   = prop->Length;
			r->nItems       = 0;
			
		} else {
			CARD8  bytes = prop->Format /8;
			len = (q->longLength *4) +offs;
			len = (len <= prop->Length ? len : prop->Length) - offs;
			if (len) {
				ClntReplyPtr (GetProperty, rr, len);
				r = rr;
			}
			r->propertyType = prop->Type;
			r->format       = prop->Format;
			r->bytesAfter   = (prop->Length - offs) - len;
			r->nItems       = len / bytes;
			
			if (!len) {
				r->nItems = 0;
				
			} else if (!clnt->DoSwap  ||  prop->Format == 8) {
				memcpy ((r +1), prop->Data + offs, len);
				
			} else if (prop->Format == 16) {
				CARD16 * src = (CARD16*)(prop->Data + offs);
				CARD16 * dst = (CARD16*)(r +1);
				size_t   num = r->nItems;
				while (num--) {
					*(dst++) = Swap16(*(src++));
				}
				
			} else { // prop->Format == 32
				CARD32 * src = (CARD32*)(prop->Data + offs);
				CARD32 * dst = (CARD32*)(r +1);
				size_t   num =  r->nItems;
				while (num--) {
					*(dst++) = Swap32(*(src++));
				}
			}
		}
		
		DEBUG (GetProperty," '%s'(%s,%u) %lu-%lu from W:%lX",
		       ATOM_Table[q->property]->Name,
		       ATOM_Table[r->propertyType]->Name,
		       r->format, offs, len, q->window);
		
		ClntReply (GetProperty, len, "all");
		
		if (prop && q->delete && !r->bytesAfter) {
			if (wind->u.List.AllMasks & PropertyChangeMask) {
				EvntPropertyNotify (wind, prop->Id, PropertyDelete);
			}
			if (prop->ICCC  &&  wind->Handle > 0) {
				_Prop_ICCC (wind, prop, xFalse);
			}
			XrscDelete (wind->Properties->Pool, prop);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ListProperties (CLIENT * clnt, xListPropertiesReq * q)
{
	// Returns a list of the atoms of all properties currently defined on the
	// window.
	//
	// CARD32 id: window
	// 
	// Reply:
	// CARD16 nProperties: # of atoms
	// (Atom*)(r +1)       list of atoms
	//...........................................................................
	
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(Window, q->id, ListProperties,"():\n          not %s.",
		                   (DBG_XRSC_TypeError ? "a window" : "found"));
	
	} else { //..................................................................
		
		ClntReplyPtr (ListProperties, r,);
		PROPERTIES * pool = wind->Properties;
		Atom       * atom = (Atom*)(r +1);
		CARD16       num  = 0;
		size_t       size = 0;
		size_t       bspc = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
		                  - sz_xListPropertiesReply;
		
		if (pool) {
			int i;
			for (i = 0; i < XrscPOOLSIZE (pool->Pool); ++i) {
				PROPERTY * prop = XrscPOOLITEM (pool->Pool, i);
				while (prop) {
					size_t need = size + sizeof(ATOM);
					if (need > bspc) {
						r = ClntOutBuffer (&clnt->oBuf,
						                   sz_xListPropertiesReply + need,
						                   sz_xListPropertiesReply + size, xTrue);
						atom = (Atom*)(r +1) + num;
						bspc = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
						     - sz_xListPropertiesReply;
					}
					*(atom++) = (clnt->DoSwap ? Swap32(prop->Id) : prop->Id);
					num++;
					size = need;
					prop = prop->NextXRSC;
				}
			}
		}
		DEBUG (ListProperties," of W:%lX (%u)", q->id, num);
		
		r->nProperties = num;
		ClntReply (ListProperties, size, ".");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_RotateProperties (CLIENT * clnt, xRotatePropertiesReq * q)
{
	// Every property named by the given atom list gets the value from the
	// property that is delta list entries apart.
	// In fact, all properties get the nameing atom from the property that is
	// -delta entries apart, same result.
	//
	// Window window:     location for the properties
	// CARD16 nAtoms:     # of properties to be rotated
	// INT16  nPositions: delta value
	// (Atom*)(q +1)      list of properties
	//...........................................................................
	
	WINDOW * wind  = WindFind (q->window);
	CARD16   num   = q->nAtoms;
	CARD16   delta = q->nPositions % num;
	
	if (!wind) {
		Bad(Window, q->window, RotateProperties,"():\n          not %s.",
		                       (DBG_XRSC_TypeError ? "a window" : "found"));
	
	} else if (q->nAtoms) { //...................................................
		
		PROPERTIES * pool = wind->Properties;
		BOOL         ok   = xTrue;
		Atom       * name = (Atom*)(q +1);
		PROPERTY   * prop[num];
		int i, j;
		
		if (clnt->DoSwap) for (i = 0; i < num; i++) {
			name[i] = Swap32(name[i]);
		}
		for (i = 0; i < num; i++) {
			if (!AtomValid (name[i])) {
				Bad(Atom, name[i], RotateProperties,"(W:%lX)", q->window);
				ok = xFalse;
				break;
	
			} else if (!(prop[(i + delta) % num] = _Prop_Find (pool, name[i]))) {
				Bad(Match,, RotateProperties,"(W:%lX):\n"
				            "          property A:%lX not found.",
				            q->window, name[i]);
				ok = xFalse;
				break;
			
			} else for (j = 0; j < i; ++j) {
				if (name[j] == name[i]) {
					Bad(Match,, RotateProperties,"(W:%lX):\n"
					            "          property A:%lX occured more than once.",
					            q->window, name[i]);
					ok = xFalse;
					i  = q->nAtoms;
					break;
				}
			}
		}
		if (ok && delta) {
			BOOL notify = (0 != (wind->u.List.AllMasks & PropertyChangeMask));
			BOOL iccc   = (wind->Handle > 0);
			
			for (i = 0; i < num; i++) {
				XrscRemove (pool->Pool, prop[i]);
				prop[i]->Id = name[i];
			}
			for (i = 0; i < num; i++) {
				XrscInsert (pool->Pool, prop[i]);
				if (notify) EvntPropertyNotify (wind, name[i], PropertyNewValue);
				if (iccc)   _Prop_ICCC (wind, prop[i], xTrue);
			}
	
			DEBUG (RotateProperties,"(W:%lX,%u,%+i)",
			       q->window, q->nAtoms, q->nPositions);
		}
	}
}
