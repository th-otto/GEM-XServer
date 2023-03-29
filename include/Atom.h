/*
 *==============================================================================
 *
 * Atom.h -- Declaration of struct 'ATOM' and related functions.
 *
 * An  Atom  is  a unique  ID  corresponding  to  a string name.  It is used to
 * identify  properties,  types and selections, without the need of sending the
 * much more time consumpting string names instead.
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-07 - Module released for beta state.
 * 2000-06-18 - Initial Version.
 *==============================================================================
 */
#ifndef __ATOM_H__
#define __ATOM_H__

#include <X11/X.h>

#define ATOM_BASE \
	Atom Id; \
	p_CLIENT SelOwner; \
	p_WINDOW SelWind; \
	CARD32 SelTime; \
	size_t Length

typedef struct s_ATOM
{
	ATOM_BASE;
#if __GNUC_PREREQ(3, 0)
	char Name[];
#else
	char Name[1];
#endif
} ATOM;
extern ATOM *ATOM_Table[];
extern CARD32 ATOM_Count;

void AtomInit(BOOL initNreset);
Atom AtomGet(const char *name, size_t len, BOOL onlyIfExists);

#define AtomValid(a) ((a) && (a) <= ATOM_Count)


/* --- Extra Atom Definitions (used bei window managers) */

#define WM_PROTOCOLS     (XA_LAST_PREDEFINED +1)
#define WM_DELETE_WINDOW (XA_LAST_PREDEFINED +2)
#define WM_TAKE_FOCUS    (XA_LAST_PREDEFINED +3)
#define WM_SAVE_YOURSELF (XA_LAST_PREDEFINED +4)

#define LAST_PREDEF_ATOM (XA_LAST_PREDEFINED +4)


#endif /* __ATOM_H__ */
