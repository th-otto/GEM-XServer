//==============================================================================
//
// window.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-06-05 - Initial Version.
//==============================================================================
//
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "window_P.h"
#include "x_gem.h"
#include "wmgr.h"
#include "event.h"
#include "pixmap.h"
#include "gcontext.h"
#include "Property.h"
#include "Atom.h"
#include "selection.h"
#include "Cursor.h"
#include "grph.h"
#include "colormap.h"

#include <X11/Xatom.h>


WINDOW WIND_Root = {
	{NULL}, xTrue, ROOT_WINDOW,   // NULL,
	{ 0,0, 0,0 }, 0u, 0, ROOT_DEPTH,
	xTrue, xTrue, NotUseful, CenterGravity, ForgetGravity, xFalse,
	xTrue, xFalse, xFalse, xFalse,  xFalse, xFalse, xFalse,   0u,
	G_BLACK, {G_WHITE},   NoEventMask, {{ NoEventMask, NULL }},
	NULL, NULL, NULL,NULL, NULL,NULL, NULL, NULL
};

static short _MAP_Inc, _MAP_IncX, _MAP_IncY;
static short _WIND_RootX2, _WIND_RootY2;

BOOL WIND_ChngTrigger = xFalse;


//==============================================================================
void
WindInit (BOOL initNreset)
{
	if (initNreset) {
		wind_get_work (0, &WIND_Root.Rect);
		_MAP_Inc = WIND_Root.Rect.g_y -1;
		_WIND_RootX2 = WIND_Root.Rect.g_x + WIND_Root.Rect.g_w -1;
		_WIND_RootY2 = WIND_Root.Rect.g_y + WIND_Root.Rect.g_h -1;
		
	} else {
		if (WIND_Root.StackBot) {
			WINDOW * w = WIND_Root.StackBot;
			x_printf ("\033pFATAL\033q root window has still children!\n");
			while (w) {
				char w_f[16] = "(-)", w_l[16] = "(-)";
				if (w->StackBot) sprintf (w_f, "W:%X", w->StackBot->Id);
				if (w->StackTop) sprintf (w_l, "W:%X", w->StackTop->Id);
				x_printf ("  W:%X -> %s .. %s %s\n",
				        w->Id, w_f, w_l, (w == WIND_Root.StackTop ? "= Last" : ""));
				w = w->NextSibl;
			}
			exit (1);
		}
		if (WIND_Root.u.List.AllMasks) {
			x_printf ("\033pFATAL\033q root window event list not empty!\n");
			exit (1);
		}
		if (WIND_Root.Properties) {
				PropDelete (&WIND_Root.Properties);
#if 0
			x_printf ("  remove Propert%s:",
			        (WIND_Root.Properties->Next ? "ies" : "y"));
			while (WIND_Root.Properties) {
				x_printf (" '%s'(%s)",
				        ATOM_Table[WIND_Root.Properties->Name]->Name,
				        ATOM_Table[WIND_Root.Properties->Type]->Name);
				if (WIND_Root.Properties->Type == XA_STRING) {
					x_printf ("='%s'", WIND_Root.Properties->Data);
				}
				PropDelete (&WIND_Root.Properties);
			}
			x_printf (",\n");
#endif
		}
		if (WIND_Root.Cursor) {
			CrsrFree (WIND_Root.Cursor, NULL);
			WIND_Root.Cursor = NULL;
		}
		if (WIND_Root.hasBackGnd) {
			WmgrSetDesktop (xFalse);
			if (WIND_Root.hasBackPix) {
				WIND_Root.hasBackPix = xFalse;
				PmapFree (WIND_Root.Back.Pixmap, NULL);
			}
			WIND_Root.hasBackGnd = xFalse;
			WIND_Root.Back.Pixel = G_WHITE;
		}
	}
	_MAP_IncX = 0;
	_MAP_IncY = 0;
}


//==============================================================================
void
WindCleanup (CLIENT * clnt)
{
	WINDOW * wind = &WIND_Root;
	BOOL     b    = xTrue;
	do {
		if (b) {
			p_BTNGRAB * pGrab = &wind->ButtonGrab;
			while (*pGrab) {
				if ((*pGrab)->Client == clnt) {
					_Wind_PgrabRemove (pGrab);
					if (!clnt->EventReffs) break;
				} else {
					pGrab = &(*pGrab)->Next;
				}
			}
			if (wind->u.List.AllMasks) {
				EvntClr (wind, clnt);
				if (!clnt->EventReffs) break;
			}
			if (wind->StackBot) {
				wind = wind->StackBot;
				continue;
			}
		}
		if (wind->NextSibl) {
			wind = wind->NextSibl;
			b    = xTrue;
			continue;
		} else {
			wind = wind->Parent;
			b    = xFalse;
		}
	} while (wind);
}


//==============================================================================
BOOL
WindButton (CARD16 prev_mask, int count)
{
	WINDOW * wind = _WIND_PointerRoot;
	CARD32   w_id = 0;
	PXY      r_xy = WindPointerPos (NULL);
	PXY      w_xy;
	CARD8    butt_r = (prev_mask & Button1Mask ? Button1 :
	                   prev_mask & Button2Mask ? Button2 :
	                   prev_mask & Button3Mask ? Button3 :
	                   None);
	CARD8    butt_p = (MAIN_KeyButMask & Button1Mask ? Button1 :
	                   MAIN_KeyButMask & Button2Mask ? Button2 :
	                   MAIN_KeyButMask & Button3Mask ? Button3 :
	                   None);
	
	//>>>>> for debugging: Alt+RightButton gives  window informations
	//
	if ((MAIN_KeyButMask & K_ALT) && (MAIN_KeyButMask & Button2Mask) && wind) {
		short dmy;
		x_printf ("\nW:%X 0x%lX #%i [%i,%i/%i,%i/%i] * %i \n",
		        wind->Id, wind->u.Event.Mask, wind->Handle,
		        wind->Rect.g_x, wind->Rect.g_y, wind->Rect.g_w, wind->Rect.g_h,
		        wind->BorderWidth, wind->Depth);
		if (wind->hasBorder || wind->hasBackGnd) {
			if (wind->hasBorder) {
				x_printf ("border = %li   ", wind->BorderPixel);
			}
			if (wind->hasBackPix) {
				x_printf ("backgnd: P:%X [%i,%i] * %i", wind->Back.Pixmap->Id,
				        wind->Back.Pixmap->W, wind->Back.Pixmap->H,
				        wind->Back.Pixmap->Depth);
			} else if (wind->hasBackGnd) {
				x_printf ("backgnd: %li", wind->Back.Pixel);
			}
			x_printf ("\n");
		}
		if (wind->Parent) {
			x_printf("parent: W:%X", wind->Parent->Id);
			if (wind->PrevSibl) x_printf ("   prev: W:%X", wind->PrevSibl->Id);
			if (wind->NextSibl) x_printf ("   next: W:%X", wind->NextSibl->Id);
			x_printf ("\n");
		}
		evnt_button (1, 2, 0, &dmy, &dmy, &dmy, &dmy);
		return xTrue;
	}
	//<<<<< end debugging
	
	if (!_WIND_PgrabWindow && wind) {
		if (WMGR_Active && (MAIN_KeyButMask & K_ALT)) {
			while (wind->Handle < 0) {
				wind = wind->Parent;
			}
			return WmgrButton (wind);
		}
		
		if (butt_r) {
			EvntPropagate (wind, ButtonReleaseMask, ButtonRelease,
			               wind->Id, r_xy, WindPointerPos (wind), butt_r);
			if (!butt_p) {
				return xFalse;
			}
			butt_r = None;
		}
		if (butt_p) {
			CLIENT * clnt;
			#define MASK (ButtonPressMask|ButtonReleaseMask)
			if (!_Wind_PgrabMatch (wind, butt_p, MAIN_Key_Mask)
			    &&  (wind->u.List.AllMasks & MASK) == MASK
			    &&  (clnt = EvntClient (wind, MASK))) {
				_Wind_PgrabSet (clnt, wind, NULL,
				                wind->u.List.AllMasks, xTrue, MAIN_TimeStamp, xTrue);
			}
			#undef MASK
		}
	}
	
	if (_WIND_PgrabWindow) {
		if (WMGR_OpenCounter && wind) {
			w_id = wind->Id;

		} else {
			int hdl = wind_find (MAIN_PointerPos->p_x, MAIN_PointerPos->p_y);
			if (hdl >= 0) {
				w_id = hdl | ROOT_WINDOW;
			}
		}
		if (!w_id) {
			if (_WIND_PgrabPassive && butt_r) {
				_Wind_PgrabClr (NULL);
			}
			return xFalse;
			
		} else {
			if (!wind) {
				wind = &WIND_Root;
			}
			if (butt_r) {
				WINDOW * wnd_r = NULL;
			   if ((!_WIND_PgrabOwnrEv ||
			        !(wnd_r = EvntSearch (wind, _WIND_PgrabClient,
			                              ButtonReleaseMask)))
			       && (_WIND_PgrabEvents & ButtonReleaseMask)) {
					wnd_r = _WIND_PgrabWindow;
				}
				if (wnd_r) {
					w_xy = WindPointerPos (wnd_r);
					EvntKeyButMotion (_WIND_PgrabClient, ButtonRelease,
					                  wnd_r->Id, WindChildOf (wnd_r, wind),
					                  r_xy, w_xy, butt_r);
				}
				if (_WIND_PgrabPassive) {
					_Wind_PgrabClr (NULL);
					butt_r = None;
					w_id   = None;
				}
				if (!butt_p) return xFalse;
			}
		}
		if (w_id && butt_p) {
			WINDOW * wnd_p = NULL;
			CARD32   c_id  = None;
			if ((!_WIND_PgrabOwnrEv ||
			        !(wnd_p = EvntSearch (wind, _WIND_PgrabClient,
			                              ButtonPressMask)))
			    && (_WIND_PgrabEvents & ButtonPressMask)) {
				wnd_p = _WIND_PgrabWindow;
			}
			if (wnd_p) {
				c_id = WindChildOf (wnd_p, wind);
				w_xy = WindPointerPos (wnd_p);
				EvntKeyButMotion (_WIND_PgrabClient, ButtonPress,
				                  wnd_p->Id, c_id, r_xy, w_xy, butt_p);
			}
			if (count > 1) {
				WINDOW * wnd_r = NULL;
				if ((!_WIND_PgrabOwnrEv ||
				        !(wnd_r = EvntSearch (wind, _WIND_PgrabClient,
				                              ButtonReleaseMask)))
				    && (_WIND_PgrabEvents & ButtonReleaseMask)) {
					wnd_r = _WIND_PgrabWindow;
				}
				if (wnd_r == wnd_p) {
					EvntKeyButMotion (_WIND_PgrabClient, ButtonRelease,
					                  wnd_r->Id, c_id, r_xy, w_xy, butt_p);
				} else if (wnd_r) {
					PXY e_xy = WindPointerPos (wnd_r);
					EvntKeyButMotion (_WIND_PgrabClient, ButtonRelease,
					                  wnd_r->Id, WindChildOf (wnd_r, wind),
					                  r_xy, e_xy, butt_p);
				}
				if (wnd_p) {
					EvntKeyButMotion (_WIND_PgrabClient, ButtonPress,
					                  wnd_p->Id, c_id, r_xy, w_xy, butt_p);
				}
			}
			return xFalse;
		}
	}
	
	if (!wind) {
		return WmgrButton (NULL);
	}
	
	if (butt_p) {
		w_xy = WindPointerPos (wind);
		EvntPropagate (wind, ButtonPressMask, ButtonPress,
		               wind->Id, r_xy, w_xy, butt_p);
		if (count > 1) {
			EvntPropagate (wind, ButtonReleaseMask, ButtonRelease,
			               wind->Id, r_xy, w_xy, butt_r);
			EvntPropagate (wind, ButtonPressMask, ButtonPress,
			               wind->Id, r_xy, w_xy, butt_p);
		}
	}
	
	return xFalse;
}


//------------------------------------------------------------------------------
static void
_Wind_setup (CLIENT * clnt, WINDOW * w, CARD32 mask, CARD32 * val, CARD8 req)
{
	if (clnt->DoSwap) {
		CARD32   m = mask;
		CARD32 * p = val;
//		int      i = 0;
		while (m) {
			if (m & 1) {
				*p = Swap32(*p);
				p++;
//				i++;
			}
			m >>= 1;
		}
//		if (i) PRINT (0,"+-\n          (%i):", i);
//	
//	} else if (mask) {
//		CARD32 m = mask;
//		int    i = 0;
//		while (m) {
//			if (m & 1) i++;
//			m >>= 1;
//		}
//		PRINT (0,"+-\n          (%i):", i);
//	
//	} else {
//		return; // mask == 0, nothing to do
	}
	
	if (mask & CWBackPixmap) {
		if (*val != None  &&  *val != ParentRelative) {
			PIXMAP * pmap = PmapFind (*val);
			if (!pmap) {
//				PRINT(0," ");
				Bad(BadPixmap, *val, req, "_          invalid backbground.");
				return;
			} else if (pmap->Depth != w->Depth) {
//				PRINT(0," ");
				Bad(BadMatch,0, req, "_          background depth %u not %u.",
				                  pmap->Depth, w->Depth);
				return;
			} else {
//				PRINT (0,"+- bpix=%lX", *val);
				w->Back.Pixmap = PmapShare(pmap);
				w->hasBackPix  = xTrue;
				w->hasBackGnd  = xTrue;
			}
		} else {
//			PRINT (0,"+- bpix=<none>");
		}
		val++;
	}
	if (mask & CWBackPixel) {
		// overrides prev
//		PRINT (0,"+- bgnd=%lu", *val);
		w->Back.Pixel = CmapPixelIdx (*(val++), w->Depth);
		w->hasBackGnd = xTrue;
	}
	if (mask & CWBorderPixmap) {
//		PRINT (0,"+- fpix=P:%lX", *val);
		val++;
	}
	if (mask & CWBorderPixel) {
		// overrides pref
//		PRINT (0,"+- fgnd=%lu", *val);
		w->BorderPixel = CmapPixelIdx (*(val++), w->Depth);
		w->hasBorder   = xTrue;
	}
	if (mask & CWBitGravity) {
//		PRINT (0,"+- bgrv=%u", (CARD8)*val);
		w->BitGravity = *(val++);
	}
	if (mask & CWWinGravity) {
//		PRINT (0,"+- wgrv=%u", (CARD8)*val);
		w->WinGravity = *(val++);
	}
	if (mask & CWBackingStore) {
//		PRINT (0,"+- bstr=%u", (CARD8)*val);
		w->BackingStore = *(val++);
	}
	if (mask & CWBackingPlanes) {
//		PRINT (0,"+- bpln=%lX", *val);
		val++;
	}
	if (mask & CWBackingPixel) {
//		PRINT (0,"+- bpix=%lX", *val);
		val++;
	}
	if (mask & CWOverrideRedirect) {
//		PRINT (0,"+- rdir=%u", (CARD8)*val);
		w->Override = *(val++);
	}
	if (mask & CWSaveUnder) {
//		PRINT (0,"+- save=%u", (CARD8)*val);
		w->SaveUnder = *(val++);
	}
	if (mask & CWEventMask) {
		CARD32 evnt = *val & AllEventMask;
		if (evnt) {
//			PRINT (0,"+- evnt=%lX", evnt);
			EvntSet (w, clnt, evnt);
//			#define __V(e) if(evnt & e##Mask) PRINT (0,"+-|" #e);
//			__V(ResizeRedirect)
//			__V(SubstructureRedirect)
//			__V(VisibilityChange)
			/*
			__V(ButtonPress)
			__V(ButtonRelease)
			__V(FocusChange)
			__V(KeyPress)
			__V(KeyRelease)
			__V(KeymapState)
			__V(Exposure)
			__V(PropertyChange)
			__V(ColormapChange)
			__V(OwnerGrabButton)
			*/
		} else if (w->u.List.AllMasks) {
//			PRINT (0,"+- evnt=<remove>");
			EvntClr (w, clnt);
		}
		val++;
	}
	if (mask & CWDontPropagate) {
		w->PropagateMask = ~*(val++) & AllEventMask;
//		PRINT (0,"+- dprp=%lX", ~w->PropagateMask & AllEventMask);
	}
	if (mask & CWColormap) {
//		PRINT (0,"+- cmap=M:%lX", (CARD32)*val);
		val++;
	}
	if (mask & CWCursor) {
//		PRINT (0,"+- crsr=");
		if (*val != None) {
			if (!(w->Cursor = CrsrGet (*val))) {
//				PRINT (0,"+-<invalid>");
			} else {
//				PRINT (0,"+-%lX", (CARD32)*val);
			}
		} else {
//			PRINT (0,"+-<none>");
		}
		val++;
	}
}


//==============================================================================
BOOL
WindSetMapped (WINDOW * wind, BOOL visible)
{
	BOOL redraw    = xFalse;
	wind->isMapped = xTrue;
	
	EvntMapNotify (wind, wind->Id, wind->Override);
	
	if (visible) {
		WINDOW * w = wind;
		BOOL     b = xTrue;
		while (1) {
			if (b) {
				if (w->isMapped && w->ClassInOut) {
					redraw = xTrue;
					if (w->u.List.AllMasks & VisibilityChangeMask) {
						EvntVisibilityNotify (w, VisibilityUnobscured);
					}
					if (w->StackTop) {
						w = w->StackTop;
						continue;
					}
				}
				if (w == wind) {
					break;
				}
			}
			if (w->PrevSibl) {
				w = w->PrevSibl;
				b = xTrue;
			} else if ((w = w->Parent) == wind) {
				break;
			} else {
				b = xFalse;
			}
		}
	}
	return redraw;
}

//==============================================================================
void
WindClrMapped (WINDOW * wind, BOOL by_conf)
{
	wind->isMapped = xFalse;
	
	EvntUnmapNotify (wind, wind->Id, by_conf);
}


//------------------------------------------------------------------------------
static void
_Wind_Unmap (WINDOW * wind, BOOL visible, BOOL ptr_check)
{
	BOOL saved = (wind->Id == _WIND_SaveUnder);
	
	if (wind->Handle > 0) {
		WmgrWindUnmap (wind, xFalse);
	
	} else {
		WindClrMapped (wind, xFalse);
		if (visible) {
			GRECT sect;
			WindGeometry (wind, &sect, wind->BorderWidth);
			if (!saved) {
				WindDrawSection (wind->Parent, &sect);
			}
			ptr_check &= PXYinRect (MAIN_PointerPos, &sect);
		} else {
			ptr_check = xFalse;
		}
	}
	
	if (wind == _WIND_PgrabWindow) {
		WINDOW * stack[] = { wind, wind->Parent };
		PXY r_xy = WindPointerPos (wind);
		EvntPointer (stack, 1, 1, r_xy, r_xy, wind->Id, NotifyUngrab);
		_Wind_PgrabClr (NULL);
	}
	
	if (saved)     WindSaveFlush (xTrue);
	if (ptr_check) WindPointerWatch (xFalse);
}


//==============================================================================
void
WindDelete (WINDOW * wind, CLIENT * clnt)
{
	BOOL     enter = xFalse;
	WINDOW * pwnd  = wind->Parent, * next = NULL;
	WINDOW * bott  = wind->Parent; // window below, not to be deleted
	WINDOW * chck;                 // either grab window or pointer root
	CARD8    mode;
	CLIENT * owner;
	CARD16   n     = 0;
	
	if (_WIND_PgrabWindow) {
		chck = _WIND_PgrabWindow;
		mode = NotifyUngrab;
	} else {
		chck = _WIND_PointerRoot;
		mode = NotifyNormal;
	}
	
	if (clnt && pwnd->u.List.AllMasks) {
		EvntClr (pwnd, clnt);
	}
	
	while(1) {	
		if (clnt) {
			if (wind->u.List.AllMasks) {
				EvntClr (wind, clnt);
			}
			
			if (!RID_Match (clnt->Id, wind->Id)) {
				// this point can't be reached in the very first loop (n == 0),
				// else the calling function has a serious bug
				
				next = wind->NextSibl;
				if (RID_Match (clnt->Id, pwnd->Id)) {
					
					// reparent window, move wind to closest anchestor
				
				}
				if ((wind = next)) {
					continue;
				
				} else {
					// that's save, if pwnd will be deleted, wind was moved lower in
					// the hirachy
					wind = pwnd;
					pwnd = pwnd->Parent;
					n--;
				}
			}
		}
		if (wind->StackBot) {
			pwnd = wind;
			wind = wind->StackBot;
			n++;
			continue;
		}
		
		// now wind is guaranteed to have no childs anymore and aren't be
		// excluded from deleting
		
		if (clnt && !RID_Match (clnt->Id, wind->Id)) {
			x_printf ("\033pPANIC\033q wind_destroy()"
			        " W:%X must be excluded from deleting!\n", wind->Id);
			exit(1);
		}
		if (wind->StackBot || wind->StackTop) {
			x_printf ("\033pPANIC\033q wind_destroy()"
			        " W:%X has still children: bot=W:%X top=W:%X!\n",
			        wind->Id, (wind->StackBot ? wind->StackBot->Id : 0),
			        (wind->StackTop ? wind->StackTop->Id : 0));
			exit(1);
		}
		
		if (wind == chck) {
			WINDOW * stack[32], * w = wind;
			PXY r_xy = WindPointerPos (wind);
			int anc  = 0;
			while (w != bott) {
				if      (w->Handle > 0) WmgrWindUnmap (w, xTrue);
				else if (w->isMapped)   WindClrMapped (w, xFalse);
				stack[anc++] = w;
				w            = w->Parent;
			}
			stack[anc] = bott;
			EvntPointer (stack, anc, anc, r_xy, r_xy, wind->Id, mode);
			if (mode == NotifyUngrab) {
				_Wind_PgrabClr (NULL);
			}
			_WIND_PointerRoot = bott;
			enter             = xTrue;
		
		} else {
			if      (wind->Handle > 0) WmgrWindUnmap (wind, xTrue);
			else if (wind->isMapped)   WindClrMapped (wind, xFalse);
		}
		
		EvntDestroyNotify (wind, wind->Id);
		
		while (wind->ButtonGrab) {
			_Wind_PgrabRemove (&wind->ButtonGrab);
		}
		EvntDel (wind);
		
		while (wind->Properties)  PropDelete (&wind->Properties);
		if    (wind->nSelections) SlctClear  (wind);
		if    (wind->Cursor)      CrsrFree   (wind->Cursor, NULL);
		if    (wind->hasBackPix)  PmapFree   (wind->Back.Pixmap, NULL);
		
		next  = wind->NextSibl;
		owner = (clnt ? clnt : ClntFind(wind->Id));
		
		if (wind->PrevSibl) wind->PrevSibl->NextSibl = wind->NextSibl;
		else                pwnd->StackBot           = wind->NextSibl;
		if (wind->NextSibl) wind->NextSibl->PrevSibl = wind->PrevSibl;
		else                pwnd->StackTop           = wind->PrevSibl;
		XrscDelete (owner->Drawables, wind);
		
		if (!n) break;
		
		if (!(wind = next)) {
			wind = pwnd;
			pwnd = pwnd->Parent;
			n--;
		}
	}
	
	if (enter) {
		if (bott->u.List.AllMasks & FocusChangeMask) {
			EvntFocusIn (bott, NotifyNormal, NotifyInferior);
		}
		if (WMGR_OpenCounter) {
			WindPointerWatch (xFalse); // correct watch rectangle
		} else {
			_WIND_PointerRoot = NULL;
			MainClrWatch();
			MainSetMove (xFalse);
			CrsrSelect (NULL);
		}
	}
}


//==============================================================================
BOOL
WindCirculate (WINDOW * wind, CARD8 place)
{
	BOOL notify = xFalse;
	BOOL redraw = xFalse;
	
	if (place == PlaceOnTop) {
		if (wind->NextSibl) {
			if ((wind->NextSibl->PrevSibl = wind->PrevSibl)) {
				  wind->PrevSibl->NextSibl = wind->NextSibl;
			} else { // (!wind->PrevSibl)
				  wind->Parent->StackBot   = wind->NextSibl;
			}
			wind->NextSibl         = NULL;
			wind->PrevSibl         = wind->Parent->StackTop;
			wind->Parent->StackTop = wind->PrevSibl->NextSibl = wind;
			notify = xTrue;
		}
		if (wind->Handle > 0) wind_set (wind->Handle, WF_TOP, 0,0,0,0);
		else                  redraw = notify;
	
	} else  { // (place == PlaceOnBottom)
		if (wind->PrevSibl) {
			if ((wind->PrevSibl->NextSibl = wind->NextSibl)) {
				  wind->NextSibl->PrevSibl = wind->PrevSibl;
			} else { // (!wind->NextSibl)
				  wind->Parent->StackTop   = wind->PrevSibl;
			}
			wind->PrevSibl         = NULL;
			wind->NextSibl         = wind->Parent->StackBot;
			wind->Parent->StackBot = wind->NextSibl->PrevSibl = wind;
			notify = xTrue;
		}
		if (wind->Handle > 0) wind_set (wind->Handle, WF_BOTTOM, 0,0,0,0);
		else                  redraw = notify;
	}
	
	if (notify) {
		EvntCirculateNotify (wind, wind->Id, place);
	}
	WindPointerWatch (xFalse);
	
	return redraw;
}


//------------------------------------------------------------------------------
static void
_Wind_Resize (WINDOW * wind, GRECT * diff)
{
	CARD32 above = (wind->PrevSibl ? wind->PrevSibl->Id : None);
	GRECT  work;
	
	work.g_x = (wind->Rect.g_x += diff->g_x) - wind->BorderWidth;
	work.g_y = (wind->Rect.g_y += diff->g_y) - wind->BorderWidth;
	work.g_w =  wind->Rect.g_w += diff->g_w;
	work.g_h =  wind->Rect.g_h += diff->g_h;
	EvntConfigureNotify (wind, wind->Id, above, &work,
	                     wind->BorderWidth, wind->Override);
	
	if ((wind = wind->StackBot)) do {
		BOOL notify = xFalse;
		if       (wind->WinGravity == NorthGravity  ||
		          wind->WinGravity == CenterGravity ||
		          wind->WinGravity == SouthGravity) {
			wind->Rect.g_x += diff->g_w /2;
			notify       =  xTrue;
		} else if (wind->WinGravity == NorthEastGravity ||
		           wind->WinGravity == EastGravity      ||
		           wind->WinGravity == SouthEastGravity) {
			wind->Rect.g_x += diff->g_w;
			notify       =  xTrue;
		}
		if       (wind->WinGravity == WestGravity   ||
		          wind->WinGravity == CenterGravity ||
		          wind->WinGravity == EastGravity) {
			wind->Rect.g_y += diff->g_h /2;
			notify       =  xTrue;
		} else if (wind->WinGravity == SouthWestGravity ||
		           wind->WinGravity == SouthGravity     ||
		           wind->WinGravity == SouthEastGravity) {
			wind->Rect.g_y += diff->g_h;
			notify       =  xTrue;
		}
		if (notify) {
			PXY pos = { wind->Rect.g_x - wind->BorderWidth,
			            wind->Rect.g_y - wind->BorderWidth };
			EvntGravityNotify (wind, wind->Id, pos);
		} else if (wind->isMapped  &&  wind->WinGravity == UnmapGravity) {
			WindClrMapped (wind, xTrue);
		}
	} while ((wind = wind->NextSibl));
}

//==============================================================================
void
WindResize (WINDOW * wind, GRECT * diff)
{
	GRECT curr;
	
	WmgrCalcBorder (&curr, wind);
	if ((curr.g_x += diff->g_x) < WIND_Root.Rect.g_x) {
		diff->g_x += WIND_Root.Rect.g_x - curr.g_x;
		curr.g_x  =  WIND_Root.Rect.g_x;
	}
	if ((curr.g_y += diff->g_y) < WIND_Root.Rect.g_y) {
		diff->g_y += WIND_Root.Rect.g_y - curr.g_y;
		curr.g_y  =  WIND_Root.Rect.g_y;
	}
	curr.g_w += diff->g_w;
	curr.g_h += diff->g_h;
	
	_Wind_Resize (wind, diff);
	
	if (wind->isMapped) {
		wind_set_curr (wind->Handle, &curr);
		if (!diff->g_x && !diff->g_y) {
			short decor = (wind->GwmDecor ? WMGR_Decor : 0);
			if (diff->g_w < decor || diff->g_h < decor) {
				WindDrawSection (wind, NULL);
			} else {
				GRECT work = wind->Rect;
				work.g_x += WIND_Root.Rect.g_x;
				work.g_y += WIND_Root.Rect.g_y;
				work.g_w -= diff->g_w - decor;
				work.g_h -= diff->g_h - decor;
				WindDrawSection (wind, &work);
			}
		}
	}
	
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//------------------------------------------------------------------------------
void
RQ_CreateWindow (CLIENT * clnt, xCreateWindowReq * q)
{
	WINDOW * pwnd = NULL;
	WINDOW * wind = NULL;
	
	if (DrawFind (q->wid).p) {
		Bad(BadIDChoice, q->wid, X_CreateWindow,"_");
		
	} else if (!(pwnd = WindFind(q->parent))) {
		Bad(BadWindow, q->parent, X_CreateWindow,"_");
		
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(BadValue, (short)((short)q->width <= 0 ? q->width : q->height), X_CreateWindow,"_ width = %i height = %i",
		           (short)q->width, (short)q->height);
		
	/* check visual */
	/* check depth */
	
	} else if (!(wind = XrscCreate(WINDOW, q->wid, clnt->Drawables,0))) {
		Bad(BadAlloc,0, X_CreateWindow,"_(W:%lX)", q->wid);
	
	} else {
//		PRINT (X_CreateWindow,"-(%u) W:%lX [%i,%i/%u,%u/%u:%u] on W:%lX with V:%lX",
//		       q->class, q->wid, q->x, q->y, q->width, q->height, q->borderWidth,
//		       q->depth, q->parent, q->visual);
		
		wind->isWind = xTrue;
		
		wind->ClassInOut = (q->class == CopyFromParent ? pwnd->ClassInOut :
		                    q->class == InputOutput    ? xTrue : xFalse);
		
		wind->Rect.g_x      = q->x + q->borderWidth;
		wind->Rect.g_y      = q->y + q->borderWidth;
		wind->Rect.g_w      = q->width;
		wind->Rect.g_h      = q->height;
		wind->BorderWidth = q->borderWidth;
		wind->Depth       = (!q->depth && wind->ClassInOut
		                     ? pwnd->Depth : q->depth);
		
		wind->Override     = xFalse;
		wind->BackingStore = NotUseful,
		wind->WinGravity   = NorthWestGravity;
		wind->BitGravity   = ForgetGravity;
		wind->SaveUnder    = xFalse;
		// BorderPixel and Back.Pixel needn't to be defined here
		
		wind->isMapped    = xFalse;
		wind->hasBorder   = xFalse;
		wind->hasBackGnd  = xFalse;
		wind->hasBackPix  = xFalse;
		wind->GwmParented = xFalse;
		wind->GwmDecor    = xFalse;
		wind->GwmIcon     = xFalse;
		
		wind->nSelections = 0;
		
		wind->PropagateMask  = AllEventMask;
		wind->u.Event.Mask   = 0uL;
		wind->u.Event.Client = NULL;
		wind->ButtonGrab     = NULL;
		
		wind->Parent     = pwnd;
		wind->PrevSibl   = NULL;
		wind->NextSibl   = NULL;
		wind->StackBot   = NULL;
		wind->StackTop   = NULL;
		wind->Cursor     = (q->mask & CWCursor ? NULL : CrsrShare (pwnd->Cursor));
		wind->Properties = NULL;
		
		if (pwnd->StackTop) {
			pwnd->StackTop->NextSibl = wind;
			wind->PrevSibl           = pwnd->StackTop;
		} else {
			pwnd->StackBot = wind;
		}
		pwnd->StackTop = wind;
		
		_Wind_setup (clnt, wind, q->mask, (CARD32*)(q +1), X_CreateWindow);
		
//		PRINT (0,"+");
		
		if (pwnd != &WIND_Root) {
			wind->Handle = -WindHandle (pwnd);
		
		} else if (!WmgrWindHandle (wind)) {
			Bad(BadAlloc,0, X_CreateWindow,"_(W:%lX): AES", q->wid);
			WindDelete (wind, clnt);
			return;
		}
		
		if (pwnd->u.List.AllMasks & SubstructureNotifyMask) {
			EvntCreateNotify (pwnd, wind->Id,
			                  &wind->Rect, wind->BorderWidth, wind->Override);
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_MapWindow (CLIENT * clnt, xMapWindowReq * q)
{
	WINDOW * wind = WindFind(q->id);

	if (!wind) {
		Bad(BadWindow, q->id, X_MapWindow,"_");
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (0,"\033pWARNING\033q MapWindow(W:%lX) ignored.", ROOT_WINDOW);
	#	endif
		
	} else if (!wind->isMapped) {
		
		DEBUG (MapWindow," W:%lX (%i)", q->id, wind->Handle);
		
		if (wind->Handle < 0) {
			if (WindSetMapped (wind, WindVisible (wind->Parent))) {
				GRECT curr;
				WindGeometry (wind, &curr, wind->BorderWidth);
				if (wind->SaveUnder) {
					WindSaveUnder (wind->Id, &curr, wind->Handle);
				}
				WindDrawSection (wind, &curr);
				if (wind->Parent == _WIND_PointerRoot) {
					WindPointerWatch (xFalse);
				}
			}
		} else {
			GRECT curr;
			BOOL  watch = WmgrWindMap (wind, &curr);
			if (watch) WindPointerWatch (xFalse);
			else       MainSetWatch (&curr, MO_ENTER);
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_MapSubwindows (CLIENT * clnt, xMapSubwindowsReq * q)
{
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(BadWindow, q->id, X_MapSubwindows,"_");
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (0,"\033pWARNING\033q MapSubwindows(W:%lX) ignored.", ROOT_WINDOW);
	#	endif
		
	} else if (wind->StackTop) {
		BOOL     visible = WindVisible (wind);
		WINDOW * w       = wind->StackTop;
		
		DEBUG (MapSubwindows," W:%lX", q->id);
		
		do {
			if (!w->isMapped && WindSetMapped (w, visible)) {
				GRECT curr;
				WindGeometry (w, &curr, w->BorderWidth);
				WindDrawSection (w, &curr);
			}
		} while ((w = w->PrevSibl));
		
		if (wind == _WIND_PointerRoot) {
			WindPointerWatch (xFalse);
		}
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_UnmapWindow (CLIENT * clnt, xUnmapWindowReq * q)
{
	// Unmap the window if it is mapped and not the root window.
	//
	// CARD32 id: window
	//...........................................................................
	
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(BadWindow, q->id, X_UnmapWindow,"_");
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (0,"\033pWARNING\033q UnmapWindow(W:%lX) ignored.", ROOT_WINDOW);
	#	endif
		
	} else if (wind->isMapped) { //..............................................
		
		DEBUG (UnmapWindow," W:%lX #%i", q->id, wind->Handle);
		
		_Wind_Unmap (wind, WindVisible (wind->Parent), xTrue);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_UnmapSubwindows (CLIENT * clnt, xUnmapSubwindowsReq * q)
{
	// Unmap all mapped children of the window, in bottom-to-top stacking order.
	//
	// CARD32 id: window
	//...........................................................................
	
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(BadWindow, q->id, X_UnmapSubwindows,"_");
	
	} else if (wind->StackBot) { //..............................................
		
		WINDOW * w    = wind->StackBot;
		BOOL     vis  = WindVisible (wind);
		GRECT    rect = { 0x7FFF, 0x7FFF, 0, 0 };
		BOOL     root = (wind == &WIND_Root);
		BOOL     draw = xFalse;
		
		PRINT (X_UnmapSubwindows," W:%lX", q->id);
		
		do if (w->isMapped) {
			if (vis && (draw = !root)) {
				GRECT work = w->Rect;
				int   b    = w->BorderWidth;
				if (b) {
					work.g_x -= b;
					work.g_y -= b;
					b *= 2;
					work.g_w += b;
					work.g_h += b;
				}
				if (rect.g_x >  work.g_x)            rect.g_x = work.g_x;
				if (rect.g_w < (work.g_w += work.g_x)) rect.g_w = work.g_w;
				if (rect.g_y >  work.g_y)            rect.g_y = work.g_y;
				if (rect.g_h < (work.g_h += work.g_y)) rect.g_h = work.g_h;
			}
			_Wind_Unmap (w, xFalse, xFalse);
		} while ((w = w->NextSibl));
		
		if (vis) {
			if (draw) {
				PXY orig = WindOrigin (wind);
				rect.g_w -= rect.g_x;
				rect.g_x += orig.p_x;
				rect.g_h -= rect.g_y;
				rect.g_y += orig.p_y;
				WindDrawSection (wind, &rect);
			}
			WindPointerWatch (xFalse);
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_ConfigureWindow (CLIENT * clnt, xConfigureWindowReq * q)
{

	WINDOW * wind = WindFind (q->window);
	
	if (!wind) {
		Bad(BadWindow, q->window, X_ConfigureWindow,"_");
	
	} else {
		CARD32 * val = (CARD32*)(q +1);
		GRECT    d   = { 0,0, 0,0 }, * r = &wind->Rect;
		short    db  = wind->BorderWidth;
		
//		PRINT (X_ConfigureWindow,"- W:%lX:", q->window);
		
		if (clnt->DoSwap) {
			CARD16 mask = q->mask;
			do if (mask & 1) {
				*val = Swap32(*val);
				val++;
			} while ((mask >>= 1));
			val = (CARD32*)(q +1);
		}
		
		if (q->mask & CWX)      { d.g_x = (short)*(val++) + db - r->g_x;
}//		                          PRINT (0,"+- x=%+i", d.g_x); }
		if (q->mask & CWY)      { d.g_y = (short)*(val++) + db - r->g_y;
}//		                          PRINT (0,"+- y=%+i", d.g_y); }
		if (q->mask & CWWidth)  { d.g_w = (short)*(val++) - r->g_w;
}//		                          PRINT (0,"+- w=%+i", d.g_w); }
		if (q->mask & CWHeight) { d.g_h = (short)*(val++) - r->g_h;
}//		                          PRINT (0,"+- h=%+i", d.g_h); }
		if (q->mask & CWBorderWidth) {
			db = (wind->BorderWidth = (short)*(val++)) - db;
			r->g_x += db;
			r->g_y += db;
//			PRINT (0,"+- brdr=%+i", db);
		}
		if (q->mask & CWSibling) {
//			PRINT (0,"+- sibl=0x%lX", (CARD32)*val);
			val++;
		}
			if (q->mask & CWStackMode) {
//				PRINT (0,"+- stck=%i", (int)(CARD8)*val);
				val++;
		}
//		PRINT (0,"+");
		
		if (wind->Handle > 0) {
			WindResize (wind, &d);
			
		} else {
			_Wind_Resize (wind, &d);
			if (WindVisible (wind) && (d.g_x || d.g_y || d.g_w || d.g_h)) {
				GRECT clip;
				WindGeometry (wind, &clip, wind->BorderWidth);
				if (d.g_x < 0) d.g_x    = -d.g_x;
				else         clip.g_x -= d.g_x;
				clip.g_w += (d.g_x > d.g_w ? d.g_x : d.g_w);
				if (d.g_y < 0) d.g_y    = -d.g_y;
				else         clip.g_y -= d.g_y;
				clip.g_h += (d.g_y > d.g_h ? d.g_y : d.g_h);
				WindDrawSection (wind->Parent, &clip);
			}
		}
		WindPointerWatch (xFalse);
	}
}

//------------------------------------------------------------------------------
void
RQ_ChangeWindowAttributes (CLIENT * clnt, xChangeWindowAttributesReq * q)
{
	WINDOW * wind = WindFind (q->window);
	
	if (!wind) {
		Bad(BadWindow, q->window, X_ChangeWindowAttributes,"_");
	
	} else {
		PIXMAP * desktop = NULL;
		
//		PRINT (X_ChangeWindowAttributes,"- W:%lX ", q->window);
		
		if ((q->valueMask & CWCursor) && wind->Cursor) {
			CrsrFree (wind->Cursor, NULL);
			wind->Cursor = NULL;
		}
		if ((q->valueMask & (CWBackPixmap|CWBackPixel)) && wind->hasBackPix) {
			if (wind->Id == ROOT_WINDOW) {
				desktop = wind->Back.Pixmap;
			} else {
				PmapFree (wind->Back.Pixmap, NULL);
			}
			wind->Back.Pixmap = NULL;
			wind->hasBackPix  = xFalse;
			wind->hasBackGnd  = xFalse;
		}
		_Wind_setup (clnt, wind, q->valueMask, (CARD32*)(q +1),
		             X_ChangeWindowAttributes);
		
//		PRINT (0,"+");
		
		if ((q->valueMask & CWOverrideRedirect)
		    && WMGR_Active && !wind->isMapped  &&  wind->Handle > 0) {
			if ((wind->Override == xFalse) != (wind->GwmDecor == xTrue)) {
				short hdl = wind->Handle;
				if (WmgrWindHandle (wind)) {
					WindSetHandles (wind);
					wind_delete (hdl);
				}
			}
		}
		if (q->valueMask & CWCursor) {
			if (_WIND_PgrabWindow) {
				if (!_WIND_PgrabCursor
				    && _Wind_IsInferior (wind, _WIND_PgrabWindow)) {
					_Wind_Cursor (_WIND_PgrabWindow);
				}
			} else if (_Wind_IsInferior (wind, _WIND_PointerRoot)) {
				_Wind_Cursor (_WIND_PointerRoot);
			}
		}
		if ((q->valueMask & (CWBackPixmap|CWBackPixel))
		    && wind->Id == ROOT_WINDOW) {
			WmgrSetDesktop (wind->hasBackGnd);
			if (desktop) PmapFree (desktop, NULL);
		}
		
		if (!wind->Id != ROOT_WINDOW) {
			WIND_ChngTrigger = xTrue;
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_CirculateWindow (CLIENT * clnt, xCirculateWindowReq * q)
{
	PRINT (- X_CirculateWindow," W:%lX %s",
	       q->window, (q->direction ? "RaiseLowest" : "LowerHighest"));
}

//------------------------------------------------------------------------------
void
RQ_GetWindowAttributes (CLIENT * clnt, xGetWindowAttributesReq * q)
{
	WINDOW * wind = NULL;
	
	if (!(wind = WindFind(q->id)) && (q->id & ~RID_MASK)) {
		Bad(BadWindow, q->id, X_GetWindowAttributes,"_");
	
	} else {
		ClntReplyPtr (GetWindowAttributes, r,0);
		
		DEBUG (GetWindowAttributes," W:%lX", q->id);
		
		if (wind) {
			r->backingStore     = wind->BackingStore;
			r->visualID         = (wind->Depth > 1 ? DFLT_VISUAL +1 : DFLT_VISUAL);
			r->class            = wind->ClassInOut;
			r->bitGravity       = wind->BitGravity;
			r->winGravity       = wind->WinGravity;
			r->backingBitPlanes = (1uL << wind->Depth) -1;
			r->backingPixel     = 0;
			r->saveUnder        = wind->SaveUnder;
			r->mapState         = (!wind->isMapped    ? IsUnmapped :
			                       WindVisible (wind) ? IsUnviewable : IsViewable);
			r->override         = wind->Override;
			r->colormap         = DFLT_COLORMAP;
			r->mapInstalled     = (q->id == ROOT_WINDOW ? xTrue : xFalse);
			r->allEventMasks    = wind->u.List.AllMasks & AllEventMask;
			r->yourEventMask      = NoEventMask;
			r->doNotPropagateMask = ~wind->PropagateMask;
			
			if (wind->u.List.AllMasks > 0  &&  wind->u.Event.Client == clnt) {
				r->yourEventMask = wind->u.Event.Mask;
			
			} else if (wind->u.List.AllMasks < 0) {
				int        num = wind->u.List.p->Length;
				WINDEVNT * lst = wind->u.List.p->Event;
				while (--num) {
					if (lst->Client == clnt) {
						r->yourEventMask = lst->Mask;
						break;
					}
					lst++;
				}
			}

		} else { // Id names an AES-Window
			r->backingStore     = NotUseful;
			r->visualID         = (GRPH_Depth > 1 ? DFLT_VISUAL +1 : DFLT_VISUAL);
			r->class            = InputOutput;
			r->bitGravity       = ForgetGravity;
			r->winGravity       = NorthWestGravity;
			r->backingBitPlanes = (1uL << GRPH_Depth) -1;
			r->backingPixel     = 0;
			r->saveUnder        = xFalse;
			r->mapState         = IsViewable;
			r->override         = xTrue;
			r->colormap         = DFLT_COLORMAP;
			r->mapInstalled     = xFalse;
			r->allEventMasks    = KeyPressMask|ButtonPressMask|ExposureMask|
			                      VisibilityChangeMask|StructureNotifyMask|
			                      ResizeRedirectMask;
			r->yourEventMask      = NoEventMask;
			r->doNotPropagateMask = (CARD16)AllEventMask;
		}
		
		ClntReply (GetWindowAttributes,0, "v.2ll4mss.");
	}
}

//------------------------------------------------------------------------------
void
RQ_DestroyWindow (CLIENT * clnt, xDestroyWindowReq * q)
{
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(BadWindow, q->id, X_DestroyWindow,"_");
	
	} else {
		WINDOW * pwnd;
		GRECT    curr;
		
		if (wind->Handle < 0  &&  WindVisible (wind)) {
			pwnd = wind->Parent;
			WindGeometry (wind, &curr, wind->BorderWidth);
		} else {
			pwnd = NULL;
		}
		
		DEBUG (DestroyWindow," W:%lX #%i", q->id, wind->Handle);
		
		WindDelete (wind, NULL);
		
		if (pwnd) WindDrawSection (pwnd, &curr);
	}
}

//------------------------------------------------------------------------------
void
RQ_DestroySubwindows (CLIENT * clnt, xDestroySubwindowsReq * q)
{
	PRINT (- X_DestroySubwindows," W:%lX", q->id);
}

//------------------------------------------------------------------------------
void
RQ_ReparentWindow (CLIENT * clnt, xReparentWindowReq * q)
{
	// Window window:
	// Window parent:
	// INT16  x, y:
	//...........................................................................
	
	WINDOW * wind = WindFind (q->window);
	WINDOW * pwnd = WindFind (q->parent);
	
	if (!wind) {
		Bad(BadWindow, q->window, X_ReparentWindow,"_(): invalid child.");
	
	} else if (!pwnd) {
		Bad(BadWindow, q->parent, X_ReparentWindow,"_(): invalid parent.");
	
	} else if (wind->Parent == pwnd) {
	#	ifndef NODEBUG
		PRINT (0,"\033pIGNORED\033q ReparentWindow() with same parent.");
	#	endif
		
	} else {
		BOOL     map;
		
		WINDOW * w = pwnd;
		do if (w == wind) {
			Bad(BadMatch,0, X_ReparentWindow,"_(): parent is inferior.");
			return;
		} while ((w = w->Parent));
		//........................................................................
		
		PRINT (X_ReparentWindow," W:%lX for W:%lX", q->window, q->parent);
		
		if ((map = wind->isMapped)) {
			_Wind_Unmap (wind, WindVisible (wind->Parent), xFalse);
		}
		if (wind->Handle > 0) {
			wind_delete (wind->Handle);
			wind->GwmParented = xFalse;
			wind->GwmDecor    = xFalse;
			wind->GwmIcon     = xFalse;
		}
		if (wind->PrevSibl) wind->PrevSibl->NextSibl = wind->NextSibl;
		else                wind->Parent->StackBot   = wind->NextSibl;
		if (wind->NextSibl) wind->NextSibl->PrevSibl = wind->PrevSibl;
		else                wind->Parent->StackTop   = wind->PrevSibl;
		wind->NextSibl = NULL;
		wind->Parent   = pwnd;
		if ((wind->PrevSibl = pwnd->StackTop)) {
			pwnd->StackTop->NextSibl = wind;
		} else {
			pwnd->StackBot = wind;
		}
		pwnd->StackTop = wind;
		wind->Rect.g_x   = q->x;
		wind->Rect.g_y   = q->y;
		
		if (wind->u.List.AllMasks & StructureNotifyMask) {
			EvntReparentNotify (wind, StructureNotifyMask,
			                    wind->Id, pwnd->Id,
			                    *(PXY*)&wind->Rect, wind->Override);
		}
		if (pwnd->u.List.AllMasks & SubstructureNotifyMask) {
			EvntReparentNotify (pwnd, SubstructureNotifyMask,
			                    wind->Id, pwnd->Id,
			                    *(PXY*)&wind->Rect, wind->Override);
		}
		
		if (pwnd == &WIND_Root) {
			WmgrWindHandle (wind);
			WindSetHandles (wind);
			if (map) {
				GRECT curr;
				WmgrWindMap (wind, &curr);
				WindPointerWatch (xFalse);
			}
		
		} else {
			wind->Handle   = -WindHandle (pwnd);
			WindSetHandles (wind);
			if (map && WindSetMapped (wind, WindVisible (pwnd))) {
				GRECT curr;
				WindGeometry (wind, &curr, wind->BorderWidth);
				WindDrawSection (wind, &curr);
				WindPointerWatch (xFalse);
			}
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_QueryTree (CLIENT * clnt, xQueryTreeReq * q)
{
	WINDOW * wind = WindFind (q->id);
	
	ClntReplyPtr (QueryTree, r,0);
	
	if (!wind) {
		Bad(BadWindow, q->id, X_QueryTree,"_");
	
	} else {
		WINDOW * chld = wind->StackBot;
		CARD32 * c_id = (CARD32*)(r +1);
		size_t   size = 0;
		size_t   bspc = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
		              - sz_xQueryTreeReply;
		
		PRINT (X_QueryTree," W:%lX", q->id);
		
		r->root      = ROOT_WINDOW;
		r->parent    = (wind->Parent ? wind->Parent->Id : None);
		r->nChildren = 0;
		
		while (chld) {
			size_t need = size + sizeof(CARD32);
			if (need > bspc) {
				r = ClntOutBuffer (&clnt->oBuf,
				                   sz_xQueryTreeReply + need,
				                   sz_xQueryTreeReply + size, xTrue);
				c_id = (CARD32*)(r +1) + r->nChildren;
				bspc = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
				     - sz_xQueryTreeReply;
			}
			*(c_id++) = (clnt->DoSwap ? Swap32(chld->Id) : chld->Id);
			r->nChildren++;
			size = need;
			chld = chld->NextSibl;
		}
		ClntReply (QueryTree, size, "ww.");
	}
}


//------------------------------------------------------------------------------
void
RQ_ChangeSaveSet (CLIENT * clnt, xChangeSaveSetReq * q)
{
	PRINT (- X_ChangeSaveSet," W:%lX", q->window);
}
