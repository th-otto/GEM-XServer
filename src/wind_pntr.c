//==============================================================================
//
// wind_pntr.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-09-28 - Initial Version.
//==============================================================================
//
#include "window_P.h"
#include "event.h"
#include "grph.h"
#include "Cursor.h"
#include "wmgr.h"
#include "x_gem.h"

#include <stdio.h>
#include <stdlib.h> // exit

#include <X11/Xproto.h>


#ifdef TRACE
#	define _TRACE_POINTER
#	define _TRACE_FOCUS
#endif


WINDOW * _WIND_PointerRoot = &WIND_Root;


#define BMOTION_MASK (Button1MotionMask|Button2MotionMask| \
                      Button3MotionMask|Button4MotionMask|Button5MotionMask)
#define ALL_MOTION_MASK (PointerMotionMask|ButtonMotionMask|BMOTION_MASK)


//------------------------------------------------------------------------------
#if defined(_TRACE_POINTER) || defined(_TRACE_FOCUS)
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

# define TRACEF(w,el,m) printf ("W:%X " el " %s \n", w->Id, _M2STR(m))

#else
# define TRACEF(w,el,m)
#endif

//==============================================================================
void
WindPointerWatch (BOOL movedNreorg)
{
	CARD8    evnt, next, last;
	GRECT    sect;
	WINDOW * stack[32], * widget = NULL;
	int      anc = 0, top, bot;
	short    hdl;
	PXY      e_p = // pointer coordinates relative to the event windows origin
			         *MAIN_PointerPos,
			   r_p = // pointer coordinates relative to the new pointer root origin
			         { MAIN_PointerPos->x - WIND_Root.Rect.x,
			           MAIN_PointerPos->y - WIND_Root.Rect.y };
	CARD32   r_id  = -1;
	short    focus = 0;
	
	BOOL watch = (0 != (WIND_Root.u.List.AllMasks & ALL_MOTION_MASK));
	
	/*--- flatten the leave path to the stack list ---*/
	
	if (!(*stack = _WIND_PointerRoot)) {
		// noting to leave
		e_p.x -= WIND_Root.Rect.x;
		e_p.y -= WIND_Root.Rect.y;
		stack[++anc] = &WIND_Root;
		
	} else while (1) {
		WINDOW * w = stack[anc];
		e_p.x -= w->Rect.x;
		e_p.y -= w->Rect.y;
		if (w->Parent) stack[++anc] = w->Parent;
		else           break;
	}
	top = anc;
	
	/*--- find the gem window the pointer is over ---*/
	
	if ((hdl = wind_find (MAIN_PointerPos->x, MAIN_PointerPos->y)) < 0) {
		// the pointer is outside of all windows, e.g. in the title bar
		
		puts("*PLONK*");
		sect  = WIND_Root.Rect;
		watch = xFalse;
		if (!anc || stack[anc-1]) stack[top] = NULL;
		else                      top        = anc -1;
		
	} else {
		r_id = (ROOT_WINDOW|hdl);
		if (hdl > 0) {
			stack[++top] = NULL;
			if (WMGR_OpenCounter) {
				WINDOW * w = WIND_Root.StackTop;
				if (!w) {
					printf ("\n\33pPANIC\33q"
					        " Inconsistency in window tree hirachy! (%i)\n",
					        WMGR_OpenCounter);
					exit (1);
				}
				do if (w->Handle == hdl) {
					// over x window
					if (w->isMapped) {
						watch |= (0 != (w->u.List.AllMasks & ALL_MOTION_MASK));
						r_p.x -= w->Rect.x;
						r_p.y -= w->Rect.y;
						stack[top] = w;
						focus      = hdl;
					}
					break;
				} while ((w = w->PrevSibl));
			}
			// else over some gem window
			
			if (stack[anc-1] == stack[top]) {
				top = --anc;
			}
		}
		// else (hdl == 0), over root window
		
		if (WMGR_Focus != focus) {
			WmgrSetFocus (focus);
		}
		
		/*--- find the windows section the pointer is in ---*/
		
		if (!stack[top]  &&  hdl == wind_get_top()) {
			// over the top gem window, can be watched including widgets
			
			wind_get_curr (hdl, &sect);
			watch = xFalse;
			
		} else {
			// find section
			wind_get_first (hdl, &sect);
			while (sect.w && sect.h && !PXYinRect (MAIN_PointerPos, &sect)) {
				wind_get_next (hdl, &sect);
			}
			if (!sect.w || !sect.h) {
				// the pointer is outside of all work areas, so it's over a gem widget
				
				*(PXY*)&sect = *MAIN_PointerPos;
				sect.w = sect.h = 1;
				watch = xFalse;
				
				if (!hdl) {
					top++;
				} else if (stack[top]  &&  anc == top) {
					stack[++anc] = &WIND_Root;
					top = anc +1;
				}
				if (!anc || stack[anc-1]) stack[top] = NULL;
				else                      top        = anc -1;
			
			} else if (hdl && stack[top]) {
				
				/*--- find the subwindow and its section the pointer is in ---*/
				
				GRECT o = // the origin of the window, absolute coordinate
				          { WIND_Root.Rect.x + stack[top]->Rect.x,
				            WIND_Root.Rect.y + stack[top]->Rect.y,
				            stack[top]->Rect.w, stack[top]->Rect.h };
				
				if (r_p.x < 0  ||  r_p.y < 0  ||  r_p.x >= o.w  ||  r_p.y >= o.h) {
					// the pointer is over the border of the main window
					
					WINDOW * w = stack[top];
					short    b = (w->GwmDecor ? WMGR_Decor : 0);
					if (b) widget = w;
					else   b      = w->BorderWidth;
					if      (r_p.x <  0)   { o.x -= b;   o.w = b;   }
					else if (r_p.x >= o.w) { o.x += o.w; o.w = b;   }
					if      (r_p.y <  0)   { o.y -= b;   o.h = b;   }
					else if (r_p.y >= o.h) { o.y += o.h; o.h = b;   }
					GrphIntersect (&sect, &o);
					watch = xFalse;
					if (stack[anc-1]) {
						top = anc;
						stack[++top] = NULL;
					} else {
						top = --anc;
					}
				
				} else {
					WINDOW * w = stack[top]->StackTop;
					while (w) {
						if (w->isMapped) {
							GRECT r = { 0, 0, o.w, o.h };
							if (GrphIntersect (&r, &w->Rect) && PXYinRect (&r_p, &r)) {
								if (anc &&  w == stack[anc-1]) {
									top = --anc;
								} else if (top == numberof(stack)) {
									break;
								} else {
									stack[++top] = w;
								}
								watch |= (0 != (w->u.List.AllMasks & ALL_MOTION_MASK));
								// set origin to inferiors value, and watching rectangle
								o.x += w->Rect.x;
								o.y += w->Rect.y;
								o.w =  r.w;
								o.h =  r.h;
								// update relative pointer position for new origin
								r_p.x -= w->Rect.x;
								r_p.y -= w->Rect.y;
								w   =  w->StackTop;
								continue;
							}
						}
						w = w->PrevSibl;
					}
					r_id = stack[top]->Id;
					GrphIntersect (&sect, &o);
					if ((w = stack[top]->StackBot)) {
						sect.w += sect.x;
						sect.h += sect.y;
						do {
							int q;
							if        ((q =  w->Rect.x) >  r_p.x) {
									 if (q <= sect.w - o.x) sect.w = q + o.x;
							} else if ((q += w->Rect.w) <= r_p.x) {
									 if (q >  sect.x - o.x) sect.x = q + o.x;
							} else if ((q =  w->Rect.y) >  r_p.y) {
									 if (q <= sect.h - o.y) sect.h = q + o.y;
							} else if ((q += w->Rect.h) <= r_p.y) {
									 if (q >  sect.y - o.y) sect.y = q + o.y;
							}
						} while ((w = w->NextSibl));
						sect.w -= sect.x;
						sect.h -= sect.y;
					}
				}
			}
		}
	}
	_WIND_PointerRoot = stack[top];
	if (WMGR_OpenCounter) {
		MainSetWatch (&sect, MO_LEAVE);
		MainSetMove  (watch);
	} else {
		MainClrWatch ();
		MainSetMove  (xFalse);
	}
	
	if (!widget  &&  stack[0] == stack[top]) {
		if (WMGR_Cursor) {
			WmgrCursorOff (NULL);
		}
		if (movedNreorg && stack[top]) {
			WindPointerMove (&r_p);
		}
		return;
	}
	
	/*--- generate events ---*/
	
	EvntPointer (stack, anc, top, e_p, r_p, r_id, NotifyNormal);
	
	if (anc == 0) {
		evnt = NotifyInferior;
		next = NotifyVirtual;
		last = NotifyAncestor;
	} else if (anc == top) {
		evnt = NotifyAncestor;
		next = NotifyVirtual;
		last = NotifyInferior;
	} else {
		evnt = NotifyNonlinear;
		next = NotifyNonlinearVirtual;
		last = NotifyNonlinear;
	}
	
	// notify in events
	
	if (stack[0]) {
		if (stack[0]->u.List.AllMasks & FocusChangeMask) {
			TRACEF (stack[0], "FocusOut", evnt);
			EvntFocusOut (stack[0], NotifyNormal, evnt);
		}
	}
	for (bot = 1; bot < anc; ++bot) {
		if (stack[bot]->u.List.AllMasks & FocusChangeMask) {
			TRACEF (stack[bot], "FocusOut", next);
			EvntFocusOut (stack[bot], NotifyNormal, next);
		}
	}
	
	if (widget && !_WIND_PgrabWindow) {
		if (WMGR_Active) {
			WmgrCursor (widget, &r_p);
		}
	
	} else { // notify leave/out events
		for (bot = anc +1; bot < top; ++bot) {
			if (stack[bot]->u.List.AllMasks & FocusChangeMask) {
				TRACEF (stack[bot], "FocusIn", next);
				EvntFocusIn (stack[bot], NotifyNormal, next);
			}
		}
		if (stack[top]) {
			if (stack[top]->u.List.AllMasks & FocusChangeMask) {
				TRACEF (stack[top], "FocusIn", last);
				EvntFocusIn (stack[top], NotifyNormal, last);
			}
		}
		
		if (!_WIND_PgrabWindow) {
			if (WMGR_Cursor) WmgrCursorOff (NULL);
			/*else*/         _Wind_Cursor  (stack[top]);
		}
	}
}

//==============================================================================
void
WindPointerMove (const p_PXY pointer_xy)
{
	PXY r_xy, e_xy;
	WINDOW * wind = _WIND_PointerRoot;
	CARD32   r_id;
	CARD32 msk =  MAIN_KeyButMask & BMOTION_MASK;
	       msk |= (msk ? PointerMotionMask|ButtonMotionMask : PointerMotionMask);
	
	if (!wind) {
		puts("*BANG*");
		MainSetMove (xFalse);
		return;
	}
	if (pointer_xy) {   // called internal, not from main
		r_xy = *pointer_xy;
	
	} else if (wind) {
		r_xy = *MAIN_PointerPos;
		do {
			r_xy.x -= wind->Rect.x;
			r_xy.y -= wind->Rect.y;
		} while ((wind = wind->Parent));
		wind = _WIND_PointerRoot;
	}
	e_xy = r_xy;
	r_id = wind->Id;
	
	do {
		if (msk & wind->u.List.AllMasks) {
			EvntMotionNotify (wind, msk, r_id, r_id, r_xy, e_xy);
		#	ifdef _TRACE_POINTER
			printf ("moved on W:%X [%i,%i] \n", wind->Id, e_xy.x, e_xy.y);
		#	endif
			msk &= !wind->u.List.AllMasks;
		}
		if (msk &= wind->PropagateMask) {
			e_xy.x += wind->Rect.x;
			e_xy.y += wind->Rect.y;
		} else {
			break;
		}
	} while ((wind = wind->Parent));
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_QueryPointer (CLIENT * clnt, xQueryPointerReq * q)
{
	// Returns the ponter coordinates relative to the root window and to the
	// given window.
	//
	// CARD32 id: requested window
	//
	// Reply:
	// BOOL   sameScreen:
	// Window root:
	// Window child:
	// INT16  rootX, rootY:
	// INT16  winX,  winY:
	// CARD16 mask:
	//...........................................................................
	
	WINDOW * wind;
	GRECT    work;
	
	if (!(wind = WindFind(q->id)) && !wind_get_work (q->id & 0x7FFF, &work)) {
		Bad(Window, q->id, QueryPointer,);
	
	} else { //..................................................................
		
		ClntReplyPtr (QueryPointer, r,);
		PXY pos = WindPointerPos (NULL);
		
		DEBUG (QueryPointer," W:%lX", q->id);
		
		r->root  = ROOT_WINDOW;
		r->rootX = pos.x;
		r->rootY = pos.y;
		r->mask  = MAIN_KeyButMask;
		r->child = None;
		
		if (!PXYinRect (MAIN_PointerPos, &WIND_Root.Rect)) {
			r->sameScreen = xFalse;
			r->winX       = 0;
			r->winY       = 0;
		
		} else {
			r->sameScreen = xTrue;
			
			if (!wind) {
				pos.x = MAIN_PointerPos->x - work.x;
				pos.y = MAIN_PointerPos->y - work.y;
			
			} else if (wind == &WIND_Root) {
				short hdl = wind_find (MAIN_PointerPos->x, MAIN_PointerPos->y);
				if (hdl > 0) {
					r->child = ROOT_WINDOW|hdl;
					if ((wind = wind->StackTop)) {
						do if (wind->isMapped  &&  wind->Handle == hdl) {
							r->child = wind->Id;
							break;
						} while ((wind = wind->PrevSibl));
					}
				}
			} else {
				pos    = WindPointerPos (wind);
				work.x = work.y = 0;
				work.w = wind->Rect.w;
				work.h = wind->Rect.h;
				if (PXYinRect (&pos, &work) && (wind = wind->StackTop)) {
					do if (wind->isMapped && PXYinRect (&pos, &wind->Rect)) {
						r->child = wind->Id;
						break;
					} while ((wind = wind->PrevSibl));
				}
			}
			r->winX = pos.x;
			r->winY = pos.y;
		}
		ClntReply (QueryPointer,, "wwPP.");
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_TranslateCoords (CLIENT * clnt, xTranslateCoordsReq * q)
{
	// Takes the source coordinates relative to the source window and returns
	// them relative to the destination window.
	//
	// Window srcWid:
	// Window dstWid:
	// INT16 srcX, srcY:
	//
	// Reply:
	// BOOL sameScreen:
	// Window child:
	// INT16 dstX, dstY:
	//...........................................................................
	
	WINDOW * wsrc = NULL, * wdst = NULL;
	BOOL     ok   = xTrue;
	PXY      p_src, p_dst;
	
	if (q->srcWid & ~RID_MASK) {
		if ((ok = ((wsrc = WindFind(q->srcWid)) != NULL))) {
			p_src = WindOrigin (wsrc);
		}
	} else {
		GRECT work;
		if ((ok = wind_get_work (q->srcWid & 0x7FFF, &work))) {
			p_src.x = work.x - WIND_Root.Rect.x;
			p_src.y = work.y - WIND_Root.Rect.y;
		}
	}
	if (!ok) {
		Bad(Window, q->srcWid, TranslateCoords,"(): source not found.");
	
	} else if (q->dstWid & ~RID_MASK) {
		if ((ok = ((wdst = WindFind(q->dstWid)) != NULL))) {
			p_dst = WindOrigin (wdst);
		}
	} else {
		GRECT work;
		if ((ok = wind_get_work (q->dstWid & 0x7FFF, &work))) {
			p_dst.x = work.x - WIND_Root.Rect.x;
			p_dst.y = work.y - WIND_Root.Rect.y;
		}
	}
	if (!ok) {
		Bad(Window, q->dstWid, TranslateCoords,"(): destination not found.");
	
	} else { //..................................................................
		
		ClntReplyPtr (TranslateCoords, r,);
		
		DEBUG (TranslateCoords," %i/%i for W:%lX on W:%lX",
		       q->srcX, q->srcY, q->dstWid, q->srcWid);
		
		r->sameScreen = xTrue;
		r->dstX       = q->srcX + p_src.x - p_dst.x;
		r->dstY       = q->srcY + p_src.y - p_dst.y;
		r->child      = None;
		
		if (wdst == &WIND_Root) {
			short hdl = wind_find (MAIN_PointerPos->x, MAIN_PointerPos->y);
			if (hdl > 0) {
				r->child = ROOT_WINDOW|hdl;
				if ((wdst = wdst->StackTop)) {
					do if (wdst->isMapped  &&  wdst->Handle == hdl) {
						r->child = wdst->Id;
						break;
					} while ((wdst = wdst->PrevSibl));
				}
			}
		} else if (wdst) {
			if (r->dstX >= 0  &&  r->dstX < wdst->Rect.w &&
		       r->dstY >= 0  &&  r->dstY < wdst->Rect.h &&
		       (wdst = wdst->StackTop)) {
				p_dst = *(PXY*)&r->dstX;
				do if (wdst->isMapped && PXYinRect (&p_dst, &wdst->Rect)) {
					r->child = wdst->Id;
					break;
				} while ((wdst = wdst->PrevSibl));
			}
		}
		ClntReply (TranslateCoords,, "wP");
	}
}

//------------------------------------------------------------------------------
void
RQ_WarpPointer (CLIENT * clnt, xWarpPointerReq * q)
{
	PRINT (- X_WarpPointer," from W:%lX [%i,%i/%u,%u] to W:%lX %i/%i",
	       q->srcWid, q->srcX, q->srcY, q->srcWidth, q->srcHeight, q->dstWid,
	       q->dstX, q->dstY);
}
