/*
 *==============================================================================
 *
 * event.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-07-01 - Initial Version.
 *==============================================================================
 */
#ifndef __EVENT_H__
#define __EVENT_H__

#include "types.h"

#include <stdarg.h>

#include <X11/X.h>


#define AllEventMask   0x01FFFFFFuL

BOOL EvntSet(p_WINDOW wind, p_CLIENT clnt, CARD32 mask);
BOOL EvntClr(p_WINDOW wind, p_CLIENT clnt);
void EvntDel(p_WINDOW wind);
p_WINDOW EvntSearch(p_WINDOW wind, p_CLIENT clnt, CARD32 mask);
p_CLIENT EvntClient(p_WINDOW wind, CARD32 mask);

/* KeyPress/KeyRelease, ButtonPress/ButtonRelease */
BOOL EvntPropagate(p_WINDOW wind, CARD32 mask, BYTE event, CARD32 c_id, PXY r_xy, PXY e_xy, BYTE detail);

/* EnterNotify/LeaveNotify */
void EvntPointer(p_WINDOW *stack, int anc, int top, PXY e_xy, PXY r_xy, CARD32 r_id, CARD8 mode);

void EvntGraphExp(p_CLIENT, p_DRAWABLE, CARD16 major, short len, const GRECT *rect);

void EvntMappingNotify(CARD8 request, CARD8 first, CARD8 count);


/*
 * p_CLIENT clnt:            the client the event is sent to
 * p_WINDOW wind:            the window the event is sent to
 * Mask event_mask:          event mask
 * BYTE evnt:			     event type
 * Window w_id:              window id the event is about
 * Window a_id:              above-sibling id
 * Window c_id:              child id
 * Window p_id:              parent id
 * Window rqst:			     requestor id
 * Drawable d_id:			 drawable id
 * Time timestamp:           time stamp
 * BYTE detl:			     byte detail
 * Atom type:                data type
 * Atom slct:                selection
 * Atom property:	         property
 * Atom target:	         	 target
 * GRECT *rect:			     rect
 * PXY w_xy:			     window x/y
 * PXY r_xy:			     root x/y
 * PXY e_xy:			     event x/y
 * CARD16 border_width:		 border-width
 * BOOL override_redirect:   override-redirect
 * BOOL cnfg:                from-configure
 * BYTE mode:			     place/state
 * CARD8 focus:              focus
 */

void EvntClientMsg(p_CLIENT clnt, Window w_id, Atom type, CARD8 format, void *data);
void EvntExpose(p_WINDOW wind, short len, const GRECT *rect);

/* ___________to_do_ */
/* KeymapNotify */
/* ConfigureRequest */
/* ResizeRequest */
/* ColormapNotify */


/* ___Inline_Funktions__________ */

void _Evnt_Struct(p_WINDOW wind, CARD16 event_type, ...);
void _Evnt_Window(p_WINDOW wind, CARD32 event_mask, CARD16 event_type, ...);
void _Evnt_Client(p_CLIENT, CARD16, ...);

static inline void EvntCirculateNotify(p_WINDOW wind, Window w_id, BYTE detail)
{
	_Evnt_Struct(wind, CirculateNotify, w_id, detail);
}

static inline void EvntCirculateRequest(p_WINDOW wind, Window w_id, BYTE detail)
{
	_Evnt_Window(wind, SubstructureRedirectMask, CirculateRequest, w_id, detail);
}

static inline void EvntConfigureNotify(p_WINDOW wind, Window w_id, Window a_id, const GRECT *rect, CARD16 border_width, BOOL override_redirect)
{
	_Evnt_Struct(wind, ConfigureNotify, w_id, a_id, rect, border_width, override_redirect);
}

static inline void EvntCreateNotify(p_WINDOW wind, Window w_id, const GRECT *rect, CARD16 border_width, BOOL override_redirect)
{
	_Evnt_Window(wind, SubstructureNotifyMask, CreateNotify, w_id, rect, border_width, override_redirect);
}

static inline void EvntDestroyNotify(p_WINDOW wind, Window w_id)
{
	_Evnt_Struct(wind, DestroyNotify, w_id);
}

static inline void EvntEnterNotify(p_WINDOW wind, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE mode, CARD8 focus, BYTE detail)
{
	_Evnt_Window(wind, EnterWindowMask, EnterNotify, w_id, c_id, r_xy, e_xy, mode, focus, detail);
}

static inline void EvntFocusIn(p_WINDOW wind, BYTE mode, BYTE detail)
{
	_Evnt_Window(wind, FocusChangeMask, FocusIn, mode, detail);
}

static inline void EvntFocusOut(p_WINDOW wind, BYTE mode, BYTE detail)
{
	_Evnt_Window(wind, FocusChangeMask, FocusOut, mode, detail);
}

static inline void EvntGravityNotify(p_WINDOW wind, Window w_id, PXY w_xy)
{
	_Evnt_Struct(wind, MapNotify, w_id, w_xy);
}

static inline void EvntKeyButMotion(p_CLIENT clnt, BYTE evnt_type, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	_Evnt_Client(clnt, evnt_type, w_id, c_id, r_xy, e_xy, detail);
}

static inline void EvntLeaveNotify(p_WINDOW wind, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE mode, CARD8 focus, BYTE detail)
{
	_Evnt_Window(wind, LeaveWindowMask, LeaveNotify, w_id, c_id, r_xy, e_xy, mode, focus, detail);
}

static inline void EvntMapNotify(p_WINDOW wind, Window w_id, BOOL override_redirect)
{
	_Evnt_Struct(wind, MapNotify, w_id, override_redirect);
}

static inline void EvntMapRequest(p_WINDOW wind, Window w_id)
{
	_Evnt_Window(wind, SubstructureRedirectMask, MapRequest, w_id);
}

static inline void EvntMotionNotify(p_WINDOW wind, Mask event_mask, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	_Evnt_Window(wind, event_mask, MotionNotify, w_id, c_id, r_xy, e_xy, detail);
}

static inline void EvntNoExposure(p_CLIENT clnt, Drawable d_id, CARD16 major_opcode)
{
	_Evnt_Client(clnt, NoExpose, d_id, (CARD16) 0, major_opcode);
}

static inline void EvntPropertyNotify(p_WINDOW wind, Atom type, BYTE mode)
{
	_Evnt_Window(wind, PropertyChangeMask, PropertyNotify, type, mode);
}

static inline void EvntReparentNotify(p_WINDOW wind, Mask event_mask, Window w_id, Window p_id, PXY w_xy, BOOL override_redirect)
{
	_Evnt_Window(wind, event_mask, ReparentNotify, w_id, p_id, w_xy, override_redirect);
}

static inline void EvntSelectionClear(p_CLIENT clnt, Window w_id, Atom selection)
{
	_Evnt_Client(clnt, SelectionClear, w_id, selection);
}

static inline void EvntSelectionNotify(p_CLIENT clnt, Time timestamp, Window w_id, Atom selection, Atom target, Atom property)
{
	_Evnt_Client(clnt, SelectionNotify, timestamp, w_id, selection, target, property);
}

static inline void EvntSelectionRequest(p_CLIENT clnt, Time timestamp, Window w_id, Window rqst, Atom selection, Atom target, Atom property)
{
	_Evnt_Client(clnt, SelectionRequest, timestamp, w_id, rqst, selection, target, property);
}

static inline void EvntUnmapNotify(p_WINDOW wind, Window w_id, BOOL cnfg)
{
	_Evnt_Struct(wind, UnmapNotify, w_id, cnfg);
}

static inline void EvntVisibilityNotify(p_WINDOW wind, BYTE mode)
{
	_Evnt_Window(wind, VisibilityChangeMask, VisibilityNotify, mode);
}

void FT_Evnt_send_MSB(p_CLIENT, p_WINDOW, CARD16 evnt, va_list);
void FT_Evnt_send_LSB(p_CLIENT, p_WINDOW, CARD16 evnt, va_list);

#endif /* __EVENT_H__ */
