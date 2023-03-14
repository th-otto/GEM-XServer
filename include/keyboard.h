/*
 *==============================================================================
 *
 * keyboard.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-11-15 - Initial Version.
 *==============================================================================
 */
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

void KybdInit(void);

short KybdEvent(CARD16 scan, CARD8 meta);

extern const CARD8 KYBD_CodeMin;
extern CARD8 KYBD_CodeMax;
extern CARD8 KYBD_PrvMeta;
extern CARD8 *KYBD_Pending;
extern CARD16 KYBD_Repeat;


#endif /* __KEYBOARD_H__ */
