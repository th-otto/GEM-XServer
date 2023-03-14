//==============================================================================
//
// event.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-07-01 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "tools.h"
#include "clnt.h"
#include "event.h"
#include "window.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/Xproto.h>


void _Evnt_Window (p_WINDOW wind, CARD32 mask, CARD16 evnt, ...);
void _Evnt_Struct (p_WINDOW wind, CARD16 evnt, ...);
void _Evnt_Client (p_CLIENT clnt, CARD16 evnt, ...);


//------------------------------------------------------------------------------
#ifdef TRACE
static const char *
_M2STR (CARD8 mode)
{
	static char inval[16];
	
	switch (mode) {
		case NotifyAncestor:           return "ancestor";
		case NotifyVirtual:            return "virtual";
		case NotifyInferior:           return "inferior";
		case NotifyNonlinear:          return "nonlinear";
		case NotifyNonlinearVirtual:   return "nonl.virt.";
		case NotifyPointer:            return "pointer";
		case NotifyPointerRoot:        return "ptr.root";
		case NotifyDetailNone:         return "no_detail";
	}
	sprintf (inval, "<%u>", mode);
	
	return inval;
}

# define TRACEP(w,el,m,p,id,r) printf ("W:%X " el " %s [%i,%i] W:%lX [%i,%i] \n", \
                                       w->Id, _M2STR(m), p.x, p.y, id, r.x, r.y)

#else
# define TRACEP(w,el,m,p,id,r)
#endif


//==============================================================================
BOOL
EvntSet (WINDOW * wind, CLIENT * clnt, CARD32 mask)
{
	CARD16     num;
	WINDEVNT * src, * dst = NULL;
	
	if (wind->u.List.AllMasks < 0) {
		int i = num = wind->u.List.p->Length;
		dst   = src = wind->u.List.p->Event;
		while (i-- && dst->Client != clnt) dst++;
		if (i < 0) dst       =  NULL;
		else       dst->Mask |= mask;
	
	} else {
		src = &wind->u.Event;
		num = 1;
		
		if (src->Client == clnt) {
			dst = src;
		} else if (!src->Client) {
			dst = src;
			dst->Client = clnt;
			clnt->EventReffs++;
		}
	}
	
	if (!dst) {
		size_t                 len = sizeof(WINDEVNT) * num;
		typeof(wind->u.List.p) lst = malloc (sizeof(*lst) + len);
		if (!lst) return xFalse;
		
		lst->Length = num +1;
		memcpy (lst->Event, src, len);
		if (num > 1) free (wind->u.List.p);
		else         wind->u.List.AllMasks |= 0x80000000uL;
		wind->u.List.p         = lst;
		lst->Event[num].Mask   = mask;
		lst->Event[num].Client = clnt;
		clnt->EventReffs++;
	
	} else if (num > 1  && (~wind->u.List.AllMasks & mask)) {
		int i;
		wind->u.List.AllMasks = 0x80000000uL;
		for (i = 0, mask = 0uL; i < num; mask |= src[i++].Mask);
	}
	
	wind->u.List.AllMasks |= mask;
	
	return xTrue;
}

//==============================================================================
BOOL
EvntClr (WINDOW * wind, CLIENT * clnt)
{
	if (wind->u.List.AllMasks < 0) {
		CARD16     num  = wind->u.List.p->Length;
		WINDEVNT * ptr  = wind->u.List.p->Event;
		CARD32     mask = 0uL;
		while (ptr->Client != clnt) {
			mask |= ptr->Mask;
			ptr++;
			if (!--num) break;
		}
		if (num) {
			if (--wind->u.List.p->Length == 1) {
				CLIENT * left = ptr[num == 2 ? +1 : -1].Client;
				free (wind->u.List.p);
				wind->u.Event.Client = left;
			} else {
				mask |= 0x80000000uL;
				while (--num) {
					mask         |=
					ptr[0].Mask   = ptr[1].Mask;
					ptr[0].Client = ptr[1].Client;
					ptr++;
				}
				ptr[0].Mask   = 0uL;
				ptr[0].Client = NULL;
			}
			wind->u.List.AllMasks = mask;
			clnt->EventReffs--;
			return xTrue;
		}
	} else if (wind->u.List.AllMasks  &&  wind->u.Event.Client == clnt) {
		wind->u.Event.Mask   = 0uL;
		wind->u.Event.Client = NULL;
		clnt->EventReffs--;
		return xTrue;
	}
	return xFalse;
}

//==============================================================================
void
EvntDel (WINDOW * wind)
{
	CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
	
	while (num--) {
		if (lst->Mask) {
			lst->Client->EventReffs--;
		}
		lst++;
	}
	if (wind->u.List.AllMasks < 0) {
		free (wind->u.List.p);
	}
	wind->u.Event.Mask   = NoEventMask;
	wind->u.Event.Client = NULL;
}

//==============================================================================
WINDOW *
EvntSearch (WINDOW * wind, CLIENT * clnt, CARD32 mask)
{
	while (wind) {
		if (mask & wind->u.List.AllMasks) {
			CARD16     num = (wind->u.List.AllMasks < 0
			                  ? wind->u.List.p->Length : 1);
			WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
			while (num--) {
				if (lst->Client == clnt  &&  lst->Mask & mask) return wind;
				lst++;
			}
		} else if (!(mask &= wind->PropagateMask)) {
			break;
		}
		wind = wind->Parent;
	}
	return NULL;
}

//==============================================================================
CLIENT *
EvntClient (WINDOW * wind, CARD32 mask)
{
	CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
	
	while (num--) {
		if ((lst->Mask & mask) == mask) return lst->Client;
		lst++;
	}
	return NULL;
}


//------------------------------------------------------------------------------
static inline xEvent *
Evnt_Buffer (O_BUFF * buf, size_t need) {
	void   * r;
	size_t   n = buf->Done + buf->Left;
	if (n + need <= buf->Size) r = buf->Mem + n;
	else                       r = ClntOutBuffer (buf, need, 0, 0);
	return r;
}

//==============================================================================
void
EvntExpose (WINDOW * wind, short len, const struct s_GRECT * rect)
{
	CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
	size_t     spc = sizeof(xEvent) * len;
	xEvent   * evn;
	
	while (num--) {
		if ((lst->Mask & ExposureMask)
		    &&  (evn = Evnt_Buffer (&lst->Client->oBuf, spc))) {
			const GRECT * rct = rect;
			int           cnt = len;
			if (lst->Client->DoSwap) {
				CARD16 seq = Swap16(lst->Client->SeqNum);
				CARD32 wid = Swap32(wind->Id);
				while (cnt--) {
					evn->u.u.type           = Expose;
					evn->u.u.sequenceNumber = seq;
					evn->u.expose.window    = wid;
					evn->u.expose.count     = Swap16(cnt);
					SwapRCT ((GRECT*)&evn->u.expose.x, rct++);
					evn++;
				}
			} else {
				CARD16 seq = lst->Client->SeqNum;
				CARD32 wid = wind->Id;
				while (cnt--) {
					evn->u.u.type             = Expose;
					evn->u.u.sequenceNumber   = seq;
					evn->u.expose.window      = wid;
					evn->u.expose.count       = cnt;
					*(GRECT*)&evn->u.expose.x = *(rct++);
					evn++;
				}
			}
			lst->Client->oBuf.Left += spc;
			MAIN_FDSET_wr          |= lst->Client->FdSet;
		}
		lst++;
	}
}

//==============================================================================
void
EvntGraphExp (CLIENT * clnt, p_DRAWABLE draw,
              CARD16 major, short len, const struct s_GRECT * rect)
{
	size_t   spc = sizeof(xEvent) * len;
	xEvent * evn = Evnt_Buffer (&clnt->oBuf, spc);
	if (evn) {
		if (clnt->DoSwap) {
			CARD16 seq = Swap16(clnt->SeqNum);
			CARD32 did = Swap32(draw.p->Id);
			major      = Swap16(major);
			while (len--) {
				evn->u.u.type                      = GraphicsExpose;
				evn->u.u.sequenceNumber            = seq;
				evn->u.graphicsExposure.drawable   = did;
				evn->u.graphicsExposure.count      = Swap16(len);
				evn->u.graphicsExposure.minorEvent = 0;
				evn->u.graphicsExposure.majorEvent = major;
				SwapRCT ((GRECT*)&evn->u.graphicsExposure.x, rect++);
				evn++;
			}
		} else {
			CARD16 seq = clnt->SeqNum;
			CARD32 did = draw.p->Id;
			while (len--) {
				evn->u.u.type                       = GraphicsExpose;
				evn->u.u.sequenceNumber             = seq;
				evn->u.graphicsExposure.drawable    = did;
				evn->u.graphicsExposure.count       = len;
				evn->u.graphicsExposure.minorEvent  = 0;
				evn->u.graphicsExposure.majorEvent  = major;
				*(GRECT*)&evn->u.graphicsExposure.x = *(rect++);
				evn++;
			}
		}
		clnt->oBuf.Left += spc;
		MAIN_FDSET_wr   |= clnt->FdSet;
	}
}

//==============================================================================
void
EvntClientMsg (CLIENT * clnt, Window id, Atom type, BYTE format, void * data)
{
	xEvent * evn = Evnt_Buffer (&clnt->oBuf, sizeof(xEvent));
	if (evn) {
		evn->u.u.type   = ClientMessage;
		evn->u.u.detail = format;
		if (clnt->DoSwap) {
			evn->u.u.sequenceNumber       = Swap16(clnt->SeqNum);
			evn->u.clientMessage.window   = Swap32(id);
			evn->u.clientMessage.u.l.type = Swap32(type);
		} else {
			evn->u.u.sequenceNumber       = clnt->SeqNum;
			evn->u.clientMessage.window   = id;
			evn->u.clientMessage.u.l.type = type;
			format = 0;
		}
		if (format == 32) {
			CARD32 * src = data;
			CARD32 * dst = &evn->u.clientMessage.u.l.longs0;
			int      num = 5;
			do { *(dst++) = Swap32(*(src++)); } while (--num);
		} else if (format == 16) {
			CARD16 * src = data;
			CARD16 * dst = &evn->u.clientMessage.u.s.shorts0;
			int      num = 10;
			do { *(dst++) = Swap16(*(src++)); } while (--num);
		} else {
			memcpy (evn->u.clientMessage.u.b.bytes, data, 20);
		}
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr   |= clnt->FdSet;
	}
}

//==============================================================================
BOOL
EvntPropagate (WINDOW * wind, CARD32 mask, BYTE event,
               Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	CARD32 chld = (c_id == wind->Id ? None : c_id);
	BOOL   exec = xFalse;
	
	do {
		if (mask & wind->u.List.AllMasks) {
			_Evnt_Window (wind, mask, event, chld, r_xy, e_xy, detail);
			exec  = xTrue;
			mask &= !wind->u.List.AllMasks;
		}
		if (mask &= wind->PropagateMask) {
			e_xy.x += wind->Rect.x;
			e_xy.y += wind->Rect.y;
			if (c_id) chld = wind->Id;
		} else {
			break;
		}
	} while ((wind = wind->Parent));
	
	return exec;
}

//==============================================================================
void
EvntPointer (WINDOW ** stack, int anc, int top,
             PXY e_xy, PXY r_xy, CARD32 r_id, CARD8 mode)
{
	CARD8 detl, next, last;
	int   bot;
	
	/*--- generate events ---*/
	
	if (anc == 0) {
		detl = NotifyInferior;
		next = NotifyVirtual;
		last = NotifyAncestor;
	} else if (anc == top) {
		detl = NotifyAncestor;
		next = NotifyVirtual;
		last = NotifyInferior;
	} else {
		detl = NotifyNonlinear;
		next = NotifyNonlinearVirtual;
		last = NotifyNonlinear;
	}
	
	// notify enter events
	
	if (stack[0]) {
		TRACEP (stack[0], "Leave", detl, e_xy, r_id, r_xy);
		if (stack[0]->u.List.AllMasks & LeaveWindowMask) {
			EvntLeaveNotify (stack[0], r_id, None, r_xy, e_xy,
			                 mode, ELFlagSameScreen, detl);
		}
		if (anc > 0) {
			e_xy.x += stack[0]->Rect.x;
			e_xy.y += stack[0]->Rect.y;
		}
	}
	for (bot = 1; bot < anc; ++bot) {
		TRACEP (stack[bot], "Leave", next, e_xy, r_id, r_xy);
		if (stack[bot]->u.List.AllMasks & LeaveWindowMask) {
			EvntLeaveNotify (stack[bot], r_id, None, r_xy, e_xy,
			                 mode, ELFlagSameScreen, next);
		}
		e_xy.x += stack[bot]->Rect.x;
		e_xy.y += stack[bot]->Rect.y;
	}
	
	// notify leave events
	
	for (bot = anc +1; bot < top; ++bot) {
		e_xy.x -= stack[bot]->Rect.x;
		e_xy.y -= stack[bot]->Rect.y;
		TRACEP (stack[bot], "Enter", next, e_xy, r_id, r_xy);
		if (stack[bot]->u.List.AllMasks & EnterWindowMask) {
			EvntEnterNotify (stack[bot], r_id, None, r_xy, e_xy,
			                 mode, ELFlagSameScreen, next);
		}
	}
	if (stack[top]) {
		if (anc < top) {
			e_xy.x -= stack[top]->Rect.x;
			e_xy.y -= stack[top]->Rect.y;
		}
		TRACEP (stack[top], "Enter", last, e_xy, r_id, r_xy);
		if (stack[top]->u.List.AllMasks & EnterWindowMask) {
			EvntEnterNotify (stack[top], r_id, None, r_xy, e_xy,
			                 mode, ELFlagSameScreen, last);
		}
	}
}


//==============================================================================
void
EvntMappingNotify (CARD8 request, CARD8 first, CARD8 count)
{
	CLIENT * clnt = (CLIENT*)CLNT_Base;
	xEvent * evn;
	while (clnt) {
		if ((evn = Evnt_Buffer (&clnt->oBuf, sizeof(xEvent)))) {
			evn->u.u.type           = MappingNotify;
			evn->u.u.sequenceNumber = (clnt->DoSwap ? Swap16(clnt->SeqNum)
			                                        :        clnt->SeqNum);
			evn->u.mappingNotify.request      = request;
			evn->u.mappingNotify.firstKeyCode = first;
			evn->u.mappingNotify.count        = count;
			clnt->oBuf.Left += sizeof(xEvent);
			MAIN_FDSET_wr   |= clnt->FdSet;
		}
		clnt = clnt->Next;
	}
}


//------------------------------------------------------------------------------
static const char * _EVNT_Form[];

void
_Evnt_Window (WINDOW * wind, CARD32 mask, CARD16 evnt, ...)
{
	CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);

	if (evnt < 2 || evnt >= LASTEvent) {
		printf ("\33pERROR\33q invalid event value %u for W:%X.\n",
		        evnt, wind->Id);
		return;
	}
	if (!_EVNT_Form[evnt]) {
		printf ("\33pERROR\33q undefined event set at %u for W:%X.\n",
		        evnt, wind->Id);
		return;
	}
	if (mask & ~AllEventMask) {
		printf ("\33pWARNING\33q invalid event mask %lX for W:%X.\n",
		        mask, wind->Id);
		mask &= AllEventMask;
	}
	
	while (num--) {
		if (lst->Mask & mask) {
			va_list   vap;
			va_start (vap, evnt);
			lst->Client->Fnct->event (lst->Client, wind, evnt, vap);
			va_end (vap);
		}
		lst++;
	}
}

//------------------------------------------------------------------------------
void
_Evnt_Struct (WINDOW * wind, CARD16 evnt, ...)
{
	if (evnt < 2 || evnt >= LASTEvent) {
		printf ("\33pERROR\33q invalid event value %u for W:%X.\n",
		        evnt, wind->Id);
		return;
	}
	if (!_EVNT_Form[evnt]) {
		printf ("\33pERROR\33q undefined event set at %u for W:%X.\n",
		        evnt, wind->Id);
		return;
	}
	
	if (wind->u.List.AllMasks & StructureNotifyMask) {
		CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
		WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
		while (num--) {
			if (lst->Mask & StructureNotifyMask) {
				va_list   vap;
				va_start (vap, evnt);
				lst->Client->Fnct->event (lst->Client, wind, evnt, vap);
				va_end (vap);
			}
			lst++;
		}
	}
	if ((wind = wind->Parent) &&
	    (wind->u.List.AllMasks & SubstructureNotifyMask)) {
		CARD16     num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
		WINDEVNT * lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
		while (num--) {
			if (lst->Mask & SubstructureNotifyMask) {
				va_list   vap;
				va_start (vap, evnt);
				lst->Client->Fnct->event (lst->Client, wind, evnt, vap);
				va_end (vap);
			}
			lst++;
		}
	}
}

//------------------------------------------------------------------------------
void
_Evnt_Client (CLIENT * clnt, CARD16 evnt, ...)
{
	if (evnt < 2 || evnt >= LASTEvent) {
		printf ("\33pERROR\33q invalid event value %u.\n", evnt);
		
	} else if (!_EVNT_Form[evnt]) {
		printf ("\33pERROR\33q undefined event set at %u.\n", evnt);
		
	} else {
		va_list   vap;
		va_start (vap, evnt);
		clnt->Fnct->event (clnt, NULL, evnt, vap);
		va_end (vap);
	}
}


//==============================================================================
// w   - Window
// d   - Drawable
// a   - Atom
// l   - CARD32 ('long')
// s   - CARD16 ('short')
// b/c - BOOL / CARD8 ('char')
// p   - PXY coordinates (not pointer of!)
// r   - GRECT*
// D   - detail, CARD8
// S   - global TIMESTAMP
// M   - global SETofKEYBUTMASK
// W   - Id from parameter WINDOW*
// X   - RootWindow Id
// T/F - xTrue / xFalse, constant CARD8 (mainly for 'same-screen')
// *   - unused long, jump over

static const char * _EVNT_Form[LASTEvent] = {
	NULL, NULL,             // starts at #2
	"SXWwppMTD",  "SXWwppMTD",         // KeyPress,        KeyRelease
	"SXWwppMTD",  "SXWwppMTD",         // ButtonPress,     ButtonRelease
	"SwWwppMT",                        // MotionNotify,
	"SwWwppMccD", "SwWwppMccD",        // EnterNotify,     LeaveNotify
	"WcD",        "WcD",               // FocusIn,         FocusOut
	"",                                // KeymapNotify
	"Wrs",        "drssc",      "dsc", // Expose,        GraphicsExpose, NoExpose
	"Wc",                              // VisibilityNotify
	"Wwrsb",      "Ww",                // CreateNotify,     DestroyNotify
	"Wwb",        "Wwb",        "Ww",  // UnmapNotify,    MapNotify,   MapRequest
	"Wwwpb",                           // ReparentNotify
	"Wwwrsb",     "wWwrssD",           // ConfigureNotify,  ConfigureRequest
	"Wwp",        "Wss",               // GravityNotify,    ResizeRequest
	"Ww*c",       "Ww*c",              // CirculateNotify,  CirculateRequest
	"WaSb",       "Swa",               // PropertyNotify,   SelectionClear
	"lwwaaa",     "lwaaa",             // SelectionRequest, SelectionNotify
	"Wlbc",                            // ColormapNotify
	"WaD",                             // ClientMessage
	"cc",                              // MappingNotify
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
FT_Evnt_send_MSB (CLIENT * clnt, WINDOW * wind, CARD16 evnt, va_list vap)
{
	const char * frm = _EVNT_Form[evnt];
	xEvent     * evn = Evnt_Buffer (&clnt->oBuf, sizeof(xEvent));
	if (evn) {
		char * ptr = ((char*)evn) +4;
		
		#define ARG(t)   { *(t*)ptr = va_arg (vap, t); ptr += sizeof(t); }
		
		evn->u.u.type = evnt;
		evn->u.u.sequenceNumber = clnt->SeqNum;
		
		while (*frm) switch (*(frm++)) {
			case 'W': if (wind) { *(Window*)ptr = wind->Id; ptr += 4; break; }
			case 'w': ARG (Window);   break;
			case 'd': ARG (Drawable); break;
			case 'a': ARG (Atom);     break;
			case 'l': ARG (CARD32);   break;
			case 's': ARG (CARD16);   break;
			case 'c': ARG (CARD8);    break;
			case 'b': ARG (BOOL);     break;
			case 'p': ARG (PXY);      break;
			case 'r': *(GRECT*)ptr      = *va_arg (vap, GRECT*); ptr += 8; break;
			case 'D': evn->u.u.detail   =  va_arg (vap, CARD8);            break;
			case 'X': *(Window*)ptr     = ROOT_WINDOW;           ptr += 4; break;
			case 'S': *(Time*)ptr       = MAIN_TimeStamp;        ptr += 4; break;
			case 'M': *(KeyButMask*)ptr = MAIN_KeyButMask;       ptr += 2; break;
			case 'T': *ptr              = xTrue;                 ptr += 1; break;
			case 'F': *ptr              = xFalse;                ptr += 1; break;
			case '*':                                            ptr += 4; break;
		}
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr   |= clnt->FdSet;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
FT_Evnt_send_LSB (CLIENT * clnt, WINDOW * wind, CARD16 evnt, va_list vap)
{
	const char * frm = _EVNT_Form[evnt];
	xEvent     * evn = Evnt_Buffer (&clnt->oBuf, sizeof(xEvent));
	if (evn) {
		char * ptr = ((char*)evn) +4;
		
		#define ARG32(t) { *(t*)ptr = Swap32(va_arg (vap, t)); ptr += 4; }
		#define ARG16(t) { *(t*)ptr = Swap16(va_arg (vap, t)); ptr += 2; }
		
		evn->u.u.type = evnt;
		evn->u.u.sequenceNumber = Swap16(clnt->SeqNum);
		
		while (*frm) switch (*(frm++)) {
			case 'W': if (wind) {*(Window*)ptr = Swap32(wind->Id); ptr += 4; break; }
			case 'w': ARG32 (Window);   break;
			case 'd': ARG32 (Drawable); break;
			case 'a': ARG32 (Atom);     break;
			case 'l': ARG32 (CARD32);   break;
			case 's': ARG16 (CARD16);   break;
			case 'c': ARG   (CARD8);    break;
			case 'b': ARG   (BOOL);     break;
			case 'p': SwapPXY ((PXY*)ptr,  &va_arg (vap, PXY));    ptr += 4; break;
			case 'r': SwapRCT ((GRECT*)ptr, va_arg (vap, GRECT*)); ptr += 8; break;
			case 'D': evn->u.u.detail   =   va_arg (vap, CARD8);             break;
			case 'X': *(Window*)ptr     = SWAP32(ROOT_WINDOW);     ptr += 4; break;
			case 'S': *(Time*)ptr       = Swap32(MAIN_TimeStamp);  ptr += 4; break;
			case 'M': *(KeyButMask*)ptr = Swap16(MAIN_KeyButMask); ptr += 2; break;
			case 'T': *ptr              = xTrue;                   ptr += 1; break;
			case 'F': *ptr              = xFalse;                  ptr += 1; break;
			case '*':                                              ptr += 4; break;
		}
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr   |= clnt->FdSet;
	}
}

//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_SendEvent (CLIENT * clnt, xSendEventReq * q)
{
	// Sends a (synthetic) event to one or more clients, ignoring any grabs.
	//
	// BOOL   propagate:
	// Window destination:
	// CARD32 eventMask:
	// xEvent event
	//...........................................................................
	
	extern WINDOW * _WIND_PointerRoot;
	
	WINDOW * wind = NULL;
	
	if (q->event.u.u.type < 2 || q->event.u.u.type >= LASTEvent) {
		Bad(Value, q->event.u.u.type, SendEvent,);
	
	} else if (q->destination == PointerWindow) {
		wind = _WIND_PointerRoot;
	
	} else if (q->destination == InputFocus) {
		
		// this is only correct if PointerRoot is an inferior of InputFocus !!!
		//
		wind = _WIND_PointerRoot;
	
	} else if (!(wind = WindFind (q->destination))) {
		Bad(Window, q->destination, SendEvent,);
	}
	//...........................................................................
	
	if (wind) {
		WINDEVNT _lst, * lst = &_lst;
		CARD16   num  = 0;
		CARD32   mask = q->eventMask;
		
		PRINT (SendEvent," W:%lX mask=%lX #%i %s",
		       q->destination, q->eventMask, q->event.u.u.type,
		       (q->propagate ? "prop." : "first"));
	
		if (!q->eventMask) {
			if ((_lst.Client = ClntFind (wind->Id))) {
				mask = _lst.Mask = AllEventMask;
				num  = 1;
			}
		} else do {
			if (wind->u.List.AllMasks & q->eventMask) {
				num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
				lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
				break;
			} else if (!q->propagate) {
				break;
			}
		} while ((wind = wind->Parent));
		
		if (q->event.u.u.type == ClientMessage) while (num--) {
			if (lst->Mask & mask) {
				EvntClientMsg (lst->Client, q->event.u.clientMessage.window,
				               q->event.u.clientMessage.u.b.type,
				               q->event.u.u.detail,
				               q->event.u.clientMessage.u.b.bytes);
			}
			lst++;
		
		} else while (num--) {
			if (lst->Mask & mask) {
				CLIENT * rcp = lst->Client;
				xEvent * evn = Evnt_Buffer (&rcp->oBuf, sizeof(xEvent));
				if (evn) {
					evn->u.u.type           = q->event.u.u.type | 0x80;
					evn->u.u.detail         = q->event.u.u.detail;
					evn->u.u.sequenceNumber = (rcp->DoSwap ? Swap16(rcp->SeqNum)
					                                       :        rcp->SeqNum );
					if (rcp->DoSwap == clnt->DoSwap) {
						memcpy (&evn->u.clientMessage.window,
						        &q->event.u.clientMessage.window, 28);
					} else {
						const char * frm = _EVNT_Form[q->event.u.u.type];
						char       * dst = ((char*)evn) +4;
						char       * src = ((char*)&q->event) +4;
						while (*frm) switch (*(frm++)) {
							case 'c': case 'b': case 'T': case 'F':
								*(dst++) = *(src++);
								break;
							case 's': case 'M':
								*(short*)dst = Swap16 (*(short*)src);
								dst += 2; src += 2;
								break;
							case 'X': case 'W': case 'w': case 'd':case 'a': case 'l':
							case 'S':
								*(long*)dst = Swap32 (*(long*)src);
							case '*':
								dst += 4; src += 4;
							case 'D':
								break;
							case 'p':
								SwapPXY ((PXY*)dst, (PXY*)src);
								dst += 4; src += 4;
								break;
							case 'r':
								SwapRCT ((GRECT*)dst, (GRECT*)src);
								dst += 8; src += 8;
								break;
						}
					}
					rcp->oBuf.Left += sizeof(xEvent);
					MAIN_FDSET_wr  |= rcp->FdSet;
				}
			}
			lst++;
		}
	}
}
