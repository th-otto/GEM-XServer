/*
 *==============================================================================
 *
 * wmgr.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-09-01 - Initial Version.
 *==============================================================================
 */
#ifndef __WMGR_H__
#define __WMGR_H__

void WmgrIntro(BOOL onNoff);
BOOL WmgrInit(BOOL initNreset);
void WmgrActivate(BOOL onNoff);
void WmgrExit(void);

void WmgrClntInsert(p_CLIENT client);
void WmgrClntUpdate(p_CLIENT client, const char *text);
void WmgrClntRemove(p_CLIENT client);

void WmgrCalcBorder(GRECT * curr, p_WINDOW wind);
BOOL WmgrWindHandle(p_WINDOW wind);
BOOL WmgrWindMap(p_WINDOW wind, GRECT *);
BOOL WmgrWindUnmap(p_WINDOW wind, BOOL destroy);
void WmgrWindName(p_WINDOW wind, const char *name, BOOL iconNmapped);
void WmgrWindIcon(p_WINDOW wind);
void WmgrCursor(p_WINDOW wind, PXY *rel_pos);
void WmgrCursorOff(p_CURSOR new_crsr);

void WmgrSetDesktop(BOOL onNoff);

void WmgrDrawDeco(p_WINDOW, p_PRECT work, p_PRECT area, p_PRECT sect, int num);
void WmgrDrawIcon(p_WINDOW, GRECT *clip);

void WmgrSetFocus(short focus);

BOOL WmgrMenu(short title, short entry, short meta);
BOOL WmgrMessage(short *msg);
BOOL WmgrButton(p_WINDOW);
void WmgrKeybd(short chng_meta);

short WmgrLaunch(const char *prg, int argc, const char *argv[]);


extern BOOL WMGR_ExitFlag;
extern BOOL WMGR_Active;
extern CARD16 WMGR_Cursor;
extern short WMGR_Decor;

extern CARD16 WMGR_OpenCounter;
extern short WMGR_Focus;
extern short WMGR_FocusHolder;


#endif /* __WMGR_H__ */
