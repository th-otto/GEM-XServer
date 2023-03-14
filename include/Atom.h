//==============================================================================
//
// Atom.h -- Declaration of struct 'ATOM' and related functions.
//
// An  Atom  is  a unique  ID  corresponding  to  a string name.  It is used to
// identify  properties,  types and selections, without the need of sending the
// much more time consumpting string names instead.
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-06-18 - Initial Version.
//==============================================================================
//
#ifndef __ATOM_H__
#	define __ATOM_H__

#include <X11/X.h>

#if defined(_Atom_) || defined(_selection_)
# define CONST
#else
# define CONST   const
#endif


typedef struct s_ATOM {
	Atom     Id;
	p_CLIENT SelOwner;
	p_WINDOW SelWind;
	CARD32   SelTime;
	size_t   Length;
	char     Name[1];
} ATOM;
extern CONST ATOM * CONST ATOM_Table[];
extern CONST CARD32       ATOM_Count;

void    AtomInit (BOOL initNreset);
Atom    AtomGet  (const char * name, size_t len, BOOL onlyIfExists);

#define AtomValid(a) ((a) && (a) <= ATOM_Count)


//--- Extra Atom Definitions (used bei window managers)

#define WM_PROTOCOLS     (XA_LAST_PREDEFINED +1)
#define WM_DELETE_WINDOW (XA_LAST_PREDEFINED +2)

#define LAST_PREDEF_ATOM (XA_LAST_PREDEFINED +2)


#undef CONST

#endif __ATOM_H__
