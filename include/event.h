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
#include "window.h"

#include <X11/X.h>

#ifndef __cplusplus
#  define c_new new
#  define c_class class
#  define c_delete delete
#endif


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

void EvntGraphExpose(p_CLIENT, p_DRAWABLE, CARD16 major, short len, const GRECT *rect);

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

void EvntClientMsg(p_CLIENT clnt, Window w_id, Atom type, CARD8 format, const void *data);
void EvntExpose(p_WINDOW wind, short len, const GRECT *rect);

/* ___________to_do_ */
/* KeymapNotify */


/* ___Inline_Funktions__________ */

void _Evnt_Struct(p_WINDOW wind, const xEvent *ev);
void _Evnt_Window(p_WINDOW wind, CARD32 event_mask, const xEvent *ev);
void _Evnt_Client(p_CLIENT, const xEvent *ev);


static inline void EvntKeyButMotion(p_CLIENT clnt, BYTE evnt_type, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	xEvent ev;

	ev.u.u.type = evnt_type;
	ev.u.u.detail = detail;
	ev.u.keyButtonPointer.time = MAIN_TimeStamp;
	ev.u.keyButtonPointer.root = ROOT_WINDOW;
	ev.u.keyButtonPointer.event = w_id;
	ev.u.keyButtonPointer.child = c_id;
	ev.u.keyButtonPointer.rootX = r_xy.p_x;
	ev.u.keyButtonPointer.rootY = r_xy.p_y;
	ev.u.keyButtonPointer.eventX = e_xy.p_x;
	ev.u.keyButtonPointer.eventY = e_xy.p_y;
	ev.u.keyButtonPointer.state = MAIN_KeyButMask;
	ev.u.keyButtonPointer.sameScreen = xTrue;
	ev.u.keyButtonPointer.pad1 = 0;
	_Evnt_Client(clnt, &ev);
}


static inline void EvntMotionNotify(p_WINDOW wind, Mask event_mask, Window w_id, Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	xEvent ev;
	
	ev.u.u.type = MotionNotify;
	ev.u.u.detail = detail;
	ev.u.keyButtonPointer.time = MAIN_TimeStamp;
	ev.u.keyButtonPointer.root = w_id;
	ev.u.keyButtonPointer.event = wind->Id;
	ev.u.keyButtonPointer.child = c_id;
	ev.u.keyButtonPointer.rootX = r_xy.p_x;
	ev.u.keyButtonPointer.rootY = r_xy.p_y;
	ev.u.keyButtonPointer.eventX = e_xy.p_x;
	ev.u.keyButtonPointer.eventY = e_xy.p_y;
	ev.u.keyButtonPointer.state = MAIN_KeyButMask;
	ev.u.keyButtonPointer.sameScreen = xTrue;
	ev.u.keyButtonPointer.pad1 = 0;
	_Evnt_Window(wind, event_mask, &ev);
}


static inline void EvntEnterNotify(p_WINDOW wind, Window r_id, Window c_id, PXY r_xy, PXY e_xy, BYTE mode, CARD8 focus, BYTE detail)
{
	xEvent ev;

	ev.u.u.type = EnterNotify;
	ev.u.u.detail = detail;
	ev.u.enterLeave.time = MAIN_TimeStamp;
	ev.u.enterLeave.root = r_id;
	ev.u.enterLeave.event = wind->Id;
	ev.u.enterLeave.child = c_id;
	ev.u.enterLeave.rootX = r_xy.p_x;
	ev.u.enterLeave.rootY = r_xy.p_y;
	ev.u.enterLeave.eventX = e_xy.p_x;
	ev.u.enterLeave.eventY = e_xy.p_y;
	ev.u.enterLeave.state = MAIN_KeyButMask;
	ev.u.enterLeave.mode = mode;
	ev.u.enterLeave.flags = focus;
	_Evnt_Window(wind, EnterWindowMask, &ev);
}


static inline void EvntLeaveNotify(p_WINDOW wind, Window r_id, Window c_id, PXY r_xy, PXY e_xy, BYTE mode, CARD8 focus, BYTE detail)
{
	xEvent ev;

	ev.u.u.type = LeaveNotify;
	ev.u.u.detail = detail;
	ev.u.enterLeave.time = MAIN_TimeStamp;
	ev.u.enterLeave.root = r_id;
	ev.u.enterLeave.event = wind->Id;
	ev.u.enterLeave.child = c_id;
	ev.u.enterLeave.rootX = r_xy.p_x;
	ev.u.enterLeave.rootY = r_xy.p_y;
	ev.u.enterLeave.eventX = e_xy.p_x;
	ev.u.enterLeave.eventY = e_xy.p_y;
	ev.u.enterLeave.state = MAIN_KeyButMask;
	ev.u.enterLeave.mode = mode;
	ev.u.enterLeave.flags = focus;
	_Evnt_Window(wind, LeaveWindowMask, &ev);
}


static inline void EvntFocusIn(p_WINDOW wind, BYTE mode, BYTE detail)
{
	xEvent ev;

	ev.u.u.type = FocusIn;
	ev.u.u.detail = detail;
	ev.u.focus.window = wind->Id;
	ev.u.focus.mode = mode;
	_Evnt_Window(wind, FocusChangeMask, &ev);
}


static inline void EvntFocusOut(p_WINDOW wind, BYTE mode, BYTE detail)
{
	xEvent ev;

	ev.u.u.type = FocusOut;
	ev.u.u.detail = detail;
	ev.u.focus.window = wind->Id;
	ev.u.focus.mode = mode;
	_Evnt_Window(wind, FocusChangeMask, &ev);
}


static inline void EvntNoExposure(p_CLIENT clnt, Drawable d_id, CARD8 major_opcode)
{
	xEvent ev;

	ev.u.u.type = NoExpose;
	ev.u.u.detail = 0;
	ev.u.noExposure.drawable = d_id;
	ev.u.noExposure.minorEvent = 0;
	ev.u.noExposure.majorEvent = major_opcode;
	_Evnt_Client(clnt, &ev);
}


static inline void EvntVisibilityNotify(p_WINDOW wind, BYTE mode)
{
	xEvent ev;
	
	ev.u.u.type = VisibilityNotify;
	ev.u.u.detail = 0;
	ev.u.visibility.window = wind->Id;
	ev.u.visibility.state = mode;
	_Evnt_Window(wind, VisibilityChangeMask, &ev);
}


static inline void EvntCreateNotify(p_WINDOW wind, Window w_id, const GRECT *rect, CARD16 border_width, BOOL override_redirect)
{
	xEvent ev;
	
	ev.u.u.type = CreateNotify;
	ev.u.u.detail = 0;
	ev.u.createNotify.parent = wind->Id;
	ev.u.createNotify.window = w_id;
	ev.u.createNotify.x = rect->g_x;
	ev.u.createNotify.y = rect->g_y;
	ev.u.createNotify.width = rect->g_w;
	ev.u.createNotify.height = rect->g_h;
	ev.u.createNotify.borderWidth = border_width;
	ev.u.createNotify.override = override_redirect;
	_Evnt_Window(wind, SubstructureNotifyMask, &ev);
}


static inline void EvntDestroyNotify(p_WINDOW wind, Window w_id)
{
	xEvent ev;
	
	ev.u.u.type = DestroyNotify;
	ev.u.u.detail = 0;
	ev.u.destroyNotify.event = wind->Id;
	ev.u.destroyNotify.window = w_id;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntUnmapNotify(p_WINDOW wind, Window w_id, BOOL from_configure)
{
	xEvent ev;
	
	ev.u.u.type = UnmapNotify;
	ev.u.u.detail = 0;
	ev.u.unmapNotify.event = wind->Id;
	ev.u.unmapNotify.window = w_id;
	ev.u.unmapNotify.fromConfigure = from_configure;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntMapNotify(p_WINDOW wind, Window w_id, BOOL override_redirect)
{
	xEvent ev;
	
	ev.u.u.type = MapNotify;
	ev.u.u.detail = 0;
	ev.u.mapNotify.event = wind->Id;
	ev.u.mapNotify.window = w_id;
	ev.u.mapNotify.override = override_redirect;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntMapRequest(p_WINDOW wind, Window w_id)
{
	xEvent ev;
	
	ev.u.u.type = MapRequest;
	ev.u.u.detail = 0;
	ev.u.mapRequest.parent = wind->Id;
	ev.u.mapRequest.window = w_id;
	_Evnt_Window(wind, SubstructureRedirectMask, &ev);
}


static inline void EvntReparentNotify(p_WINDOW wind, Mask event_mask, Window w_id, Window p_id, PXY w_xy, BOOL override_redirect)
{
	xEvent ev;
	
	ev.u.u.type = ReparentNotify;
	ev.u.u.detail = 0;
	ev.u.reparent.event = wind->Id;
	ev.u.reparent.window = w_id;
	ev.u.reparent.parent = p_id;
	ev.u.reparent.x = w_xy.p_x;
	ev.u.reparent.y = w_xy.p_y;
	ev.u.reparent.override = override_redirect;
	_Evnt_Window(wind, event_mask, &ev);
}


static inline void EvntConfigureNotify(p_WINDOW wind, Window w_id, Window a_id, const GRECT *rect, CARD16 border_width, BOOL override_redirect)
{
	xEvent ev;
	
	ev.u.u.type = ConfigureNotify;
	ev.u.u.detail = 0;
	ev.u.configureNotify.event = wind->Id;
	ev.u.configureNotify.window = w_id;
	ev.u.configureNotify.aboveSibling = a_id;
	ev.u.configureNotify.x = rect->g_x;
	ev.u.configureNotify.y = rect->g_y;
	ev.u.configureNotify.width = rect->g_w;
	ev.u.configureNotify.height = rect->g_h;
	ev.u.configureNotify.borderWidth = border_width;
	ev.u.configureNotify.override = override_redirect;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntConfigureRequest(p_WINDOW wind, Window w_id, Window s_id, const GRECT *rect, CARD16 border_width, CARD16 mask, BYTE detail)
{
	xEvent ev;
	
	ev.u.u.type = ConfigureRequest;
	ev.u.u.detail = detail;
	ev.u.configureRequest.parent = wind->Id;
	ev.u.configureRequest.window = w_id;
	ev.u.configureRequest.sibling = s_id;
	ev.u.configureRequest.x = rect->g_x;
	ev.u.configureRequest.y = rect->g_y;
	ev.u.configureRequest.width = rect->g_w;
	ev.u.configureRequest.height = rect->g_h;
	ev.u.configureRequest.borderWidth = border_width;
	ev.u.configureRequest.valueMask = mask;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntGravityNotify(p_WINDOW wind, Window w_id, PXY w_xy)
{
	xEvent ev;
	
	ev.u.u.type = GravityNotify;
	ev.u.u.detail = 0;
	ev.u.gravity.event = wind->Id;
	ev.u.gravity.window = w_id;
	ev.u.gravity.x = w_xy.p_x;
	ev.u.gravity.y = w_xy.p_y;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntResizeRequest(p_WINDOW wind, CARD16 width, CARD16 height)
{
	xEvent ev;
	
	ev.u.u.type = ResizeRequest;
	ev.u.u.detail = 0;
	ev.u.resizeRequest.window = wind->Id;
	ev.u.resizeRequest.width = width;
	ev.u.resizeRequest.height = height;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntCirculateNotify(p_WINDOW wind, Window w_id, BYTE place)
{
	xEvent ev;
	
	ev.u.u.type = CirculateNotify;
	ev.u.u.detail = 0;
	ev.u.circulate.event = wind->Id;
	ev.u.circulate.window = w_id;
	ev.u.circulate.parent = None;
	ev.u.circulate.place = place;
	_Evnt_Struct(wind, &ev);
}


static inline void EvntCirculateRequest(p_WINDOW wind, Window w_id, BYTE place)
{
	xEvent ev;
	
	ev.u.u.type = CirculateRequest;
	ev.u.u.detail = 0;
	ev.u.circulate.event = wind->Id;
	ev.u.circulate.window = w_id;
	ev.u.circulate.parent = None;
	ev.u.circulate.place = place;
	_Evnt_Window(wind, SubstructureRedirectMask, &ev);
}


static inline void EvntPropertyNotify(p_WINDOW wind, Atom type, BYTE mode)
{
	xEvent ev;
	
	ev.u.u.type = PropertyNotify;
	ev.u.u.detail = 0;
	ev.u.property.window = wind->Id;
	ev.u.property.atom = type;
	ev.u.property.time = MAIN_TimeStamp;
	ev.u.property.state = mode;
	_Evnt_Window(wind, PropertyChangeMask, &ev);
}


static inline void EvntSelectionClear(p_CLIENT clnt, Window w_id, Atom selection)
{
	xEvent ev;
	
	ev.u.u.type = SelectionClear;
	ev.u.u.detail = 0;
	ev.u.selectionClear.time = MAIN_TimeStamp;
	ev.u.selectionClear.window = w_id;
	ev.u.selectionClear.atom = selection;
	_Evnt_Client(clnt, &ev);
}


static inline void EvntSelectionRequest(p_CLIENT clnt, Time timestamp, Window w_id, Window requestor, Atom selection, Atom target, Atom property)
{
	xEvent ev;
	
	ev.u.u.type = SelectionRequest;
	ev.u.u.detail = 0;
	ev.u.selectionRequest.time = timestamp;
	ev.u.selectionRequest.owner = w_id;
	ev.u.selectionRequest.requestor = requestor;
	ev.u.selectionRequest.selection = selection;
	ev.u.selectionRequest.target = target;
	ev.u.selectionRequest.property = property;
	_Evnt_Client(clnt, &ev);
}


static inline void EvntSelectionNotify(p_CLIENT clnt, Time timestamp, Window w_id, Atom selection, Atom target, Atom property)
{
	xEvent ev;
	
	ev.u.u.type = SelectionNotify;
	ev.u.u.detail = 0;
	ev.u.selectionNotify.time = timestamp;
	ev.u.selectionNotify.requestor = w_id;
	ev.u.selectionNotify.selection = selection;
	ev.u.selectionNotify.target = target;
	ev.u.selectionNotify.property = property;
	_Evnt_Client(clnt, &ev);
}


static inline void EvntColormapNotify(WINDOW *wind, Colormap colormap, BOOL c_new, BYTE state)
{
	xEvent ev;
	
	ev.u.u.type = ColormapNotify;
	ev.u.u.detail = 0;
	ev.u.colormap.window = wind->Id;
	ev.u.colormap.colormap = colormap;
	ev.u.colormap.c_new = c_new;
	ev.u.colormap.state = state;
	_Evnt_Struct(wind, &ev);
}


void FT_Evnt_send_Unswapped(p_CLIENT, p_WINDOW, const xEvent *ev);
void FT_Evnt_send_Swapped(p_CLIENT, p_WINDOW, const xEvent *ev);

#endif /* __EVENT_H__ */
