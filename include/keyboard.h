//==============================================================================
//
// keyboard.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-11-15 - Initial Version.
//==============================================================================
//
#ifndef __KEYBOARD_H__
# define __KEYBOARD_H__

# ifdef _keyboard_
#	define CONST
# else
#	define CONST const
# endif


void  KybdInit (void);

short KybdEvent (CARD16 scan, CARD8 meta);

extern const CARD8   KYBD_CodeMin;
extern CONST CARD8   KYBD_CodeMax;
extern CONST CARD8   KYBD_PrvMeta;
extern CONST CARD8 * KYBD_Pending;
extern CONST CARD16  KYBD_Repeat;


# undef CONST

#endif __KEYBOARD_H__
