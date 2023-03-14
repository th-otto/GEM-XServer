//==============================================================================
//
// event.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-07-01 - Initial Version.
//==============================================================================
//
#ifndef __EVENT_H__
# define __EVENT_H__

#include "types.h"

#include <stdarg.h>

#include <X11/X.h>


#define AllEventMask   0x01FFFFFFuL

BOOL     EvntSet    (p_WINDOW wind, p_CLIENT clnt, CARD32 mask);
BOOL     EvntClr    (p_WINDOW wind, p_CLIENT clnt);
void     EvntDel    (p_WINDOW wind);
p_WINDOW EvntSearch (p_WINDOW wind, p_CLIENT clnt, CARD32 mask);
p_CLIENT EvntClient (p_WINDOW wind, CARD32 mask);

// KeyPress/KeyRelease, ButtonPress/ButtonRelease
BOOL EvntPropagate (p_WINDOW wind, CARD32 mask, BYTE event,
                    CARD32 c_id, PXY r_xy, PXY e_xy, BYTE detail);

// EnterNotify/LeaveNotify
void EvntPointer (p_WINDOW * stack, int anc, int top,
                  PXY e_xy, PXY r_xy, CARD32 r_id, CARD8 mode);

void EvntGraphExp (p_CLIENT , p_DRAWABLE ,
                   CARD16 major, short len, const struct s_GRECT * rect);

void EvntMappingNotify (CARD8 request, CARD8 first, CARD8 count);


#define _Void   static inline void
#define CLT    p_CLIENT clnt   // the client the event is sent to
#define WND    p_WINDOW wind   // the window the event is sent to
#define MSK    Mask     mask   // event mask
#define EVN    BYTE     evnt   // event
#define WID    Window   w_id   // window id the event is about
#define AID    Window   a_id   // above-sibling id
#define CID    Window   c_id   // child id
#define PID    Window   p_id   // parent id
#define REQ    Window   rqst   // requestor id
#define DID    Drawable d_id   // drawable id
#define TIM    Time     tmst   // time stamp
#define DTL    char     detl   // byte detail
#define TYP    Atom     type   // 
#define SEL    Atom     slct   // selection
#define PRP    Atom     prop   // property
#define TRG    Atom     trgt   // target
#define FRM    CARD8    frmt   // 
#define RCT    GRECT  * rect   // 
#define WXY    PXY      w_xy   // window x/y
#define RXY    PXY      r_xy   // root x/y
#define EXY    PXY      e_xy   // event x/y
#define BRD    CARD16   brdr   // border-width
#define OVR    BOOL     ovrd   // override-redirect
#define CFG    BOOL     cnfg   // from-configure
#define MOD    BYTE     mode   // place/state
#define FOC    CARD8    focs   // 
#define OPC    CARD16   opcd   // major-opcode

_Void EvntCirculateNotify (WND,      WID, DTL);
_Void EvntCirculateRequest(WND,      WID, DTL);
 void EvntClientMsg       (CLT,      WID, TYP, FRM, void *);
_Void EvntConfigureNotify (WND,      WID, AID, RCT, BRD, OVR);
_Void EvntCreateNotify    (WND,      WID,      RCT, BRD, OVR);
_Void EvntDestroyNotify   (WND,      WID);
_Void EvntEnterNotify     (WND,      WID, CID, RXY, EXY, MOD, FOC, DTL);
 void EvntExpose          (WND,      short len, const struct s_GRECT * rect);
_Void EvntFocusIn         (WND,                          MOD,      DTL);
_Void EvntFocusOut        (WND,                          MOD,      DTL);
_Void EvntGravityNotify   (WND,      WID, WXY);
_Void EvntKeyButMotion    (CLT, EVN, WID, CID, RXY, EXY, DTL);
_Void EvntLeaveNotify     (WND,      WID, CID, RXY, EXY, MOD, FOC, DTL);
_Void EvntMapNotify       (WND,      WID,                OVR);
_Void EvntMapRequest      (WND,      WID);
_Void EvntMotionNotify    (WND, MSK, WID, CID, RXY, EXY);
_Void EvntNoExposure      (CLT,      DID, OPC);
_Void EvntPropertyNotify  (WND,           TYP,           MOD);
_Void EvntReparentNotify  (WND, MSK, WID, PID, WXY,      OVR);
_Void EvntSelectionClear  (CLT,      WID,      SEL);
_Void EvntSelectionNotify (CLT, TIM, WID,      SEL, TRG, PRP);
_Void EvntSelectionRequest(CLT, TIM, WID, REQ, SEL, TRG, PRP);
_Void EvntUnmapNotify     (WND,      WID, CFG);
_Void EvntVisibilityNotify(WND,      MOD);

//___________to_do_
//KeymapNotify
//ConfigureRequest
//ResizeRequest
//ColormapNotify


//___Inline_Funktions__________

_Void EvntCirculateNotify (WND, WID, DTL) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, CirculateNotify, w_id, detl);
}
_Void EvntCirculateRequest(WND, WID, DTL) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, SubstructureRedirectMask, CirculateRequest, w_id, detl);
}
_Void EvntConfigureNotify (WND, WID, AID, RCT, BRD, OVR) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, ConfigureNotify, w_id, a_id, rect, brdr, ovrd);
}
_Void EvntCreateNotify (WND, WID, RCT, BRD, OVR) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16, ...);
	_Evnt_Window (wind, SubstructureNotifyMask, CreateNotify,
	              w_id, rect, brdr, ovrd);
}
_Void EvntDestroyNotify (WND, WID) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, DestroyNotify, w_id);
}
_Void EvntEnterNotify (WND, WID, CID, RXY, EXY, MOD, FOC, DTL) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, EnterWindowMask, EnterNotify,
	              w_id, c_id, r_xy, e_xy, mode, focs, detl);
}
_Void EvntFocusIn (WND, MOD, DTL) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, FocusChangeMask, FocusIn, mode, detl);
}
_Void EvntFocusOut (WND, MOD, DTL) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, FocusChangeMask, FocusOut, mode, detl);
}
_Void EvntGravityNotify (WND, WID, WXY) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, MapNotify, w_id, w_xy);
}
_Void EvntKeyButMotion (CLT, EVN, WID, CID, RXY, EXY, DTL) {
	extern void _Evnt_Client (p_CLIENT , CARD16 , ...);
	_Evnt_Client (clnt, evnt, w_id, c_id, r_xy, e_xy, detl);
}
_Void EvntLeaveNotify (WND, WID, CID, RXY, EXY, MOD, FOC, DTL) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, LeaveWindowMask, LeaveNotify,
	         w_id, c_id, r_xy, e_xy, mode, focs, detl);
}
_Void EvntMapNotify (WND, WID, OVR) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, MapNotify, w_id, ovrd);
}
_Void EvntMapRequest (WND, WID) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, SubstructureRedirectMask, MapRequest, w_id);
}
_Void EvntMotionNotify (WND, MSK, WID, CID, RXY, EXY) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, mask, MotionNotify, w_id, c_id, r_xy, e_xy);
}
_Void EvntNoExposure (CLT, DID, OPC) {
	extern void _Evnt_Client (p_CLIENT , CARD16 , ...);
	_Evnt_Client (clnt, NoExpose, d_id, (CARD16)0, opcd);
}
_Void EvntPropertyNotify (WND, TYP, MOD) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, PropertyChangeMask, PropertyNotify, type, mode);
}
_Void EvntReparentNotify (WND, MSK, WID, PID, WXY, OVR) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, mask, ReparentNotify, w_id, p_id, w_xy, ovrd);
}
_Void EvntSelectionClear  (CLT, WID, SEL) {
	extern void _Evnt_Client (p_CLIENT , CARD16 , ...);
	_Evnt_Client (clnt, SelectionClear, w_id, slct);
}
_Void EvntSelectionNotify (CLT, TIM, WID, SEL, TRG, PRP) {
	extern void _Evnt_Client (p_CLIENT , CARD16 , ...);
	_Evnt_Client (clnt, SelectionNotify, tmst, w_id, slct, trgt, prop);
}
_Void EvntSelectionRequest (CLT, TIM, WID, REQ, SEL, TRG, PRP) {
	extern void _Evnt_Client (p_CLIENT , CARD16 , ...);
	_Evnt_Client (clnt, SelectionRequest, tmst, w_id, rqst, slct, trgt, prop);
}
_Void EvntUnmapNotify (WND,  WID, CFG) {
	extern void _Evnt_Struct (p_WINDOW , CARD16 , ...);
	_Evnt_Struct (wind, UnmapNotify, w_id, cnfg);
}
_Void EvntVisibilityNotify (WND, MOD) {
	extern void _Evnt_Window (p_WINDOW , CARD32 , CARD16 , ...);
	_Evnt_Window (wind, VisibilityChangeMask, VisibilityNotify, mode);
}

#undef _Void
#undef CLT
#undef WND
#undef MSK
#undef EVN
#undef WID
#undef AID
#undef CID
#undef PID
#undef REQ
#undef DID
#undef TIM
#undef DTL
#undef TYP
#undef SEL
#undef PRP
#undef TRG
#undef FRM
#undef RCT
#undef WXY
#undef RXY
#undef EXY
#undef BRD
#undef OVR
#undef CFG
#undef MOD
#undef FOC
#undef OPC


void FT_Evnt_send_MSB (p_CLIENT , p_WINDOW , CARD16 evnt, va_list);
void FT_Evnt_send_LSB (p_CLIENT , p_WINDOW , CARD16 evnt, va_list);

#endif __EVENT_H__
