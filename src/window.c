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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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
		_MAP_Inc = WIND_Root.Rect.y -1;
		_WIND_RootX2 = WIND_Root.Rect.x + WIND_Root.Rect.w -1;
		_WIND_RootY2 = WIND_Root.Rect.y + WIND_Root.Rect.h -1;
		
	} else {
		if (WIND_Root.StackBot) {
			WINDOW * w = WIND_Root.StackBot;
			printf ("\33pFATAL\33q root window has still children!\n");
			while (w) {
				char w_f[16] = "(-)", w_l[16] = "(-)";
				if (w->StackBot) sprintf (w_f, "W:%X", w->StackBot->Id);
				if (w->StackTop) sprintf (w_l, "W:%X", w->StackTop->Id);
				printf ("  W:%X -> %s .. %s %s\n",
				        w->Id, w_f, w_l, (w == WIND_Root.StackTop ? "= Last" : ""));
				w = w->NextSibl;
			}
			exit (1);
		}
		if (WIND_Root.u.List.AllMasks) {
			printf ("\33pFATAL\33q root window event list not empty!\n");
			exit (1);
		}
		if (WIND_Root.Properties) {
				PropDelete (&WIND_Root.Properties);
			/*
			printf ("  remove Propert%s:",
			        (WIND_Root.Properties->Next ? "ies" : "y"));
			while (WIND_Root.Properties) {
				printf (" '%s'(%s)",
				        ATOM_Table[WIND_Root.Properties->Name]->Name,
				        ATOM_Table[WIND_Root.Properties->Type]->Name);
				if (WIND_Root.Properties->Type == XA_STRING) {
					printf ("='%s'", WIND_Root.Properties->Data);
				}
				PropDelete (&WIND_Root.Properties);
			}
			printf (",\n");
			*/
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
		printf ("\nW:%X 0x%lX #%i [%i,%i/%i,%i/%i] * %i \n",
		        wind->Id, wind->u.Event.Mask, wind->Handle,
		        wind->Rect.x, wind->Rect.y, wind->Rect.w, wind->Rect.h,
		        wind->BorderWidth, wind->Depth);
		if (wind->hasBorder || wind->hasBackGnd) {
			if (wind->hasBorder) {
				printf ("border = %li   ", wind->BorderPixel);
			}
			if (wind->hasBackPix) {
				printf ("backgnd: P:%X [%i,%i] * %i", wind->Back.Pixmap->Id,
				        wind->Back.Pixmap->W, wind->Back.Pixmap->H,
				        wind->Back.Pixmap->Depth);
			} else if (wind->hasBackGnd) {
				printf ("backgnd: %li", wind->Back.Pixel);
			}
			printf ("\n");
		}
		if (wind->Parent) {
			printf("parent: W:%X", wind->Parent->Id);
			if (wind->PrevSibl) printf ("   prev: W:%X", wind->PrevSibl->Id);
			if (wind->NextSibl) printf ("   next: W:%X", wind->NextSibl->Id);
			printf ("\n");
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
			int hdl = wind_find (MAIN_PointerPos->x, MAIN_PointerPos->y);
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
//		if (i) PRINT (,"+-\n          (%i):", i);
//	
//	} else if (mask) {
//		CARD32 m = mask;
//		int    i = 0;
//		while (m) {
//			if (m & 1) i++;
//			m >>= 1;
//		}
//		PRINT (,"+-\n          (%i):", i);
//	
//	} else {
//		return; // mask == 0, nothing to do
	}
	
	if (mask & CWBackPixmap) {
		if (*val != None  &&  *val != ParentRelative) {
			PIXMAP * pmap = PmapFind (*val);
			if (!pmap) {
//				PRINT(," ");
				Bad(Pixmap, *val, +req, "          invalid backbground.");
				return;
			} else if (pmap->Depth != w->Depth) {
//				PRINT(," ");
				Bad(Match,, +req, "          background depth %u not %u.",
				                  pmap->Depth, w->Depth);
				return;
			} else {
//				PRINT (,"+- bpix=%lX", *val);
				w->Back.Pixmap = PmapShare(pmap);
				w->hasBackPix  = xTrue;
				w->hasBackGnd  = xTrue;
			}
		} else {
//			PRINT (,"+- bpix=<none>");
		}
		val++;
	}
	if (mask & CWBackPixel) {
		// overrides prev
//		PRINT (,"+- bgnd=%lu", *val);
		w->Back.Pixel = CmapPixelIdx (*(val++), w->Depth);
		w->hasBackGnd = xTrue;
	}
	if (mask & CWBorderPixmap) {
//		PRINT (,"+- fpix=P:%lX", *val);
		val++;
	}
	if (mask & CWBorderPixel) {
		// overrides pref
//		PRINT (,"+- fgnd=%lu", *val);
		w->BorderPixel = CmapPixelIdx (*(val++), w->Depth);
		w->hasBorder   = xTrue;
	}
	if (mask & CWBitGravity) {
//		PRINT (,"+- bgrv=%u", (CARD8)*val);
		w->BitGravity = *(val++);
	}
	if (mask & CWWinGravity) {
//		PRINT (,"+- wgrv=%u", (CARD8)*val);
		w->WinGravity = *(val++);
	}
	if (mask & CWBackingStore) {
//		PRINT (,"+- bstr=%u", (CARD8)*val);
		w->BackingStore = *(val++);
	}
	if (mask & CWBackingPlanes) {
//		PRINT (,"+- bpln=%lX", *val);
		val++;
	}
	if (mask & CWBackingPixel) {
//		PRINT (,"+- bpix=%lX", *val);
		val++;
	}
	if (mask & CWOverrideRedirect) {
//		PRINT (,"+- rdir=%u", (CARD8)*val);
		w->Override = *(val++);
	}
	if (mask & CWSaveUnder) {
//		PRINT (,"+- save=%u", (CARD8)*val);
		w->SaveUnder = *(val++);
	}
	if (mask & CWEventMask) {
		CARD32 evnt = *val & AllEventMask;
		if (evnt) {
//			PRINT (,"+- evnt=%lX", evnt);
			EvntSet (w, clnt, evnt);
//			#define __V(e) if(evnt & e##Mask) PRINT (,"+-|" #e);
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
//			PRINT (,"+- evnt=<remove>");
			EvntClr (w, clnt);
		}
		val++;
	}
	if (mask & CWDontPropagate) {
		w->PropagateMask = ~*(val++) & AllEventMask;
//		PRINT (,"+- dprp=%lX", ~w->PropagateMask & AllEventMask);
	}
	if (mask & CWColormap) {
//		PRINT (,"+- cmap=M:%lX", (CARD32)*val);
		val++;
	}
	if (mask & CWCursor) {
//		PRINT (,"+- crsr=");
		if (*val != None) {
			if (!(w->Cursor = CrsrGet (*val))) {
//				PRINT (,"+-<invalid>");
			} else {
//				PRINT (,"+-%lX", (CARD32)*val);
			}
		} else {
//			PRINT (,"+-<none>");
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
			printf ("\33pPANIC\33q wind_destroy()"
			        " W:%X must be excluded from deleting!\n", wind->Id);
			exit(1);
		}
		if (wind->StackBot || wind->StackTop) {
			printf ("\33pPANIC\33q wind_destroy()"
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
	
	work.x = (wind->Rect.x += diff->x) - wind->BorderWidth;
	work.y = (wind->Rect.y += diff->y) - wind->BorderWidth;
	work.w =  wind->Rect.w += diff->w;
	work.h =  wind->Rect.h += diff->h;
	EvntConfigureNotify (wind, wind->Id, above, &work,
	                     wind->BorderWidth, wind->Override);
	
	if ((wind = wind->StackBot)) do {
		BOOL notify = xFalse;
		if       (wind->WinGravity == NorthGravity  ||
		          wind->WinGravity == CenterGravity ||
		          wind->WinGravity == SouthGravity) {
			wind->Rect.x += diff->w /2;
			notify       =  xTrue;
		} else if (wind->WinGravity == NorthEastGravity ||
		           wind->WinGravity == EastGravity      ||
		           wind->WinGravity == SouthEastGravity) {
			wind->Rect.x += diff->w;
			notify       =  xTrue;
		}
		if       (wind->WinGravity == WestGravity   ||
		          wind->WinGravity == CenterGravity ||
		          wind->WinGravity == EastGravity) {
			wind->Rect.y += diff->h /2;
			notify       =  xTrue;
		} else if (wind->WinGravity == SouthWestGravity ||
		           wind->WinGravity == SouthGravity     ||
		           wind->WinGravity == SouthEastGravity) {
			wind->Rect.y += diff->h;
			notify       =  xTrue;
		}
		if (notify) {
			PXY pos = { wind->Rect.x - wind->BorderWidth,
			            wind->Rect.y - wind->BorderWidth };
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
	if ((curr.x += diff->x) < WIND_Root.Rect.x) {
		diff->x += WIND_Root.Rect.x - curr.x;
		curr.x  =  WIND_Root.Rect.x;
	}
	if ((curr.y += diff->y) < WIND_Root.Rect.y) {
		diff->y += WIND_Root.Rect.y - curr.y;
		curr.y  =  WIND_Root.Rect.y;
	}
	curr.w += diff->w;
	curr.h += diff->h;
	
	_Wind_Resize (wind, diff);
	
	if (wind->isMapped) {
		wind_set_curr (wind->Handle, &curr);
		if (!diff->x && !diff->y) {
			short decor = (wind->GwmDecor ? WMGR_Decor : 0);
			if (diff->w < decor || diff->h < decor) {
				WindDrawSection (wind, NULL);
			} else {
				GRECT work = wind->Rect;
				work.x += WIND_Root.Rect.x;
				work.y += WIND_Root.Rect.y;
				work.w -= diff->w - decor;
				work.h -= diff->h - decor;
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
		Bad(IDChoice, q->wid, CreateWindow,);
		
	} else if (!(pwnd = WindFind(q->parent))) {
		Bad(Window, q->parent, CreateWindow,);
		
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(Value, (short)((short)q->width <= 0 ? q->width : q->height),
		           CreateWindow," width = %i height = %i",
		           (short)q->width, (short)q->height);
		
	/* check visual */
	/* check depth */
	
	} else if (!(wind = XrscCreate(WINDOW, q->wid, clnt->Drawables,))) {
		Bad(Alloc,, CreateWindow,"(W:%lX)", q->wid);
	
	} else {
//		PRINT (CreateWindow,"-(%u) W:%lX [%i,%i/%u,%u/%u:%u] on W:%lX with V:%lX",
//		       q->class, q->wid, q->x, q->y, q->width, q->height, q->borderWidth,
//		       q->depth, q->parent, q->visual);
		
		wind->isWind = xTrue;
		
		wind->ClassInOut = (q->class == CopyFromParent ? pwnd->ClassInOut :
		                    q->class == InputOutput    ? xTrue : xFalse);
		
		wind->Rect.x      = q->x + q->borderWidth;
		wind->Rect.y      = q->y + q->borderWidth;
		wind->Rect.w      = q->width;
		wind->Rect.h      = q->height;
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
		
//		PRINT (,"+");
		
		if (pwnd != &WIND_Root) {
			wind->Handle = -WindHandle (pwnd);
		
		} else if (!WmgrWindHandle (wind)) {
			Bad(Alloc,, CreateWindow,"(W:%lX): AES", q->wid);
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
		Bad(Window, q->id, MapWindow,);
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (,"\33pWARNING\33q MapWindow(W:%lX) ignored.", ROOT_WINDOW);
	#	endif NODEBUG
		
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
		Bad(Window, q->id, MapSubwindows,);
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (,"\33pWARNING\33q MapSubwindows(W:%lX) ignored.", ROOT_WINDOW);
	#	endif NODEBUG
		
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
		Bad(Window, q->id, UnmapWindow,);
	
	} else if (wind == &WIND_Root) {
	#	ifndef NODEBUG
		PRINT (,"\33pWARNING\33q UnmapWindow(W:%lX) ignored.", ROOT_WINDOW);
	#	endif NODEBUG
		
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
		Bad(Window, q->id, UnmapSubwindows,);
	
	} else if (wind->StackBot) { //..............................................
		
		WINDOW * w    = wind->StackBot;
		BOOL     vis  = WindVisible (wind);
		GRECT    rect = { 0x7FFF, 0x7FFF, 0, 0 };
		BOOL     root = (wind == &WIND_Root);
		BOOL     draw = xFalse;
		
		PRINT (UnmapSubwindows," W:%lX", q->id);
		
		do if (w->isMapped) {
			if (vis && (draw = !root)) {
				GRECT work = w->Rect;
				int   b    = w->BorderWidth;
				if (b) {
					work.x -= b;
					work.y -= b;
					b *= 2;
					work.w += b;
					work.h += b;
				}
				if (rect.x >  work.x)            rect.x = work.x;
				if (rect.w < (work.w += work.x)) rect.w = work.w;
				if (rect.y >  work.y)            rect.y = work.y;
				if (rect.h < (work.h += work.y)) rect.h = work.h;
			}
			_Wind_Unmap (w, xFalse, xFalse);
		} while ((w = w->NextSibl));
		
		if (vis) {
			if (draw) {
				PXY orig = WindOrigin (wind);
				rect.w -= rect.x;
				rect.x += orig.x;
				rect.h -= rect.y;
				rect.y += orig.y;
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
		Bad(Window, q->window, ConfigureWindow,);
	
	} else {
		CARD32 * val = (CARD32*)(q +1);
		GRECT    d   = { 0,0, 0,0 }, * r = &wind->Rect;
		short    db  = wind->BorderWidth;
		
//		PRINT (ConfigureWindow,"- W:%lX:", q->window);
		
		if (clnt->DoSwap) {
			CARD16 mask = q->mask;
			do if (mask & 1) {
				*val = Swap32(*val);
				val++;
			} while ((mask >>= 1));
			val = (CARD32*)(q +1);
		}
		
		if (q->mask & CWX)      { d.x = (short)*(val++) + db - r->x;
}//		                          PRINT (,"+- x=%+i", d.x); }
		if (q->mask & CWY)      { d.y = (short)*(val++) + db - r->y;
}//		                          PRINT (,"+- y=%+i", d.y); }
		if (q->mask & CWWidth)  { d.w = (short)*(val++) - r->w;
}//		                          PRINT (,"+- w=%+i", d.w); }
		if (q->mask & CWHeight) { d.h = (short)*(val++) - r->h;
}//		                          PRINT (,"+- h=%+i", d.h); }
		if (q->mask & CWBorderWidth) {
			db = (wind->BorderWidth = (short)*(val++)) - db;
			r->x += db;
			r->y += db;
//			PRINT (,"+- brdr=%+i", db);
		}
		if (q->mask & CWSibling) {
//			PRINT (,"+- sibl=0x%lX", (CARD32)*val);
			val++;
		}
			if (q->mask & CWStackMode) {
//				PRINT (,"+- stck=%i", (int)(CARD8)*val);
				val++;
		}
//		PRINT (,"+");
		
		if (wind->Handle > 0) {
			WindResize (wind, &d);
			
		} else {
			_Wind_Resize (wind, &d);
			if (WindVisible (wind) && (d.x || d.y || d.w || d.h)) {
				GRECT clip;
				WindGeometry (wind, &clip, wind->BorderWidth);
				if (d.x < 0) d.x    = -d.x;
				else         clip.x -= d.x;
				clip.w += (d.x > d.w ? d.x : d.w);
				if (d.y < 0) d.y    = -d.y;
				else         clip.y -= d.y;
				clip.h += (d.y > d.h ? d.y : d.h);
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
		Bad(Window, q->window, ChangeWindowAttributes,);
	
	} else {
		PIXMAP * desktop = NULL;
		
//		PRINT (ChangeWindowAttributes,"- W:%lX ", q->window);
		
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
		
//		PRINT (,"+");
		
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
		Bad(Window, q->id, GetWindowAttributes,);
	
	} else {
		ClntReplyPtr (GetWindowAttributes, r,);
		
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
		
		ClntReply (GetWindowAttributes,, "v.2ll4mss.");
	}
}

//------------------------------------------------------------------------------
void
RQ_DestroyWindow (CLIENT * clnt, xDestroyWindowReq * q)
{
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(Window, q->id, DestroyWindow,);
	
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
		Bad(Window, q->window, ReparentWindow,"(): invalid child.");
	
	} else if (!pwnd) {
		Bad(Window, q->parent, ReparentWindow,"(): invalid parent.");
	
	} else if (wind->Parent == pwnd) {
	#	ifndef NODEBUG
		PRINT (,"\33pIGNORED\33q ReparentWindow() with same parent.");
	#	endif NODEBUG
		
	} else {
		BOOL     map;
		
		WINDOW * w = pwnd;
		do if (w == wind) {
			Bad(Match,, ReparentWindow,"(): parent is inferior.");
			return;
		} while ((w = w->Parent));
		//........................................................................
		
		PRINT (ReparentWindow," W:%lX for W:%lX", q->window, q->parent);
		
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
		wind->Rect.x   = q->x;
		wind->Rect.y   = q->y;
		
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
	
	ClntReplyPtr (QueryTree, r,);
	
	if (!wind) {
		Bad(Window, q->id, QueryTree,);
	
	} else {
		WINDOW * chld = wind->StackBot;
		CARD32 * c_id = (CARD32*)(r +1);
		size_t   size = 0;
		size_t   bspc = clnt->oBuf.Size - (clnt->oBuf.Done + clnt->oBuf.Left)
		              - sz_xQueryTreeReply;
		
		PRINT (QueryTree," W:%lX", q->id);
		
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
