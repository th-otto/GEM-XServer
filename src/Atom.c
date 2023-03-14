//==============================================================================
//
// Atom.c -- Implementation of struct 'ATOM' related functions.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-06-18 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "tools.h"
#include "Atom.h"
#include "clnt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xatom.h>


#define MAX_ATOM   200

ATOM * ATOM_Table[MAX_ATOM +2] = {
#	define ENTRY(e) \
               (ATOM*) "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" #e
	ENTRY (None),
	ENTRY (XA_PRIMARY),			ENTRY (XA_SECONDARY),	ENTRY (XA_ARC),
	ENTRY (XA_ATOM),				ENTRY (XA_BITMAP), 		ENTRY (XA_CARDINAL),
	ENTRY (XA_COLORMAP),			ENTRY (XA_CURSOR),		ENTRY (XA_CUT_BUFFER0),
	ENTRY (XA_CUT_BUFFER1),		ENTRY (XA_CUT_BUFFER2),	ENTRY (XA_CUT_BUFFER3),
	ENTRY (XA_CUT_BUFFER4),		ENTRY (XA_CUT_BUFFER5),	ENTRY (XA_CUT_BUFFER6),
	ENTRY (XA_CUT_BUFFER7),		ENTRY (XA_DRAWABLE),		ENTRY (XA_FONT),
	ENTRY (XA_INTEGER),			ENTRY (XA_PIXMAP),		ENTRY (XA_POINT),
	ENTRY (XA_RECTANGLE),		ENTRY (XA_RESOURCE_MANAGER),
	ENTRY (XA_RGB_COLOR_MAP),	ENTRY (XA_RGB_BEST_MAP),ENTRY (XA_RGB_BLUE_MAP),
	ENTRY (XA_RGB_DEFAULT_MAP),ENTRY (XA_RGB_GRAY_MAP),ENTRY (XA_RGB_GREEN_MAP),
	ENTRY (XA_RGB_RED_MAP),		ENTRY (XA_STRING),		ENTRY (XA_VISUALID),
	ENTRY (XA_WINDOW),			ENTRY (XA_WM_COMMAND),	ENTRY (XA_WM_HINTS),
	ENTRY (XA_WM_CLIENT_MACHINE),								ENTRY (XA_WM_ICON_NAME),
	ENTRY (XA_WM_ICON_SIZE),	ENTRY (XA_WM_NAME),		ENTRY (XA_WM_NORMAL_HINTS),
	ENTRY (XA_WM_SIZE_HINTS),	ENTRY (XA_WM_ZOOM_HINTS),	ENTRY (XA_MIN_SPACE),
	ENTRY (XA_NORM_SPACE),		ENTRY (XA_MAX_SPACE),	ENTRY (XA_END_SPACE),
	ENTRY (XA_SUPERSCRIPT_X),	ENTRY (XA_SUPERSCRIPT_Y),	ENTRY (XA_SUBSCRIPT_X),
	ENTRY (XA_SUBSCRIPT_Y),		ENTRY (XA_UNDERLINE_POSITION),
	ENTRY (XA_UNDERLINE_THICKNESS),	ENTRY (XA_STRIKEOUT_ASCENT),
	ENTRY (XA_STRIKEOUT_DESCENT),	ENTRY (XA_ITALIC_ANGLE),ENTRY (XA_X_HEIGHT),
	ENTRY (XA_QUAD_WIDTH),		ENTRY (XA_WEIGHT),		ENTRY (XA_POINT_SIZE),
	ENTRY (XA_RESOLUTION),		ENTRY (XA_COPYRIGHT),	ENTRY (XA_NOTICE),
	ENTRY (XA_FONT_NAME),		ENTRY (XA_FAMILY_NAME),	ENTRY (XA_FULL_NAME),
	ENTRY (XA_CAP_HEIGHT),		ENTRY (XA_WM_CLASS),	ENTRY (XA_WM_TRANSIENT_FOR),
	//--- Extra Atom Definitions
	ENTRY (WM_PROTOCOLS),		ENTRY (WM_DELETE_WINDOW),
};
CARD32 ATOM_Count = LAST_PREDEF_ATOM;

// list of alphabetial sorted Atoms by name
static short _ATOM_Order[] = {
	WM_DELETE_WINDOW, WM_PROTOCOLS,        XA_ARC,           XA_ATOM,
	XA_BITMAP,        XA_CAP_HEIGHT,       XA_CARDINAL,      XA_COLORMAP,
	XA_COPYRIGHT,     XA_CURSOR,           XA_CUT_BUFFER0,   XA_CUT_BUFFER1,
	XA_CUT_BUFFER2,   XA_CUT_BUFFER3,      XA_CUT_BUFFER4,   XA_CUT_BUFFER5,
	XA_CUT_BUFFER6,   XA_CUT_BUFFER7,      XA_DRAWABLE,      XA_END_SPACE,
	XA_FAMILY_NAME,   XA_FONT,             XA_FONT_NAME,     XA_FULL_NAME,
	XA_INTEGER,       XA_ITALIC_ANGLE,     XA_MAX_SPACE,     XA_MIN_SPACE,
	XA_NORM_SPACE,    XA_NOTICE,           XA_PIXMAP,        XA_POINT,
	XA_POINT_SIZE,    XA_PRIMARY,          XA_QUAD_WIDTH,    XA_RECTANGLE,
	XA_RESOLUTION,    XA_RESOURCE_MANAGER, XA_RGB_BEST_MAP,  XA_RGB_BLUE_MAP,
	XA_RGB_COLOR_MAP, XA_RGB_DEFAULT_MAP,  XA_RGB_GRAY_MAP,  XA_RGB_GREEN_MAP,
	XA_RGB_RED_MAP,   XA_SECONDARY,        XA_STRIKEOUT_ASCENT,
	XA_STRIKEOUT_DESCENT,                  XA_STRING,        XA_SUBSCRIPT_X,
	XA_SUBSCRIPT_Y,   XA_SUPERSCRIPT_X,    XA_SUPERSCRIPT_Y,
	XA_UNDERLINE_POSITION,                 XA_UNDERLINE_THICKNESS,
	XA_VISUALID,      XA_WEIGHT,           XA_WINDOW,        XA_WM_CLASS,
	XA_WM_CLIENT_MACHINE,                  XA_WM_COMMAND,    XA_WM_HINTS,
	XA_WM_ICON_NAME,  XA_WM_ICON_SIZE,     XA_WM_NAME,       XA_WM_NORMAL_HINTS,
	XA_WM_SIZE_HINTS, XA_WM_TRANSIENT_FOR, XA_WM_ZOOM_HINTS, XA_X_HEIGHT
};

static ATOM * _ATOM_Sort[MAX_ATOM -1];


//==============================================================================
void
AtomInit (BOOL initNreset)
{
	int i;
	
	// first call from Server initialization, fill in the name length for each
	// predefined atom
	//
	if (initNreset) {
		if (numberof(_ATOM_Order) != LAST_PREDEF_ATOM) {
			printf ("pFATALq Internal failure in AtomInit():\n"
			        "      order list = %li, count = %li\n",
			        numberof(_ATOM_Order), LAST_PREDEF_ATOM);
			exit(1);
		}
		for (i = 1; i <= LAST_PREDEF_ATOM; ++i) {
			ATOM_Table[i]->Id     = i;
			ATOM_Table[i]->Length = strlen (ATOM_Table[i]->Name);
		}
	
	// later calls from server reset, delete all non-predefined atoms
	//
	} else {
		for (i = LAST_PREDEF_ATOM +1; i <= ATOM_Count; ++i) {
			free (ATOM_Table[i]);
		}
	#	ifdef TRACE
		if ((i = ATOM_Count - LAST_PREDEF_ATOM) > 0) {
			printf ("  remove %i Atom%s.\n", i, (i == 1 ? "" : "s"));
		}
	#	endif TRACE
	}
	
	// always clear all empty table entries
	//
	memset (ATOM_Table + LAST_PREDEF_ATOM +1, 0,
	         (MAX_ATOM - LAST_PREDEF_ATOM) * sizeof(ATOM*));
	ATOM_Count = LAST_PREDEF_ATOM;
	
	for (i = 0; i < LAST_PREDEF_ATOM; ++i) {
		_ATOM_Sort[i] = ATOM_Table[_ATOM_Order[i]];
	}
}


//==============================================================================
Atom
AtomGet (const char * name, size_t len, BOOL onlyIfExists)
{
	short  beg = 0;
	short  end = ATOM_Count -1;
	short  num = 0, dir = 0;
	ATOM * atom;
	
	while (end >= beg) {
		num = (end + beg) /2;
		dir = strncmp (name, _ATOM_Sort[num]->Name, len);
		if (!dir) dir = len  - _ATOM_Sort[num]->Length;
		if      (dir > 0) beg  = num +1;
		else if (dir < 0) end  = num -1;
		else              return _ATOM_Sort[num]->Id;
	}
	if (dir > 0) num++;
	
	if (onlyIfExists  ||  ATOM_Count >= MAX_ATOM
	    || !(atom = malloc (sizeof(ATOM) + len))) return None;
	
	((char*)memcpy (atom->Name, name, len))[len] = '\0';
	atom->Id       = ++ATOM_Count;
	atom->Length   = len;
	atom->SelOwner = NULL;
	atom->SelWind  = NULL;
	atom->SelTime  = 0uL;
	
	if (num < ATOM_Count) {
		memmove (&_ATOM_Sort[num+1], &_ATOM_Sort[num], (ATOM_Count - num) *4);
	}
	_ATOM_Sort[num] = ATOM_Table[ATOM_Count] = atom;
	
	return ATOM_Count;
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_InternAtom (CLIENT * clnt, xInternAtomReq * q)
{
	// Returns the atom for a given name
	//
	// BOOL   onlyIfExists:      TRUE: a nonexisting atom won't be created
	// CARD16 nbytes:            number of bytes in string
	char * name = (char*)(q +1); // name to be found
	//
	// Reply:
	// Atom atom:   Atom id or 'None' if not existing and onlyIfExists = TRUE
	//...........................................................................
	
	if (!q->nbytes) {
		Bad(Value, 0, InternAtom, "():\n          zero length string.");
	
	} else { //..................................................................
		
	#	ifdef TRACE
		CARD32 cnt  = ATOM_Count;
	#	endif
		Atom   atom = AtomGet (name, q->nbytes, q->onlyIfExists);
		
		if (atom == None  &&  !q->onlyIfExists) {
			Bad(Alloc,, InternAtom, "('%.*s'):\n"
			            "          %s.", q->nbytes, name,
			            (ATOM_Count >= MAX_ATOM
			             ? "maximum number reached" : "memory exhausted"));
		
		} else {
			ClntReplyPtr (InternAtom, r,);
			
			DEBUG (InternAtom, "('%.*s') -> %lu%s",
			       q->nbytes, name, atom, (cnt < ATOM_Count < 0 ? "+" : ""));
			
			r->atom = atom;
			ClntReply (InternAtom,, "a");
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_GetAtomName (CLIENT * clnt, xGetAtomNameReq * q)
{
	// Returns the name for a given atom
	//
	// CARD32 id: requested atom
	//
	// Reply:
	// CARD16 nameLength: # of characters in name
	// (char*)(r +1):     name
	//...........................................................................
	
	if (!AtomValid(q->id)) {
		Bad(Atom, q->id, GetAtomName,);
	
	} else { //..................................................................
		
		size_t len = ATOM_Table[q->id]->Length;
		ClntReplyPtr (GetAtomName, r, len);
		
		DEBUG (GetAtomName, "(%lu) ->'%.*s'",
		       q->id, (int)len, ATOM_Table[q->id]->Name);
		
		r->nameLength = ATOM_Table[q->id]->Length;
		memcpy (r +1, ATOM_Table[q->id]->Name, len);
		ClntReply (GetAtomName, len, ".");
	}
}
