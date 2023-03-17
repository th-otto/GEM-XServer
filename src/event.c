/*
 *==============================================================================
 *
 * event.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-07-01 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "tools.h"
#include "clnt.h"
#include "event.h"
#include "window.h"
#include "x_printf.h"

#include <X11/Xproto.h>

#ifndef GenericEvent
#define GenericEvent 35
#endif

/* ------------------------------------------------------------------------------ */
#ifdef TRACE
static const char *_M2STR(CARD8 mode)
{
	static char inval[16];

	switch (mode)
	{
	case NotifyAncestor:
		return "ancestor";
	case NotifyVirtual:
		return "virtual";
	case NotifyInferior:
		return "inferior";
	case NotifyNonlinear:
		return "nonlinear";
	case NotifyNonlinearVirtual:
		return "nonl.virt.";
	case NotifyPointer:
		return "pointer";
	case NotifyPointerRoot:
		return "ptr.root";
	case NotifyDetailNone:
		return "no_detail";
	}
	sprintf(inval, "<%u>", mode);

	return inval;
}

#define TRACEP(w,el,m,p,id,r) x_printf ("W:%X " el " %s [%i,%i] W:%lX [%i,%i] \n", \
                                       w->Id, _M2STR(m), p.p_x, p.p_y, id, r.p_x, r.p_y)

#else
#define TRACEP(w,el,m,p,id,r)
#endif


/* ============================================================================== */
BOOL EvntSet(WINDOW *wind, CLIENT *clnt, CARD32 mask)
{
	CARD16 num;
	WINDEVNT *src;
	WINDEVNT *dst = NULL;

	if (wind->u.List.AllMasks < 0)
	{
		int i = num = wind->u.List.p->Length;

		dst = src = wind->u.List.p->Event;
		while (i-- && dst->Client != clnt)
			dst++;
		if (i < 0)
			dst = NULL;
		else
			dst->Mask |= mask;
	} else
	{
		src = &wind->u.Event;
		num = 1;

		if (src->Client == clnt)
		{
			dst = src;
		} else if (!src->Client)
		{
			dst = src;
			dst->Client = clnt;
			clnt->EventReffs++;
		}
	}

	if (dst == NULL)
	{
		size_t len = sizeof(WINDEVNT) * num;

		typeof(wind->u.List.p) lst = malloc(sizeof(*lst) + len);
		if (!lst)
			return xFalse;

		lst->Length = num + 1;
		memcpy(lst->Event, src, len);
		if (num > 1)
			free(wind->u.List.p);
		else
			wind->u.List.AllMasks |= 0x80000000uL;
		wind->u.List.p = lst;
		lst->Event[num].Mask = mask;
		lst->Event[num].Client = clnt;
		clnt->EventReffs++;

	} else if (num > 1 && (~wind->u.List.AllMasks & mask))
	{
		int i;

		wind->u.List.AllMasks = 0x80000000uL;
		for (i = 0, mask = 0uL; i < num; mask |= src[i++].Mask)
			;
	}

	wind->u.List.AllMasks |= mask;

	return xTrue;
}

/* ============================================================================== */
BOOL EvntClr(WINDOW *wind, CLIENT *clnt)
{
	if (wind->u.List.AllMasks < 0)
	{
		CARD16 num = wind->u.List.p->Length;
		WINDEVNT *ptr = wind->u.List.p->Event;
		CARD32 mask = 0uL;

		while (ptr->Client != clnt)
		{
			mask |= ptr->Mask;
			ptr++;
			if (!--num)
				break;
		}
		if (num)
		{
			if (--wind->u.List.p->Length == 1)
			{
				CLIENT *left = ptr[num == 2 ? +1 : -1].Client;

				free(wind->u.List.p);
				wind->u.Event.Client = left;
			} else
			{
				mask |= 0x80000000uL;
				while (--num)
				{
					mask |= ptr[0].Mask = ptr[1].Mask;
					ptr[0].Client = ptr[1].Client;
					ptr++;
				}
				ptr[0].Mask = 0uL;
				ptr[0].Client = NULL;
			}
			wind->u.List.AllMasks = mask;
			clnt->EventReffs--;
			return xTrue;
		}
	} else if (wind->u.List.AllMasks && wind->u.Event.Client == clnt)
	{
		wind->u.Event.Mask = 0uL;
		wind->u.Event.Client = NULL;
		clnt->EventReffs--;
		return xTrue;
	}
	return xFalse;
}

/* ============================================================================== */
void EvntDel(WINDOW *wind)
{
	CARD16 num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT *lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);

	while (num--)
	{
		if (lst->Mask)
		{
			lst->Client->EventReffs--;
		}
		lst++;
	}
	if (wind->u.List.AllMasks < 0)
	{
		free(wind->u.List.p);
	}
	wind->u.Event.Mask = NoEventMask;
	wind->u.Event.Client = NULL;
}

/* ============================================================================== */
WINDOW *EvntSearch(WINDOW *wind, CLIENT *clnt, CARD32 mask)
{
	while (wind)
	{
		if (mask & wind->u.List.AllMasks)
		{
			CARD16 num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
			WINDEVNT *lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);

			while (num--)
			{
				if (lst->Client == clnt && lst->Mask & mask)
					return wind;
				lst++;
			}
		} else if (!(mask &= wind->PropagateMask))
		{
			break;
		}
		wind = wind->Parent;
	}
	return NULL;
}

/* ============================================================================== */
CLIENT *EvntClient(WINDOW *wind, CARD32 mask)
{
	CARD16 num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT *lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);

	while (num--)
	{
		if ((lst->Mask & mask) == mask)
			return lst->Client;
		lst++;
	}
	return NULL;
}


/* ------------------------------------------------------------------------------ */
static inline xEvent *Evnt_Buffer(O_BUFF *buf, size_t need)
{
	void *r;
	size_t n = buf->Done + buf->Left;

	if (n + need <= buf->Size)
		r = buf->Mem + n;
	else
		r = ClntOutBuffer(buf, need, 0, 0);
	return r;
}

/* ============================================================================== */
void EvntExpose(WINDOW *wind, short len, const GRECT *rect)
{
	CARD16 num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
	WINDEVNT *lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
	size_t spc = sizeof(xEvent) * len;
	xEvent *evn;

	while (num--)
	{
		if ((lst->Mask & ExposureMask) && (evn = Evnt_Buffer(&lst->Client->oBuf, spc)) != NULL)
		{
			const GRECT *rct = rect;
			int cnt = len;

			if (lst->Client->DoSwap)
			{
				CARD16 seq = Swap16(lst->Client->SeqNum);
				CARD32 wid = Swap32(wind->Id);

				while (cnt--)
				{
					evn->u.u.type = Expose;
					evn->u.u.sequenceNumber = seq;
					evn->u.expose.window = wid;
					evn->u.expose.count = Swap16(cnt);
					evn->u.expose.x = Swap16(rct->g_x);
					evn->u.expose.y = Swap16(rct->g_y);
					evn->u.expose.width = Swap16(rct->g_w);
					evn->u.expose.height = Swap16(rct->g_h);
					rct++;
					evn++;
				}
			} else
			{
				CARD16 seq = lst->Client->SeqNum;
				CARD32 wid = wind->Id;

				while (cnt--)
				{
					evn->u.u.type = Expose;
					evn->u.u.sequenceNumber = seq;
					evn->u.expose.window = wid;
					evn->u.expose.count = cnt;
					evn->u.expose.x = rct->g_x;
					evn->u.expose.y = rct->g_y;
					evn->u.expose.width = rct->g_w;
					evn->u.expose.height = rct->g_h;
					rct++;
					evn++;
				}
			}
			lst->Client->oBuf.Left += spc;
			MAIN_FDSET_wr |= lst->Client->FdSet;
		}
		lst++;
	}
}

/* ============================================================================== */
void EvntGraphExpose(CLIENT *clnt, p_DRAWABLE draw, CARD16 major, short len, const GRECT *rect)
{
	size_t spc = sizeof(xEvent) * len;
	xEvent *evn = Evnt_Buffer(&clnt->oBuf, spc);

	if (evn)
	{
		if (clnt->DoSwap)
		{
			CARD16 seq = Swap16(clnt->SeqNum);
			CARD32 did = Swap32(draw.p->Id);

			major = Swap16(major);
			while (len--)
			{
				evn->u.u.type = GraphicsExpose;
				evn->u.u.sequenceNumber = seq;
				evn->u.graphicsExposure.drawable = did;
				evn->u.graphicsExposure.count = Swap16(len);
				evn->u.graphicsExposure.minorEvent = 0;
				evn->u.graphicsExposure.majorEvent = major;
				evn->u.graphicsExposure.x = Swap16(rect->g_x);
				evn->u.graphicsExposure.y = Swap16(rect->g_y);
				evn->u.graphicsExposure.width = Swap16(rect->g_w);
				evn->u.graphicsExposure.height = Swap16(rect->g_h);
				rect++;
				evn++;
			}
		} else
		{
			CARD16 seq = clnt->SeqNum;
			CARD32 did = draw.p->Id;

			while (len--)
			{
				evn->u.u.type = GraphicsExpose;
				evn->u.u.sequenceNumber = seq;
				evn->u.graphicsExposure.drawable = did;
				evn->u.graphicsExposure.count = len;
				evn->u.graphicsExposure.minorEvent = 0;
				evn->u.graphicsExposure.majorEvent = major;
				evn->u.graphicsExposure.x = rect->g_x;
				evn->u.graphicsExposure.y = rect->g_y;
				evn->u.graphicsExposure.width = rect->g_w;
				evn->u.graphicsExposure.height = rect->g_h;
				rect++;
				evn++;
			}
		}
		clnt->oBuf.Left += spc;
		MAIN_FDSET_wr |= clnt->FdSet;
	}
}

/* ============================================================================== */
void EvntClientMsg(CLIENT *clnt, Window id, Atom type, BYTE format, const void *data)
{
	xEvent *evn = Evnt_Buffer(&clnt->oBuf, sizeof(xEvent));

	if (evn)
	{
		evn->u.u.type = ClientMessage;
		evn->u.u.detail = format;
		if (clnt->DoSwap)
		{
			evn->u.u.sequenceNumber = Swap16(clnt->SeqNum);
			evn->u.clientMessage.window = Swap32(id);
			evn->u.clientMessage.u.l.type = Swap32(type);
		} else
		{
			evn->u.u.sequenceNumber = clnt->SeqNum;
			evn->u.clientMessage.window = id;
			evn->u.clientMessage.u.l.type = type;
			format = 0;
		}
		if (format == 32)
		{
			const CARD32 *src = data;
			INT32 *dst = &evn->u.clientMessage.u.l.longs0;

			*dst++ = Swap32(*src++);
			*dst++ = Swap32(*src++);
			*dst++ = Swap32(*src++);
			*dst++ = Swap32(*src++);
			*dst++ = Swap32(*src++);
		} else if (format == 16)
		{
			const CARD16 *src = data;
			INT16 *dst = &evn->u.clientMessage.u.s.shorts0;

			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
			*dst++ = Swap16(*src++);
		} else
		{
			const CARD32 *src = data;
			INT32 *dst = &evn->u.clientMessage.u.l.longs0;

			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
		}
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr |= clnt->FdSet;
	}
}

/* ============================================================================== */
BOOL EvntPropagate(WINDOW *wind, CARD32 mask, BYTE event, Window c_id, PXY r_xy, PXY e_xy, BYTE detail)
{
	CARD32 chld = c_id == wind->Id ? None : c_id;
	BOOL exec = xFalse;

	do
	{
		if (mask & wind->u.List.AllMasks)
		{
			xEvent ev;
			
			ev.u.u.type = event;
			ev.u.u.detail = detail;
			ev.u.keyButtonPointer.time = MAIN_TimeStamp;
			ev.u.keyButtonPointer.root = ROOT_WINDOW;
			ev.u.keyButtonPointer.event = wind->Id;
			ev.u.keyButtonPointer.child = chld;
			ev.u.keyButtonPointer.rootX = r_xy.p_x;
			ev.u.keyButtonPointer.rootY = r_xy.p_y;
			ev.u.keyButtonPointer.eventX = e_xy.p_x;
			ev.u.keyButtonPointer.eventY = e_xy.p_y;
			ev.u.keyButtonPointer.state = MAIN_KeyButMask;
			ev.u.keyButtonPointer.sameScreen = xTrue;
			ev.u.keyButtonPointer.pad1 = 0;
			_Evnt_Window(wind, mask, &ev);
			exec = xTrue;
			mask &= !wind->u.List.AllMasks;
		}
		if (mask &= wind->PropagateMask)
		{
			e_xy.p_x += wind->Rect.g_x;
			e_xy.p_y += wind->Rect.g_y;
			if (c_id)
				chld = wind->Id;
		} else
		{
			break;
		}
	} while ((wind = wind->Parent) != NULL);

	return exec;
}

/* ============================================================================== */
void EvntPointer(WINDOW **stack, int anc, int top, PXY e_xy, PXY r_xy, CARD32 r_id, CARD8 mode)
{
	CARD8 detail;
	CARD8 next;
	CARD8 last;
	int bot;

	/*--- generate events ---*/

	if (anc == 0)
	{
		detail = NotifyInferior;
		next = NotifyVirtual;
		last = NotifyAncestor;
	} else if (anc == top)
	{
		detail = NotifyAncestor;
		next = NotifyVirtual;
		last = NotifyInferior;
	} else
	{
		detail = NotifyNonlinear;
		next = NotifyNonlinearVirtual;
		last = NotifyNonlinear;
	}

	/* notify enter events */

	if (stack[0])
	{
		TRACEP(stack[0], "Leave", detail, e_xy, r_id, r_xy);
		if (stack[0]->u.List.AllMasks & LeaveWindowMask)
		{
			EvntLeaveNotify(stack[0], r_id, None, r_xy, e_xy, mode, ELFlagSameScreen, detail);
		}
		if (anc > 0)
		{
			e_xy.p_x += stack[0]->Rect.g_x;
			e_xy.p_y += stack[0]->Rect.g_y;
		}
	}
	for (bot = 1; bot < anc; ++bot)
	{
		TRACEP(stack[bot], "Leave", next, e_xy, r_id, r_xy);
		if (stack[bot]->u.List.AllMasks & LeaveWindowMask)
		{
			EvntLeaveNotify(stack[bot], r_id, None, r_xy, e_xy, mode, ELFlagSameScreen, next);
		}
		e_xy.p_x += stack[bot]->Rect.g_x;
		e_xy.p_y += stack[bot]->Rect.g_y;
	}

	/* notify leave events */

	for (bot = anc + 1; bot < top; ++bot)
	{
		e_xy.p_x -= stack[bot]->Rect.g_x;
		e_xy.p_y -= stack[bot]->Rect.g_y;
		TRACEP(stack[bot], "Enter", next, e_xy, r_id, r_xy);
		if (stack[bot]->u.List.AllMasks & EnterWindowMask)
		{
			EvntEnterNotify(stack[bot], r_id, None, r_xy, e_xy, mode, ELFlagSameScreen, next);
		}
	}
	if (stack[top])
	{
		if (anc < top)
		{
			e_xy.p_x -= stack[top]->Rect.g_x;
			e_xy.p_y -= stack[top]->Rect.g_y;
		}
		TRACEP(stack[top], "Enter", last, e_xy, r_id, r_xy);
		if (stack[top]->u.List.AllMasks & EnterWindowMask)
		{
			EvntEnterNotify(stack[top], r_id, None, r_xy, e_xy, mode, ELFlagSameScreen, last);
		}
	}
}


/* ============================================================================== */
void EvntMappingNotify(CARD8 request, CARD8 first, CARD8 count)
{
	CLIENT *clnt = CLNT_Base;
	xEvent *evn;

	while (clnt)
	{
		if ((evn = Evnt_Buffer(&clnt->oBuf, sizeof(xEvent))) != NULL)
		{
			evn->u.u.type = MappingNotify;
			evn->u.u.sequenceNumber = clnt->DoSwap ? Swap16(clnt->SeqNum) : clnt->SeqNum;
			evn->u.mappingNotify.request = request;
			evn->u.mappingNotify.firstKeyCode = first;
			evn->u.mappingNotify.count = count;
			clnt->oBuf.Left += sizeof(xEvent);
			MAIN_FDSET_wr |= clnt->FdSet;
		}
		clnt = clnt->Next;
	}
}


/* ------------------------------------------------------------------------------ */

void _Evnt_Window(WINDOW *wind, CARD32 mask, const xEvent *ev)
{
	CARD16 num = wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1;
	WINDEVNT *lst = num > 1 ? wind->u.List.p->Event : &wind->u.Event;

	if (ev->u.u.type < 2 || ev->u.u.type >= LASTEvent)
	{
		x_printf("\033pERROR\033q invalid event value %u for W:%X.\n", ev->u.u.type, wind->Id);
		return;
	}
	if (mask & ~AllEventMask)
	{
		x_printf("\033pWARNING\033q invalid event mask %lX for W:%X.\n", mask, wind->Id);
		mask &= AllEventMask;
	}

	while (num--)
	{
		if (lst->Mask & mask)
		{
			lst->Client->Fnct->event_send(lst->Client, wind, ev);
		}
		lst++;
	}
}

/* ------------------------------------------------------------------------------ */
void _Evnt_Struct(WINDOW *wind, const xEvent *ev)
{
	if (ev->u.u.type < 2 || ev->u.u.type >= LASTEvent)
	{
		x_printf("\033pERROR\033q invalid event value %u for W:%X.\n", ev->u.u.type, wind->Id);
		return;
	}
	if (wind->u.List.AllMasks & StructureNotifyMask)
	{
		CARD16 num = wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1;
		WINDEVNT *lst = num > 1 ? wind->u.List.p->Event : &wind->u.Event;

		while (num--)
		{
			if (lst->Mask & StructureNotifyMask)
			{
				lst->Client->Fnct->event_send(lst->Client, wind, ev);
			}
			lst++;
		}
	}
	if ((wind = wind->Parent) != NULL && (wind->u.List.AllMasks & SubstructureNotifyMask))
	{
		CARD16 num = wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1;
		WINDEVNT *lst = num > 1 ? wind->u.List.p->Event : &wind->u.Event;

		while (num--)
		{
			if (lst->Mask & SubstructureNotifyMask)
			{
				lst->Client->Fnct->event_send(lst->Client, wind, ev);
			}
			lst++;
		}
	}
}

/* ------------------------------------------------------------------------------ */
void _Evnt_Client(CLIENT *clnt, const xEvent *ev)
{
	if (ev->u.u.type < 2 || ev->u.u.type >= LASTEvent)
	{
		x_printf("\033pERROR\033q invalid event value %u.\n", ev->u.u.type);
	} else
	{
		clnt->Fnct->event_send(clnt, NULL, ev);
	}
}


/*
 *==============================================================================
 */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FT_Evnt_send_Unswapped(CLIENT *clnt, WINDOW *wind, const xEvent *ev)
{
	xEvent *evn = Evnt_Buffer(&clnt->oBuf, sizeof(xEvent));

	if (evn)
	{
		*evn = *ev;
		evn->u.u.sequenceNumber = clnt->SeqNum;
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr |= clnt->FdSet;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FT_Evnt_send_Swapped(CLIENT *clnt, WINDOW *wind, const xEvent *ev)
{
	xEvent *evn = Evnt_Buffer(&clnt->oBuf, sizeof(xEvent));

	if (evn)
	{
		*evn = *ev;
		evn->u.u.sequenceNumber = Swap16(clnt->SeqNum);

		switch (evn->u.u.type)
		{
		case KeyPress:
		case KeyRelease:
		case ButtonPress:
		case ButtonRelease:
		case MotionNotify:
			evn->u.keyButtonPointer.time = Swap32(evn->u.keyButtonPointer.time);
			evn->u.keyButtonPointer.root = Swap32(evn->u.keyButtonPointer.root);
			evn->u.keyButtonPointer.event = Swap32(evn->u.keyButtonPointer.event);
			evn->u.keyButtonPointer.child = Swap32(evn->u.keyButtonPointer.child);
			evn->u.keyButtonPointer.rootX = Swap16(evn->u.keyButtonPointer.rootX);
			evn->u.keyButtonPointer.rootY = Swap16(evn->u.keyButtonPointer.rootY);
			evn->u.keyButtonPointer.eventX = Swap16(evn->u.keyButtonPointer.eventX);
			evn->u.keyButtonPointer.eventY = Swap16(evn->u.keyButtonPointer.eventY);
			evn->u.keyButtonPointer.state = Swap16(evn->u.keyButtonPointer.state);
			break;
		
		case EnterNotify:
		case LeaveNotify:
			evn->u.enterLeave.time = Swap32(evn->u.enterLeave.time);
			evn->u.enterLeave.root = Swap32(evn->u.enterLeave.root);
			evn->u.enterLeave.event = Swap32(evn->u.enterLeave.event);
			evn->u.enterLeave.child = Swap32(evn->u.enterLeave.child);
			evn->u.enterLeave.rootX = Swap16(evn->u.enterLeave.rootX);
			evn->u.enterLeave.rootY = Swap16(evn->u.enterLeave.rootY);
			evn->u.enterLeave.eventX = Swap16(evn->u.enterLeave.eventX);
			evn->u.enterLeave.eventY = Swap16(evn->u.enterLeave.eventY);
			evn->u.enterLeave.state = Swap16(evn->u.enterLeave.state);
			break;
		
		case FocusIn:
		case FocusOut:
			evn->u.focus.window = Swap32(evn->u.focus.window);
			break;
		
		case KeymapNotify:
			/* nothing to do */
			break;
			
		case Expose:
			/* swapping already done in EvntExpose */
			break;
		
		case GraphicsExpose:
			/* swapping already done in EvntGraphExpose */
			break;
		
		case NoExpose:
			evn->u.noExposure.drawable = Swap32(evn->u.noExposure.drawable);
			evn->u.noExposure.minorEvent = Swap16(evn->u.noExposure.minorEvent);
			break;
		
		case VisibilityNotify:
			evn->u.visibility.window = Swap32(evn->u.visibility.window);
			break;
		
		case CreateNotify:
			evn->u.createNotify.parent = Swap32(evn->u.createNotify.parent);
			evn->u.createNotify.window = Swap32(evn->u.createNotify.window);
			evn->u.createNotify.x = Swap16(evn->u.createNotify.x);
			evn->u.createNotify.y = Swap16(evn->u.createNotify.y);
			evn->u.createNotify.width = Swap16(evn->u.createNotify.width);
			evn->u.createNotify.height = Swap16(evn->u.createNotify.height);
			evn->u.createNotify.borderWidth = Swap16(evn->u.createNotify.borderWidth);
			break;
		
		case DestroyNotify:
			evn->u.destroyNotify.event = Swap32(evn->u.destroyNotify.event);
			evn->u.destroyNotify.window = Swap32(evn->u.destroyNotify.window);
			break;
		
		case UnmapNotify:
			evn->u.unmapNotify.event = Swap32(evn->u.unmapNotify.event);
			evn->u.unmapNotify.window = Swap32(evn->u.unmapNotify.window);
			break;
		
		case MapNotify:
			evn->u.mapNotify.event = Swap32(evn->u.mapNotify.event);
			evn->u.mapNotify.window = Swap32(evn->u.mapNotify.window);
			break;
		
		case MapRequest:
			evn->u.mapRequest.parent = Swap32(evn->u.mapRequest.parent);
			evn->u.mapRequest.window = Swap32(evn->u.mapRequest.window);
			break;
			
		case ReparentNotify:
			evn->u.reparent.event = Swap32(evn->u.reparent.event);
			evn->u.reparent.window = Swap32(evn->u.reparent.window);
			evn->u.reparent.parent = Swap32(evn->u.reparent.parent);
			evn->u.reparent.x = Swap16(evn->u.reparent.x);
			evn->u.reparent.y = Swap16(evn->u.reparent.y);
			break;
		
		case ConfigureNotify:
			evn->u.configureNotify.event = Swap32(evn->u.configureNotify.event);
			evn->u.configureNotify.window = Swap32(evn->u.configureNotify.window);
			evn->u.configureNotify.aboveSibling = Swap32(evn->u.configureNotify.aboveSibling);
			evn->u.configureNotify.x = Swap16(evn->u.configureNotify.x);
			evn->u.configureNotify.y = Swap16(evn->u.configureNotify.y);
			evn->u.configureNotify.width = Swap16(evn->u.configureNotify.width);
			evn->u.configureNotify.height = Swap16(evn->u.configureNotify.height);
			evn->u.configureNotify.borderWidth = Swap16(evn->u.configureNotify.borderWidth);
			break;
		
		case ConfigureRequest:
			evn->u.configureRequest.parent = Swap32(evn->u.configureRequest.parent);
			evn->u.configureRequest.window = Swap32(evn->u.configureRequest.window);
			evn->u.configureRequest.sibling = Swap32(evn->u.configureRequest.sibling);
			evn->u.configureRequest.x = Swap16(evn->u.configureRequest.x);
			evn->u.configureRequest.y = Swap16(evn->u.configureRequest.y);
			evn->u.configureRequest.width = Swap16(evn->u.configureRequest.width);
			evn->u.configureRequest.height = Swap16(evn->u.configureRequest.height);
			evn->u.configureRequest.borderWidth = Swap16(evn->u.configureRequest.borderWidth);
			evn->u.configureRequest.valueMask = Swap16(evn->u.configureRequest.valueMask);
			break;
		
		case GravityNotify:
			evn->u.gravity.event = Swap32(evn->u.gravity.event);
			evn->u.gravity.window = Swap32(evn->u.gravity.window);
			evn->u.gravity.x = Swap16(evn->u.gravity.x);
			evn->u.gravity.y = Swap16(evn->u.gravity.y);
			break;
		
		case ResizeRequest:
			evn->u.resizeRequest.window = Swap32(evn->u.resizeRequest.window);
			evn->u.resizeRequest.width = Swap16(evn->u.resizeRequest.width);
			evn->u.resizeRequest.height = Swap16(evn->u.resizeRequest.height);
			break;
		
		case CirculateNotify:
		case CirculateRequest:
			evn->u.circulate.event = Swap32(evn->u.circulate.event);
			evn->u.circulate.window = Swap32(evn->u.circulate.window);
			evn->u.circulate.parent = Swap32(evn->u.circulate.parent);
			break;
		
		case PropertyNotify:
			evn->u.property.window = Swap32(evn->u.property.window);
			evn->u.property.atom = Swap32(evn->u.property.atom);
			evn->u.property.time = Swap32(evn->u.property.time);
			break;
		
		case SelectionClear:
			evn->u.selectionClear.time = Swap32(evn->u.selectionClear.time);
			evn->u.selectionClear.window = Swap32(evn->u.selectionClear.window);
			evn->u.selectionClear.atom = Swap32(evn->u.selectionClear.atom);
			break;
		
		case SelectionRequest:
			evn->u.selectionRequest.time = Swap32(evn->u.selectionRequest.time);
			evn->u.selectionRequest.owner = Swap32(evn->u.selectionRequest.owner);
			evn->u.selectionRequest.requestor = Swap32(evn->u.selectionRequest.requestor);
			evn->u.selectionRequest.selection = Swap32(evn->u.selectionRequest.selection);
			evn->u.selectionRequest.target = Swap32(evn->u.selectionRequest.target);
			evn->u.selectionRequest.property = Swap32(evn->u.selectionRequest.property);
			break;
		
		case SelectionNotify:
			evn->u.selectionNotify.time = Swap32(evn->u.selectionNotify.time);
			evn->u.selectionNotify.requestor = Swap32(evn->u.selectionNotify.requestor);
			evn->u.selectionNotify.selection = Swap32(evn->u.selectionNotify.selection);
			evn->u.selectionNotify.target = Swap32(evn->u.selectionNotify.target);
			evn->u.selectionNotify.property = Swap32(evn->u.selectionNotify.property);
			break;
		
		case ColormapNotify:
			evn->u.colormap.window = Swap32(evn->u.colormap.window);
			evn->u.colormap.colormap = Swap32(evn->u.colormap.colormap);
			break;
		
		case MappingNotify:
			/* nothing to do */
			break;
		
		case ClientMessage:
			/* already swapped in EvntClientMsg */
			break;
		}
		clnt->oBuf.Left += sizeof(xEvent);
		MAIN_FDSET_wr |= clnt->FdSet;
	}
}

/* ============================================================================== */
/* */
/* Callback Functions */

#include "Request.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_SendEvent(CLIENT *clnt, xSendEventReq *q)
{
	/*
	 * Sends a (synthetic) event to one or more clients, ignoring any grabs.
	 *
	 * BOOL   propagate:
	 * Window destination:
	 * CARD32 eventMask:
	 * xEvent event
	 *...........................................................................
	 */
	WINDOW *wind = NULL;

	if (q->event.u.u.type < 2 || q->event.u.u.type >= LASTEvent)
	{
		Bad(BadValue, q->event.u.u.type, X_SendEvent, "_");
	} else if (q->destination == PointerWindow)
	{
		wind = _WIND_PointerRoot;
	} else if (q->destination == InputFocus)
	{
		/* this is only correct if PointerRoot is an inferior of InputFocus !!! */
		wind = _WIND_PointerRoot;
	} else if ((wind = WindFind(q->destination)) == NULL)
	{
		Bad(BadWindow, q->destination, X_SendEvent, "_");
	}

	if (wind)
	{
		WINDEVNT _lst;
		WINDEVNT *lst = &_lst;
		CARD16 num = 0;
		CARD32 mask = q->eventMask;

		PRINT(X_SendEvent, " W:%lX mask=%lX #%i %s",
			  q->destination, q->eventMask, q->event.u.u.type, q->propagate ? "prop." : "first");

		if (!q->eventMask)
		{
			if ((_lst.Client = ClntFind(wind->Id)) != NULL)
			{
				mask = _lst.Mask = AllEventMask;
				num = 1;
			}
		} else
		{
			do
			{
				if (wind->u.List.AllMasks & q->eventMask)
				{
					num = (wind->u.List.AllMasks < 0 ? wind->u.List.p->Length : 1);
					lst = (num > 1 ? wind->u.List.p->Event : &wind->u.Event);
					break;
				} else if (!q->propagate)
				{
					break;
				}
			} while ((wind = wind->Parent) != NULL);
		}

		if (q->event.u.u.type == ClientMessage)
		{
			while (num--)
			{
				if (lst->Mask & mask)
				{
					EvntClientMsg(lst->Client, q->event.u.clientMessage.window,
								  q->event.u.clientMessage.u.l.type,
								  q->event.u.u.detail, q->event.u.clientMessage.u.b.bytes);
				}
				lst++;
			}
		} else
		{
			while (num--)
			{
				if (lst->Mask & mask)
				{
					CLIENT *rcp = lst->Client;
					xEvent *evn = Evnt_Buffer(&rcp->oBuf, sizeof(xEvent));

					if (evn != NULL)
					{
						evn->u.u.type = q->event.u.u.type | 0x80;
						evn->u.u.detail = q->event.u.u.detail;
						evn->u.u.sequenceNumber = rcp->DoSwap ? Swap16(rcp->SeqNum) : rcp->SeqNum;
						if (rcp->DoSwap == clnt->DoSwap)
						{
							memcpy(&evn->u.clientMessage.window, &q->event.u.clientMessage.window, 28);
						} else
						{
							switch (q->event.u.u.type)
							{
							case KeyPress:
							case KeyRelease:
							case ButtonPress:
							case ButtonRelease:
							case MotionNotify:
								evn->u.keyButtonPointer.time = Swap32(q->event.u.keyButtonPointer.time);
								evn->u.keyButtonPointer.root = Swap32(q->event.u.keyButtonPointer.root);
								evn->u.keyButtonPointer.event = Swap32(q->event.u.keyButtonPointer.event);
								evn->u.keyButtonPointer.child = Swap32(q->event.u.keyButtonPointer.child);
								evn->u.keyButtonPointer.rootX = Swap16(q->event.u.keyButtonPointer.rootX);
								evn->u.keyButtonPointer.rootY = Swap16(q->event.u.keyButtonPointer.rootY);
								evn->u.keyButtonPointer.eventX = Swap16(q->event.u.keyButtonPointer.eventX);
								evn->u.keyButtonPointer.eventY = Swap16(q->event.u.keyButtonPointer.eventY);
								evn->u.keyButtonPointer.state = Swap16(q->event.u.keyButtonPointer.state);
								evn->u.keyButtonPointer.sameScreen = q->event.u.keyButtonPointer.sameScreen;
								break;
							
							case EnterNotify:
							case LeaveNotify:
								evn->u.enterLeave.time = Swap32(q->event.u.enterLeave.time);
								evn->u.enterLeave.root = Swap32(q->event.u.enterLeave.root);
								evn->u.enterLeave.event = Swap32(q->event.u.enterLeave.event);
								evn->u.enterLeave.child = Swap32(q->event.u.enterLeave.child);
								evn->u.enterLeave.rootX = Swap16(q->event.u.enterLeave.rootX);
								evn->u.enterLeave.rootY = Swap16(q->event.u.enterLeave.rootY);
								evn->u.enterLeave.eventX = Swap16(q->event.u.enterLeave.eventX);
								evn->u.enterLeave.eventY = Swap16(q->event.u.enterLeave.eventY);
								evn->u.enterLeave.state = Swap16(q->event.u.enterLeave.state);
								evn->u.enterLeave.mode = q->event.u.enterLeave.mode;
								evn->u.enterLeave.flags = q->event.u.enterLeave.flags;
								break;
							
							case FocusIn:
							case FocusOut:
								evn->u.focus.window = Swap32(q->event.u.focus.window);
								evn->u.focus.mode = q->event.u.focus.mode;
								break;
							
							case KeymapNotify:
								memcpy((char *)evn + 1, (const char *)q + 1, 31);
								break;
								
							case Expose:
								evn->u.expose.window = Swap32(q->event.u.expose.window);
								evn->u.expose.x = Swap16(q->event.u.expose.x);
								evn->u.expose.y = Swap16(q->event.u.expose.y);
								evn->u.expose.width = Swap16(q->event.u.expose.width);
								evn->u.expose.height = Swap16(q->event.u.expose.height);
								evn->u.expose.count = Swap16(q->event.u.expose.count);
								break;
							
							case GraphicsExpose:
								evn->u.graphicsExposure.drawable = Swap32(q->event.u.graphicsExposure.drawable);
								evn->u.graphicsExposure.x = Swap16(q->event.u.graphicsExposure.x);
								evn->u.graphicsExposure.y = Swap16(q->event.u.graphicsExposure.y);
								evn->u.graphicsExposure.width = Swap16(q->event.u.graphicsExposure.width);
								evn->u.graphicsExposure.height = Swap16(q->event.u.graphicsExposure.height);
								evn->u.graphicsExposure.minorEvent = Swap16(q->event.u.graphicsExposure.minorEvent);
								evn->u.graphicsExposure.count = Swap16(q->event.u.graphicsExposure.count);
								evn->u.graphicsExposure.majorEvent = q->event.u.graphicsExposure.majorEvent;
								break;
							
							case NoExpose:
								evn->u.noExposure.drawable = Swap32(q->event.u.noExposure.drawable);
								evn->u.noExposure.minorEvent = Swap16(q->event.u.noExposure.minorEvent);
								evn->u.noExposure.majorEvent = q->event.u.noExposure.majorEvent;
								break;
							
							case VisibilityNotify:
								evn->u.visibility.window = Swap32(q->event.u.visibility.window);
								evn->u.visibility.state = q->event.u.visibility.state;
								break;
							
							case CreateNotify:
								evn->u.createNotify.parent = Swap32(q->event.u.createNotify.parent);
								evn->u.createNotify.window = Swap32(q->event.u.createNotify.window);
								evn->u.createNotify.x = Swap16(q->event.u.createNotify.x);
								evn->u.createNotify.y = Swap16(q->event.u.createNotify.y);
								evn->u.createNotify.width = Swap16(q->event.u.createNotify.width);
								evn->u.createNotify.height = Swap16(q->event.u.createNotify.height);
								evn->u.createNotify.borderWidth = Swap16(q->event.u.createNotify.borderWidth);
								evn->u.createNotify.override = q->event.u.createNotify.override;
								break;
							
							case DestroyNotify:
								evn->u.destroyNotify.event = Swap32(q->event.u.destroyNotify.event);
								evn->u.destroyNotify.window = Swap32(q->event.u.destroyNotify.window);
								break;
							
							case UnmapNotify:
								evn->u.unmapNotify.event = Swap32(q->event.u.unmapNotify.event);
								evn->u.unmapNotify.window = Swap32(q->event.u.unmapNotify.window);
								evn->u.unmapNotify.fromConfigure = q->event.u.unmapNotify.fromConfigure;
								break;
							
							case MapNotify:
								evn->u.mapNotify.event = Swap32(q->event.u.mapNotify.event);
								evn->u.mapNotify.window = Swap32(q->event.u.mapNotify.window);
								evn->u.mapNotify.override = q->event.u.mapNotify.override;
								break;
							
							case MapRequest:
								evn->u.mapRequest.parent = Swap32(q->event.u.mapRequest.parent);
								evn->u.mapRequest.window = Swap32(q->event.u.mapRequest.window);
								break;
								
							case ReparentNotify:
								evn->u.reparent.event = Swap32(q->event.u.reparent.event);
								evn->u.reparent.window = Swap32(q->event.u.reparent.window);
								evn->u.reparent.parent = Swap32(q->event.u.reparent.parent);
								evn->u.reparent.x = Swap16(q->event.u.reparent.x);
								evn->u.reparent.y = Swap16(q->event.u.reparent.y);
								evn->u.reparent.override = q->event.u.reparent.override;
								break;
							
							case ConfigureNotify:
								evn->u.configureNotify.event = Swap32(q->event.u.configureNotify.event);
								evn->u.configureNotify.window = Swap32(q->event.u.configureNotify.window);
								evn->u.configureNotify.aboveSibling = Swap32(q->event.u.configureNotify.aboveSibling);
								evn->u.configureNotify.x = Swap16(q->event.u.configureNotify.x);
								evn->u.configureNotify.y = Swap16(q->event.u.configureNotify.y);
								evn->u.configureNotify.width = Swap16(q->event.u.configureNotify.width);
								evn->u.configureNotify.height = Swap16(q->event.u.configureNotify.height);
								evn->u.configureNotify.borderWidth = Swap16(q->event.u.configureNotify.borderWidth);
								evn->u.configureNotify.override = q->event.u.configureNotify.override;
								break;
							
							case ConfigureRequest:
								evn->u.configureRequest.parent = Swap32(q->event.u.configureRequest.parent);
								evn->u.configureRequest.window = Swap32(q->event.u.configureRequest.window);
								evn->u.configureRequest.sibling = Swap32(q->event.u.configureRequest.sibling);
								evn->u.configureRequest.x = Swap16(q->event.u.configureRequest.x);
								evn->u.configureRequest.y = Swap16(q->event.u.configureRequest.y);
								evn->u.configureRequest.width = Swap16(q->event.u.configureRequest.width);
								evn->u.configureRequest.height = Swap16(q->event.u.configureRequest.height);
								evn->u.configureRequest.borderWidth = Swap16(q->event.u.configureRequest.borderWidth);
								evn->u.configureRequest.valueMask = Swap16(q->event.u.configureRequest.valueMask);
								break;
							
							case GravityNotify:
								evn->u.gravity.event = Swap32(q->event.u.gravity.event);
								evn->u.gravity.window = Swap32(q->event.u.gravity.window);
								evn->u.gravity.x = Swap16(q->event.u.gravity.x);
								evn->u.gravity.y = Swap16(q->event.u.gravity.y);
								break;
							
							case ResizeRequest:
								evn->u.resizeRequest.window = Swap32(q->event.u.resizeRequest.window);
								evn->u.resizeRequest.width = Swap16(q->event.u.resizeRequest.width);
								evn->u.resizeRequest.height = Swap16(q->event.u.resizeRequest.height);
								break;
							
							case CirculateNotify:
							case CirculateRequest:
								evn->u.circulate.event = Swap32(q->event.u.circulate.event);
								evn->u.circulate.window = Swap32(q->event.u.circulate.window);
								evn->u.circulate.parent = Swap32(q->event.u.circulate.parent);
								evn->u.circulate.place = q->event.u.circulate.place;
								break;
							
							case PropertyNotify:
								evn->u.property.window = Swap32(q->event.u.property.window);
								evn->u.property.atom = Swap32(q->event.u.property.atom);
								evn->u.property.time = Swap32(q->event.u.property.time);
								evn->u.property.state = q->event.u.property.state;
								break;
							
							case SelectionClear:
								evn->u.selectionClear.time = Swap32(q->event.u.selectionClear.time);
								evn->u.selectionClear.window = Swap32(q->event.u.selectionClear.window);
								evn->u.selectionClear.atom = Swap32(q->event.u.selectionClear.atom);
								break;
							
							case SelectionRequest:
								evn->u.selectionRequest.time = Swap32(q->event.u.selectionRequest.time);
								evn->u.selectionRequest.owner = Swap32(q->event.u.selectionRequest.owner);
								evn->u.selectionRequest.requestor = Swap32(q->event.u.selectionRequest.requestor);
								evn->u.selectionRequest.selection = Swap32(q->event.u.selectionRequest.selection);
								evn->u.selectionRequest.target = Swap32(q->event.u.selectionRequest.target);
								evn->u.selectionRequest.property = Swap32(q->event.u.selectionRequest.property);
								break;
							
							case SelectionNotify:
								evn->u.selectionNotify.time = Swap32(q->event.u.selectionNotify.time);
								evn->u.selectionNotify.requestor = Swap32(q->event.u.selectionNotify.requestor);
								evn->u.selectionNotify.selection = Swap32(q->event.u.selectionNotify.selection);
								evn->u.selectionNotify.target = Swap32(q->event.u.selectionNotify.target);
								evn->u.selectionNotify.property = Swap32(q->event.u.selectionNotify.property);
								break;
							
							case ColormapNotify:
								evn->u.colormap.window = Swap32(q->event.u.colormap.window);
								evn->u.colormap.colormap = Swap32(q->event.u.colormap.colormap);
								evn->u.colormap.c_new = q->event.u.colormap.c_new;
								evn->u.colormap.state = q->event.u.colormap.state;
								break;
							
							case MappingNotify:
								evn->u.mappingNotify.request = q->event.u.mappingNotify.request;
								evn->u.mappingNotify.firstKeyCode = q->event.u.mappingNotify.firstKeyCode;
								evn->u.mappingNotify.count = q->event.u.mappingNotify.count;
								break;
							
							case ClientMessage:
								evn->u.clientMessage.window = Swap32(q->event.u.clientMessage.window);
								evn->u.clientMessage.u.l.type = Swap32(q->event.u.clientMessage.u.l.type);
								if (q->event.u.u.detail == 32)
								{
									evn->u.clientMessage.u.l.longs0 = Swap32(q->event.u.clientMessage.u.l.longs0);
									evn->u.clientMessage.u.l.longs1 = Swap32(q->event.u.clientMessage.u.l.longs1);
									evn->u.clientMessage.u.l.longs2 = Swap32(q->event.u.clientMessage.u.l.longs2);
									evn->u.clientMessage.u.l.longs3 = Swap32(q->event.u.clientMessage.u.l.longs3);
									evn->u.clientMessage.u.l.longs4 = Swap32(q->event.u.clientMessage.u.l.longs4);
								} else if (q->event.u.u.detail == 16)
								{
									evn->u.clientMessage.u.s.shorts0 = Swap16(q->event.u.clientMessage.u.s.shorts0);
									evn->u.clientMessage.u.s.shorts1 = Swap16(q->event.u.clientMessage.u.s.shorts1);
									evn->u.clientMessage.u.s.shorts2 = Swap16(q->event.u.clientMessage.u.s.shorts2);
									evn->u.clientMessage.u.s.shorts3 = Swap16(q->event.u.clientMessage.u.s.shorts3);
									evn->u.clientMessage.u.s.shorts4 = Swap16(q->event.u.clientMessage.u.s.shorts4);
									evn->u.clientMessage.u.s.shorts5 = Swap16(q->event.u.clientMessage.u.s.shorts5);
									evn->u.clientMessage.u.s.shorts6 = Swap16(q->event.u.clientMessage.u.s.shorts6);
									evn->u.clientMessage.u.s.shorts7 = Swap16(q->event.u.clientMessage.u.s.shorts7);
									evn->u.clientMessage.u.s.shorts8 = Swap16(q->event.u.clientMessage.u.s.shorts8);
									evn->u.clientMessage.u.s.shorts9 = Swap16(q->event.u.clientMessage.u.s.shorts9);
								} else
								{
									evn->u.clientMessage.u.l.longs0 = q->event.u.clientMessage.u.l.longs0;
									evn->u.clientMessage.u.l.longs1 = q->event.u.clientMessage.u.l.longs1;
									evn->u.clientMessage.u.l.longs2 = q->event.u.clientMessage.u.l.longs2;
									evn->u.clientMessage.u.l.longs3 = q->event.u.clientMessage.u.l.longs3;
									evn->u.clientMessage.u.l.longs4 = q->event.u.clientMessage.u.l.longs4;
								}
								break;
							
							case GenericEvent:
								evn->u.clientMessage.window = Swap32(q->event.u.clientMessage.window); /* xGenericEvent.length */
								evn->u.expose.x = Swap16(q->event.u.expose.x); /* xGenericEvent.evtype */
								evn->u.expose.y = Swap16(q->event.u.expose.y); /* xGenericEvent.pad2 */
								evn->u.clientMessage.u.l.longs0 = Swap32(q->event.u.clientMessage.u.l.longs0); /* xGenericEvent.pad3 */
								evn->u.clientMessage.u.l.longs1 = Swap32(q->event.u.clientMessage.u.l.longs1); /* xGenericEvent.pad4 */
								evn->u.clientMessage.u.l.longs2 = Swap32(q->event.u.clientMessage.u.l.longs2); /* xGenericEvent.pad5 */
								evn->u.clientMessage.u.l.longs3 = Swap32(q->event.u.clientMessage.u.l.longs3); /* xGenericEvent.pad6 */
								evn->u.clientMessage.u.l.longs4 = Swap32(q->event.u.clientMessage.u.l.longs4); /* xGenericEvent.pad7 */
								break;
							}
						}
						rcp->oBuf.Left += sizeof(xEvent);
						MAIN_FDSET_wr |= rcp->FdSet;
					}
				}
				lst++;
			}
		}
	}
}
