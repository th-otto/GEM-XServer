/*
 *==============================================================================
 *
 * wmgr.c -- Built-in window manager.
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-09-01 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>

#include "main.h"
#include "clnt.h"
#include "tools.h"
#include "wmgr.h"
#include "window.h"
#include "pixmap.h"
#include "event.h"
#include "Cursor.h"
#include "grph.h"
#include "colormap.h"
#include "Atom.h"
#include "Property.h"
#include "Request.h"
#include "x_gem.h"
#include "x_mint.h"
#include "Xapp.h"
#include "version.h"
#include "x_printf.h"

#include <X11/Xatom.h>
#include <X11/cursorfont.h>


BOOL WMGR_ExitFlag = xFalse;
BOOL WMGR_Active = xFalse;;
CARD16 WMGR_Cursor = 0x0000;

#define WMGR_DECOR      6
short WMGR_Decor = WMGR_DECOR;

#define A_WIDGETS       (NAME|MOVER|CLOSER|SMALLER)
#define P_WIDGETS       0

CARD16 WMGR_OpenCounter = 0;
short WMGR_Focus = 0;
short _WMGR_FocusHolder = 0;
static short _WMGR_HasFocus = 0;
static short _WMGR_WidgetColor[3] = { W_NAME, };

#include "intro.xbm"
static MFDB _WMGR_Logo = { x_bits, x_width, x_height, (x_width + 15) / 16, 0, 1 };
static MFDB _WMGR_Qbuf = { NULL, x_width, x_height, (x_width + 15) / 16, 0, 0 };
static short _WMGR_P[8] = { 0, 0, x_width - 1, x_height - 1 };

static MFDB _WMGR_Icon = { NULL, 0, 0, 0, 0, 1 };

static OBJECT *_WMGR_Menu = NULL;
static char _WMGR_MenuEmpty[] = "   ";

static struct
{
	CURSOR L;
	CURSOR LD;
	CURSOR D;
	CURSOR RD;
	CURSOR R;
	CURSOR X;
} _WMGR_Cursor = {
	{ NULL, 0, XC_left_side, 1uL, { 0, 0 }, 0, G_WHITE, G_RED },
	{ NULL, 0, XC_bottom_left_corner, 1uL, { 0, 0 }, 0, G_WHITE, G_RED },
	{ NULL, 0, XC_bottom_side, 1uL, { 0, 0 }, 0, G_WHITE, G_RED },
	{ NULL, 0, XC_bottom_right_corner, 1uL, { 0, 0 }, 0, G_WHITE, G_RED },
	{ NULL, 0, XC_right_side, 1uL, { 0, 0 }, 0, G_WHITE, G_RED },
	{ NULL, 0, XC_fleur, 1uL, { 0, 0 }, 0, G_WHITE, G_RED }
};

static void FT_Wmgr_reply(p_CLIENT, CARD32 size, const char *form);
static void FT_Wmgr_error(p_CLIENT, CARD8 code, CARD8, CARD16, CARD32 val);
static void FT_Wmgr_event(p_CLIENT, p_WINDOW, const xEvent *ev);
static xReply _WMGR_Obuf;

static FUNCTABL _WMGR_Table = {
	FT_Wmgr_reply,
	FT_Wmgr_error,
	FT_Wmgr_event,
	FT_Grph_ShiftArc_Unswapped,
	FT_Grph_ShiftPnt_Unswapped,
	FT_Grph_ShiftR2P_Unswapped
};

static CLIENT _WMGR_Client = {
	NULL, xFalse, 0x00000ul,			/* NULL, */
	-1, -1, 0L, NULL,
	NULL, 0u, xFalse, RetainPermanent,
	(char *) "localhost", (char *) "127.0.0.1",
	{0ul, 0ul, sizeof(_WMGR_Obuf), (char *) &_WMGR_Obuf}, {0ul, 0ul, NULL},
	NULL, &_WMGR_Table, 0ul,
	{{0}}, {{0}}, {{0}}, "  wm"
};


/* ============================================================================== */
void WmgrIntro(BOOL onNoff)
{
	static BOOL on_screen = xFalse;

	if (onNoff)
	{
		if (GrphInit() && !on_screen)
		{
			short pxy[8] = {
				(_WMGR_P[4] = (GRPH_ScreenW - x_width) / 2),
				(_WMGR_P[5] = (GRPH_ScreenH - x_height) / 2),
				(_WMGR_P[6] = _WMGR_P[4] + x_width - 1),
				(_WMGR_P[7] = _WMGR_P[5] + x_height - 1),
				_WMGR_P[0], _WMGR_P[1], _WMGR_P[2], _WMGR_P[3]
			};
			MFDB scrn = { NULL };
			short clr[2] = { G_BLACK, G_YELLOW };
			short wg[4];

			graf_mouse(BUSYBEE, NULL);
			WindUpdate(xTrue);

			wind_get(0, WF_SCREEN, wg, wg + 1, wg + 2, wg + 3);
			_WMGR_Qbuf.fd_addr = (void *) (((unsigned long) (unsigned short) wg[0] << 16) | ((unsigned long) (unsigned short) wg[1]));
			_WMGR_Qbuf.fd_nplanes = GRPH_Depth;

			vst_alignment(GRPH_Vdi, TA_LEFT, TA_TOP, wg, wg);
			vst_effects(GRPH_Vdi, TXT_UNDERLINED);
			vswr_mode(GRPH_Vdi, MD_XOR);

			v_hide_c(GRPH_Vdi);
			vro_cpyfm(GRPH_Vdi, S_ONLY, pxy, &scrn, &_WMGR_Qbuf);
			vrt_cpyfm(GRPH_Vdi, MD_TRANS, _WMGR_P, &_WMGR_Logo, &scrn, clr);
			v_gtext(GRPH_Vdi, pxy[0] + 27, pxy[1], "V11 R6.4");
			v_show_c(GRPH_Vdi, 1);

			vst_alignment(GRPH_Vdi, TA_LEFT, TA_BASE, wg, wg);
			vst_effects(GRPH_Vdi, TXT_NORMAL);

			on_screen = xTrue;
		}

	} else
	{
		if (on_screen)
		{
			MFDB scrn = { NULL };

			v_hide_c(GRPH_Vdi);
			vro_cpyfm(GRPH_Vdi, S_ONLY, _WMGR_P, &_WMGR_Qbuf, &scrn);
			v_show_c(GRPH_Vdi, 1);

			WindUpdate(xFalse);

			on_screen = xFalse;
		}
		graf_mouse(ARROW, NULL);
	}
}

/* ------------------------------------------------------------------------------ */
#if 0
static void _Wmgr_request(void *buf, size_t len)
{
	xReq *q = buf;

	q->length = Units(len);
	_WMGR_Client.iBuf.Mem = buf;
	_WMGR_Client.iBuf.Done = len;
	_WMGR_Client.SeqNum++;
}
#endif

/* ============================================================================== */
BOOL WmgrInit(BOOL initNreset)
{
	BOOL ok = xTrue;

	if (initNreset)
	{
		XrscPoolInit(_WMGR_Client.Drawables);
		XrscPoolInit(_WMGR_Client.Fontables);
		XrscPoolInit(_WMGR_Client.Cursors);

		if (!rsrc_load(PATH_RSRC) && !rsrc_load(PATH_XBIN_RSRC))
		{
			WmgrIntro(xFalse);
			form_alert(1, "[3][Can't load RSC file!][ Quit ]");
			ok = xFalse;
		} else
		{
			OBJECT *tree;
			short i;

			rsrc_gaddr(R_TREE, ABOUT, &tree);
			_WMGR_Icon.fd_addr = tree[ABOUT_LOGO].ob_spec.bitblk->bi_pdata;
			_WMGR_Icon.fd_w = tree[ABOUT_LOGO].ob_height;
			_WMGR_Icon.fd_h = tree[ABOUT_LOGO].ob_spec.bitblk->bi_hl;
			_WMGR_Icon.fd_wdwidth = (tree[ABOUT_LOGO].ob_spec.bitblk->bi_wb + 1) / 2;

			rsrc_gaddr(R_TREE, MENU, &_WMGR_Menu);
			if (_app)
			{
				for (i = MENU_CLNT_FRST; i <= MENU_CLNT_LAST; ++i)
				{
					_WMGR_Menu[i].ob_spec.free_string = _WMGR_MenuEmpty;
					_WMGR_Menu[i].ob_flags |= OF_HIDETREE;
				}
				_WMGR_Menu[MENU_CLNT].ob_state |= OS_DISABLED;
				menu_bar(_WMGR_Menu, 1);
				menu_ienable(_WMGR_Menu, MENU_GWM, 1);
			}
			i = *_WMGR_WidgetColor;
			wind_get(0, WF_DCOLOR, &i, &_WMGR_WidgetColor[1], &_WMGR_WidgetColor[2], &i);
			_WMGR_FocusHolder = wind_create(0, -100, -100, 99, 99);
		}

	} else
	{
		/* something to reset ... */
		WmgrIntro(xFalse);
	}

	return ok;
}

/* ============================================================================== */
void WmgrExit(void)
{
	WmgrSetDesktop(xFalse);
	ClntInit(xFalse);

	if (_WMGR_Menu)
	{
		menu_bar(_WMGR_Menu, 0);
		rsrc_free();
		_WMGR_Menu = NULL;
	}
	WmgrIntro(xFalse);
	GrphExit();
}


/* ============================================================================== */
void WmgrActivate(BOOL onNoff)
{
	WINDOW *w = WIND_Root.StackBot;

	static BOOL __init = xTrue;

	if (__init)
	{
		/* init cursors */
		CrsrSetGlyph(&_WMGR_Cursor.L, _WMGR_Cursor.L.Id);
		CrsrSetGlyph(&_WMGR_Cursor.LD, _WMGR_Cursor.LD.Id);
		CrsrSetGlyph(&_WMGR_Cursor.D, _WMGR_Cursor.D.Id);
		CrsrSetGlyph(&_WMGR_Cursor.RD, _WMGR_Cursor.RD.Id);
		CrsrSetGlyph(&_WMGR_Cursor.R, _WMGR_Cursor.R.Id);
		CrsrSetGlyph(&_WMGR_Cursor.X, _WMGR_Cursor.X.Id);

		while (Kbshift(-1) & (K_LSHIFT | K_RSHIFT))
			;
		WmgrIntro(xFalse);

		__init = xFalse;
	} else if (onNoff == WMGR_Active)
	{
		puts("*BOINK*");
		return;
	}

	if (onNoff)
	{
		WMGR_Active = xTrue;
		while (w)
		{
			short hdl = w->Handle;

			if (!w->Override && WmgrWindHandle(w))
			{
				if (w->Properties && w->Properties->WindName)
				{
					wind_set_str(w->Handle, WF_NAME, w->Properties->WindName);
				}
				WindSetHandles(w);
				if (w->isMapped)
				{
					GRECT dummy;

					WindClrMapped(w, xFalse);
					WMGR_OpenCounter--;
					WmgrWindMap(w, &dummy);
				} else
				{
#	define clnt &_WMGR_Client
					PRINT(X_ReparentWindow, "(W:%X) active", w->Id);
#	undef clnt
				}
				wind_delete(hdl);
			}
			w = w->NextSibl;
		}
		if (_app)
			menu_icheck(_WMGR_Menu, MENU_GWM, 1);

		if (CLNT_BaseNum == 0 && access(PATH_XmodmapRc, R_OK) == 0 && access(PATH_Xmodmap, X_OK) == 0)
		{
			const char *argv[] = { PATH_XmodmapRc, NULL };
			WmgrLaunch(PATH_Xmodmap, 1, argv);
		}
	} else
	{
		WmgrCursorOff(NULL);
		WMGR_Active = xFalse;
		while (w)
		{
			short hdl = w->Handle;

			if (w->GwmDecor && WmgrWindHandle(w))
			{
				WindSetHandles(w);
				if (w->isMapped || w->GwmIcon)
				{
					GRECT curr;

					if (w->Properties && w->Properties->WindName)
					{
						wind_set_str(w->Handle, WF_NAME, w->Properties->WindName);
					}
					if (w->isMapped)
					{
						WindClrMapped(w, xFalse);
						WMGR_OpenCounter--;
						wind_get_curr(hdl, &curr);
					} else
					{
						w->GwmIcon = xFalse;
						wind_get_grect(hdl, WF_UNICONIFY, &curr);
					}
#	define clnt &_WMGR_Client
					PRINT(X_ReparentWindow, "(W:%X) revert", w->Id);
#	undef clnt
					if (w->WinGravity == NorthWestGravity ||
						w->WinGravity == NorthGravity || w->WinGravity == NorthEastGravity)
					{
						w->Rect.g_y = curr.g_y - WIND_Root.Rect.g_y + w->BorderWidth;
					} else if (w->WinGravity == SouthWestGravity ||
							   w->WinGravity == SouthGravity || w->WinGravity == SouthEastGravity)
					{
						w->Rect.g_y = curr.g_y - WIND_Root.Rect.g_y - w->BorderWidth * 2 + curr.g_h - w->Rect.g_h;
					}
					if (w->WinGravity == NorthWestGravity ||
						w->WinGravity == WestGravity || w->WinGravity == SouthWestGravity)
					{
						w->Rect.g_x = curr.g_x - WIND_Root.Rect.g_x + w->BorderWidth;
					} else if (w->WinGravity == NorthEastGravity ||
							   w->WinGravity == EastGravity || w->WinGravity == SouthEastGravity)
					{
						w->Rect.g_x = curr.g_x - WIND_Root.Rect.g_x - w->BorderWidth * 2 + curr.g_w - w->Rect.g_w;
					}
					WmgrWindMap(w, &curr);
					if (w->u.List.AllMasks & StructureNotifyMask)
					{
						PXY p;

						p.p_x = w->Rect.g_x;
						p.p_y = w->Rect.g_y;
						EvntReparentNotify(w, StructureNotifyMask, w->Id, ROOT_WINDOW, p, w->Override);
					}
					if (WIND_Root.u.List.AllMasks & SubstructureNotifyMask)
					{
						PXY p;

						p.p_x = w->Rect.g_x;
						p.p_y = w->Rect.g_y;
						EvntReparentNotify(&WIND_Root, SubstructureNotifyMask, w->Id, ROOT_WINDOW, p, w->Override);
					}
				} else
				{
#	define clnt &_WMGR_Client
					PRINT(X_ReparentWindow, "(W:%X) passive", w->Id);
#	undef clnt
				}
				wind_delete(hdl);
			}
			w = w->NextSibl;
		}
		if (_app)
			menu_icheck(_WMGR_Menu, MENU_GWM, 0);
	}
}


/* ============================================================================== */
void WmgrClntInsert(CLIENT *client)
{
	char *ent = client->Entry;

	sprintf(ent, "  ---: [%s:%i]", client->Addr, client->Port);

	if (_app)
	{
		int n = MENU_CLNT_FRST;

		while (n <= MENU_CLNT_LAST)
		{
			char *tmp = _WMGR_Menu[n].ob_spec.free_string;

			_WMGR_Menu[n].ob_spec.free_string = ent;
			if (_WMGR_Menu[n].ob_flags & OF_HIDETREE)
			{
				_WMGR_Menu[n].ob_flags &= ~OF_HIDETREE;
				_WMGR_Menu[MENU_CLNT].ob_state &= ~OS_DISABLED;
				_WMGR_Menu[MENU_CLNT_FACE].ob_height = _WMGR_Menu[n].ob_y + _WMGR_Menu[n].ob_height;
				break;
			}
			ent = tmp;
			n++;
		}
	}
}

/* ============================================================================== */
void WmgrClntUpdate(CLIENT *client, const char *text)
{
	if (text)
	{
		strncpy(client->Entry + 7, text, CLNTENTLEN - 8);
		client->Entry[CLNTENTLEN - 1] = ' ';
		client->Entry[CLNTENTLEN] = '\0';

	} else
	{
		char buf[8];
		int len = sprintf(buf, "%03X", client->Id << (RID_MASK_BITS & 3));

		strncpy(client->Entry + 2, buf, len);
	}
}

/* ============================================================================== */
void WmgrClntRemove(CLIENT *client)
{
	if (_app)
	{
		int n = MENU_CLNT_FRST;

		while (n < MENU_CLNT_LAST && _WMGR_Menu[n].ob_spec.free_string != client->Entry)
			n++;
		while (n < MENU_CLNT_LAST && !(_WMGR_Menu[n + 1].ob_flags & OF_HIDETREE))
		{
			_WMGR_Menu[n].ob_spec.free_string = _WMGR_Menu[n + 1].ob_spec.free_string;
			n++;
		}
		_WMGR_Menu[n].ob_spec.free_string = _WMGR_MenuEmpty;
		_WMGR_Menu[n].ob_flags |= OF_HIDETREE;
		if (n == MENU_CLNT_FRST)
		{
			_WMGR_Menu[MENU_CLNT].ob_state |= OS_DISABLED;
			menu_bar(_WMGR_Menu, 1);
		} else
		{
			_WMGR_Menu[MENU_CLNT_FACE].ob_height = _WMGR_Menu[n].ob_y;
		}
	}
}


/* ============================================================================== */
#define A_WIDGETS (NAME|MOVER|CLOSER|SMALLER)
#define P_WIDGETS 0

void WmgrCalcBorder(GRECT *curr, WINDOW *wind)
{
	GRECT work = wind->Rect;

	work.g_x += WIND_Root.Rect.g_x;
	work.g_y += WIND_Root.Rect.g_y;

	if (wind->GwmDecor)
	{
		work.g_x -= WMGR_DECOR;
		work.g_w += WMGR_DECOR * 2;
		work.g_h += WMGR_DECOR;
		wind_calc_grect(WC_BORDER, A_WIDGETS, &work, curr);

	} else
	{
		CARD16 b = wind->BorderWidth;

		if (b)
		{
			if (wind->hasBorder && wind->BorderPixel == G_BLACK)
			{
				b--;
			}
			work.g_x -= b;
			work.g_y -= b;
			work.g_w += b * 2;
			work.g_h += b * 2;
		}
		wind_calc_grect(WC_BORDER, P_WIDGETS, &work, curr);
	}
}


/* ------------------------------------------------------------------------------ */
static void _Wmgr_SetWidgets(short hdl, int state)
{
	/* state ==  0: normal */
	/*       >= +1: topped */
	/*       <= -1: bottom */

	int a = (state >= 0 ? 1 : 2);
	int b = (state <= 0 ? 2 : 1);

	wind_set(hdl, WF_COLOR, *_WMGR_WidgetColor, _WMGR_WidgetColor[a], _WMGR_WidgetColor[b], -1);
}

/* ============================================================================== */
BOOL WmgrWindHandle(WINDOW *wind)
{
	BOOL decor = WMGR_Active && !wind->Override;
	short hdl = wind_create_grect((decor ? A_WIDGETS : P_WIDGETS), &WIND_Root.Rect);

	if (hdl > 0)
	{
		OBJECT *icons;

		wind_set(hdl, WF_BEVENT, 0x0001, 0, 0, 0);
		wind->Handle = hdl;
		wind->GwmDecor = decor;
		wind->GwmParented = xFalse;
		if (rsrc_gaddr(R_TREE, ICONS, &icons))
		{
			wind_set_proc(hdl, icons[ICONS_NAME_X].ob_spec.ciconblk);
		}
		_Wmgr_SetWidgets(hdl, -1);

		return xTrue;
	}
	return xFalse;
}

/* ============================================================================== */
BOOL WmgrWindMap(WINDOW *wind, GRECT *curr)
{
	short action = 0;

	WmgrCalcBorder(curr, wind);

	if (wind->GwmIcon)
	{
		if (wind->Properties && wind->Properties->WindName)
		{
			wind_set_str(wind->Handle, WF_NAME, wind->Properties->WindName);
		}
		wind_set(wind->Handle, WF_BEVENT, 0x0001, 0, 0, 0);
		_Wmgr_SetWidgets(wind->Handle, -1);
		wind->GwmIcon = xFalse;
		action = -1;

	} else if (!wind->isMapped)
	{

		if (!wind->GwmDecor)
		{
			if (curr->g_y < 0)
			{
				CARD32 above = (wind->PrevSibl ? wind->PrevSibl->Id : None);
				GRECT work;

				wind->Rect.g_y -= curr->g_y;
				curr->g_y = 0;
				WmgrCalcBorder(curr, wind);
				work.g_x = wind->Rect.g_x - wind->BorderWidth;
				work.g_y = wind->Rect.g_y - wind->BorderWidth;
				work.g_w = wind->Rect.g_w;
				work.g_h = wind->Rect.g_h;
				EvntConfigureNotify(wind, wind->Id, above, &work, wind->BorderWidth, wind->Override);
			}
			if (wind->SaveUnder && wind->Override)
			{
				GRECT rect = *curr;

				rect.g_w += 2;
				rect.g_h += 2;
				WindSaveUnder(wind->Id, &rect, 0);
			}
			wind->GwmParented = xFalse;

		} else if (!wind->GwmParented)
		{
			CARD16 b = wind->BorderWidth;
			int dx = wind->Rect.g_x - curr->g_x;
			int dy = wind->Rect.g_y - curr->g_y;
			int d;
			PXY pos;

#	define clnt &_WMGR_Client
			PRINT(X_ReparentWindow, "(W:%X) mapped", wind->Id);
#	undef clnt
			if (wind->WinGravity == NorthWestGravity ||
				wind->WinGravity == WestGravity || wind->WinGravity == SouthWestGravity)
			{
				curr->g_x = wind->Rect.g_x + WIND_Root.Rect.g_x - b;
			} else if (wind->WinGravity == NorthEastGravity ||
					   wind->WinGravity == EastGravity || wind->WinGravity == SouthEastGravity)
			{
				curr->g_x = wind->Rect.g_x + WIND_Root.Rect.g_x + wind->Rect.g_w + b * 2 - curr->g_w;
			}
			d = (curr->g_x + curr->g_w) - (WIND_Root.Rect.g_x + WIND_Root.Rect.g_w);
			if (d > 0)
				curr->g_x -= d;
			if (curr->g_x < WIND_Root.Rect.g_x)
				curr->g_x = WIND_Root.Rect.g_x;
			wind->Rect.g_x = curr->g_x + dx;

			if (wind->WinGravity == NorthWestGravity ||
				wind->WinGravity == NorthGravity || wind->WinGravity == NorthEastGravity)
			{
				curr->g_y = wind->Rect.g_y + WIND_Root.Rect.g_y - b;
			} else if (wind->WinGravity == SouthWestGravity ||
					   wind->WinGravity == SouthGravity || wind->WinGravity == SouthEastGravity)
			{
				curr->g_y = wind->Rect.g_y + WIND_Root.Rect.g_y + wind->Rect.g_h + b * 2 - curr->g_h;
			}
			d = (curr->g_y + curr->g_h) - (WIND_Root.Rect.g_y + WIND_Root.Rect.g_h);
			if (d > 0)
				curr->g_y -= d;
			if (curr->g_y < WIND_Root.Rect.g_y)
				curr->g_y = WIND_Root.Rect.g_y;
			wind->Rect.g_y = curr->g_y + dy;

			pos.p_x = wind->Rect.g_x - b;
			pos.p_y = wind->Rect.g_y - b;
			if (wind->u.List.AllMasks & StructureNotifyMask)
			{
				EvntReparentNotify(wind, StructureNotifyMask, wind->Id, ROOT_WINDOW, pos, wind->Override);
			}
			if (WIND_Root.u.List.AllMasks & SubstructureNotifyMask)
			{
				EvntReparentNotify(&WIND_Root, SubstructureNotifyMask, wind->Id, ROOT_WINDOW, pos, wind->Override);
			}
			wind->GwmParented = xTrue;
		}
		action = +1;
	}

	if (action)
	{
		if (!WMGR_OpenCounter++)
		{
			wind_open(_WMGR_FocusHolder, -100, -100, 10, 10);
			_WMGR_HasFocus = 1;
		} else
		{
			_WMGR_HasFocus = 2;
		}
		if (action > 0)
			wind_open_grect(wind->Handle, curr);
		else
			wind_set_grect(wind->Handle, WF_UNICONIFY, curr);
		WindSetMapped(wind, xTrue);
	}
	return (WMGR_OpenCounter > 1);
}

/* ============================================================================== */
BOOL WmgrWindUnmap(WINDOW *wind, BOOL destroy)
{
	if (wind->GwmIcon)
	{
		wind_close(wind->Handle);
		wind->GwmIcon = xFalse;

	} else if (wind->isMapped)
	{
		wind_close(wind->Handle);
		if (!--WMGR_OpenCounter)
		{
			wind_close(_WMGR_FocusHolder);
		}
		WindClrMapped(wind, xFalse);
	}

	if (destroy)
	{
		wind_delete(wind->Handle);
		wind->Handle = -1;
	}

	return (WMGR_OpenCounter > 0);
}


/* ------------------------------------------------------------------------------ */
static void _Wmgr_DrawIcon(WINDOW *wind, GRECT *clip)
{
	GRECT work;
	GRECT sect;
	MFDB screen = { NULL };
	MFDB *icon = &_WMGR_Icon;
	MFDB *mask = NULL;
	short pxy[8] = { 0, 0, };
	short col[3] = { G_BLACK, G_WHITE, 0 };
	PXY rec[2];

	typeof(vrt_cpyfm) * cpyfm = vrt_cpyfm;
	int mode = MD_TRANS;

	if (wind->Properties)
	{
		PIXMAP *pmap;

		if ((pmap = wind->Properties->IconPmap))
		{
			icon = PmapMFDB(pmap);
			if (pmap->Depth != 1)
			{
				cpyfm = (typeof(vrt_cpyfm) *) vro_cpyfm;
				mode = S_OR_D;
			}
			if ((pmap = wind->Properties->IconMask) && (pmap->Depth == 1))
			{
				mask = PmapMFDB(pmap);
			}
		}
	}
	WindUpdate(xTrue);
	wind_get_work(wind->Handle, &work);
	pxy[2] = icon->fd_w - 1;
	pxy[3] = icon->fd_h - 1;
	pxy[4] = work.g_x + ((work.g_w - icon->fd_w) / 2);
	pxy[5] = work.g_y + ((work.g_h - icon->fd_h) / 2);
	pxy[6] = pxy[4] + icon->fd_w - 1;
	pxy[7] = pxy[5] + icon->fd_h - 1;
	rec[1].p_x = (rec[0].p_x = work.g_x) + work.g_w - 1;
	rec[1].p_y = (rec[0].p_y = work.g_y) + work.g_h - 1;

	if (!clip || GrphIntersect(&work, clip))
	{
		vswr_mode(GRPH_Vdi, MD_REPLACE);
		vsf_color(GRPH_Vdi, G_LWHITE);
		v_hide_c(GRPH_Vdi);
		wind_get_first(wind->Handle, &sect);
		while (sect.g_w > 0 && sect.g_h > 0)
		{
			if (GrphIntersect(&sect, &work))
			{
				sect.g_w += sect.g_x - 1;
				sect.g_h += sect.g_y - 1;
				vs_clip_pxy(GRPH_Vdi, (PXY *) & sect);
				v_bar(GRPH_Vdi, (short *) rec);
				if (mask)
					vrt_cpyfm(GRPH_Vdi, MD_TRANS, pxy, mask, &screen, col + 1);
				(*cpyfm) (GRPH_Vdi, mode, pxy, icon, &screen, col);
			}
			wind_get_next(wind->Handle, &sect);
		}
		vs_clip_off(GRPH_Vdi);
		v_show_c(GRPH_Vdi, 1);
	}
	WindUpdate(xFalse);
}


/* ============================================================================== */
void WmgrWindName(WINDOW *wind, const char *name, BOOL windNicon)
{
	if (wind->GwmIcon == windNicon)
	{
		wind_set_str(wind->Handle, WF_NAME, name);
	}
}

/* ============================================================================== */
void WmgrWindIcon(WINDOW *wind)
{
	if (wind->GwmIcon)
	{
		_Wmgr_DrawIcon(wind, NULL);
	}
}


/* ============================================================================== */
void WmgrCursor(WINDOW *wind, PXY *pos)
{
	CARD16 type;
	PXY pos_;

	if (!wind || (MAIN_KeyButMask & K_ALT)
		|| (!(MAIN_KeyButMask & K_CTRL) && wind->Properties && wind->Properties->FixedSize))
	{
		type = 0x1111;
	} else
	{									/* (wind || pos) */
		if (!pos)
		{
			pos = &pos_;
			*pos = WindPointerPos(wind);
		}
		type = 0x0000;
		if (pos->p_y >= 0)
		{
			if (pos->p_x < 0)
				type = 0x100;
			else if (pos->p_x >= wind->Rect.g_w)
				type = 0x001;
			if (pos->p_y >= wind->Rect.g_h)
				type |= 0x010;
		}
	}
	if (type != WMGR_Cursor)
	{
		switch (type)
		{
		case 0x0100:
			CrsrSelect(&_WMGR_Cursor.L);
			break;
		case 0x0110:
			CrsrSelect(&_WMGR_Cursor.LD);
			break;
		case 0x0010:
			CrsrSelect(&_WMGR_Cursor.D);
			break;
		case 0x0011:
			CrsrSelect(&_WMGR_Cursor.RD);
			break;
		case 0x0001:
			CrsrSelect(&_WMGR_Cursor.R);
			break;
		case 0x1111:
			CrsrSelect(&_WMGR_Cursor.X);
			break;
		default:
			CrsrSelect(NULL);
		}
		WMGR_Cursor = type;
	}
}

/* ============================================================================== */
void WmgrCursorOff(CURSOR *new_crsr)
{
	CrsrSelect(new_crsr);
	WMGR_Cursor = 0x0000;
}


/* ------------------------------------------------------------------------------ */
static short desktop_fill(PARMBLK *pblk)
{
	pblk->pb_w += pblk->pb_x - 1;
	pblk->pb_h += pblk->pb_y - 1;
	pblk->pb_wc += pblk->pb_xc - 1;
	pblk->pb_hc += pblk->pb_yc - 1;

	if (GrphIntersectP((PRECT *) & pblk->pb_xc, (PRECT *) & pblk->pb_x))
	{
		vswr_mode(GRPH_Vdi, MD_REPLACE);
		vsf_color(GRPH_Vdi, pblk->pb_parm);
		v_bar(GRPH_Vdi, (short *) &pblk->pb_xc);
	}
	return pblk->pb_currstate;
}

/* ------------------------------------------------------------------------------ */
static short desktop_pmap(PARMBLK *pblk)
{
	pblk->pb_w += pblk->pb_x - 1;
	pblk->pb_h += pblk->pb_y - 1;
	pblk->pb_wc += pblk->pb_xc - 1;
	pblk->pb_hc += pblk->pb_yc - 1;

	if (GrphIntersectP((PRECT *) & pblk->pb_xc, (PRECT *) & pblk->pb_x))
	{
		WindDrawPmap(WIND_Root.Back.Pixmap, *(PXY *) & pblk->pb_x, (PRECT *) & pblk->pb_xc);
	}
	return pblk->pb_currstate;
}

/* ============================================================================== */
OBJECT WMGR_Desktop = { -1, -1, -1, 000, OF_LASTOB, OS_NORMAL, {0l}, 0, 0, };

void WmgrSetDesktop(BOOL onNoff)
{
	static USERBLK ublk;

	if (onNoff)
	{
		WIND_UPDATE_BEG;
		*(GRECT *) & WMGR_Desktop.ob_x = WIND_Root.Rect;
		if (WIND_Root.hasBackPix)
		{
			ublk.ub_code = desktop_pmap;
			WMGR_Desktop.ob_type = G_USERDEF;
			WMGR_Desktop.ob_spec.userblk = &ublk;
		} else if (WIND_Root.Back.Pixel >= 16)
		{
			ublk.ub_code = desktop_fill;
			ublk.ub_parm = WIND_Root.Back.Pixel;
			WMGR_Desktop.ob_type = G_USERDEF;
			WMGR_Desktop.ob_spec.userblk = &ublk;
		} else
		{
			WMGR_Desktop.ob_type = G_BOX;
			WMGR_Desktop.ob_spec.index = 0x000011F0L | WIND_Root.Back.Pixel;
		}
		wind_set(0, WF_NEWDESK, (long) &WMGR_Desktop >> 16, (short) (long) &WMGR_Desktop, 0, 0);
		WIND_UPDATE_END;

	} else if (WMGR_Desktop.ob_type)
	{
		wind_set(0, WF_NEWDESK, 0, 0, 0, 0);
		WMGR_Desktop.ob_type = 000;
	}
}


/* ============================================================================== */
BOOL WmgrMenu(short title, short entry, short meta)
{
	BOOL trigger = xFalse;

	switch (entry)
	{

	case MENU_ABOUT:
		{
			OBJECT *form = NULL;
			GRECT rect;

			if (rsrc_gaddr(R_TREE, ABOUT, &form) && form)
			{
				GRECT sbox = WIND_Root.Rect;

				sprintf(form[ABOUT_VERSN].ob_spec.tedinfo->te_ptext, "Version %s", GLBL_Version);
				form[ABOUT_BUILD].ob_spec.tedinfo->te_ptext = (char *) GLBL_Build;
				form[ABOUT_OK].ob_state &= ~OS_SELECTED;

				form_center_grect(form, &rect);
				form_dial_grect(FMD_SHRINK, &rect, &sbox);
				form_dial_grect(FMD_START, &rect, &rect);
				objc_draw(form, ROOT, MAX_DEPTH, rect.g_x, rect.g_y, rect.g_w, rect.g_h);
				form_do(form, 0);
				objc_offset(form, ABOUT_OK, &sbox.g_x, &sbox.g_y);
				sbox.g_w = form[ABOUT_OK].ob_width;
				sbox.g_h = form[ABOUT_OK].ob_height;
				form_dial_grect(FMD_SHRINK, &sbox, &rect);
				form_dial_grect(FMD_FINISH, &rect, &rect);
			}
		} break;

	case MENU_GWM:
		WmgrActivate(!WMGR_Active);
		WindPointerWatch(xFalse);
		trigger = CLNT_BaseNum == 0;
		break;

	case MENU_QUIT:
		if (CLNT_BaseNum == 0)
		{
			trigger = xTrue;
		} else
		{
			static char const question[] = "[2][Really quit the server|and all runng clients?][ quit |continue]";

			trigger = form_alert(1, question) == 1;
		}
		WMGR_ExitFlag = trigger;
		break;

	case MENU_CLNT_FRST ... MENU_CLNT_LAST:
		{
			char *str = _WMGR_Menu[entry].ob_spec.free_string;

			if (str != _WMGR_MenuEmpty)
			{
				CLIENT *clnt = (CLIENT *) (str - offsetof(CLIENT, Entry));

				if (meta & K_CTRL)
				{
					menu_tnormal(_WMGR_Menu, title, 1);
					title = 0;
					ClntDelete(clnt);
				}
			}
			trigger = CLNT_BaseNum == 0;
		}
		break;
	}
	if (title)
	{
		menu_tnormal(_WMGR_Menu, title, 1);
	}
	return trigger;
}


/* ------------------------------------------------------------------------------ */
static WINDOW *_Wmgr_WindByHandle(short hdl)
{
	WINDOW *wind;

	if (!hdl)
	{
		wind = &WIND_Root;

	} else
	{
		wind = WIND_Root.StackBot;
		while (wind && wind->Handle != hdl)
			wind = wind->NextSibl;
	}
	return wind;
}

/* ------------------------------------------------------------------------------ */
static WINDOW *_Wmgr_WindByPointer(void)
{
	WINDOW *wind = NULL;
	short hdl = wind_find(MAIN_PointerPos->p_x, MAIN_PointerPos->p_y);

	if (hdl >= 0)
	{
		wind = _Wmgr_WindByHandle(hdl);
	}
	return wind;
}

/* ============================================================================== */
void WmgrSetFocus(short focus)
{
	short place = WF_BOTTOM;

	if (WMGR_Focus)
	{
		_Wmgr_SetWidgets(WMGR_Focus, -1);
	}
	if (focus)
	{
		_Wmgr_SetWidgets(focus, +1);
		if (_Wmgr_WindByHandle(wind_get_top()) == NULL)
		{
			place = WF_TOP;
		}
	}
	wind_set(_WMGR_FocusHolder, place, 0, 0, 0, 0);

	WMGR_Focus = focus;
}

/* ------------------------------------------------------------------------------ */
static jmp_buf _jmp_sig_av;
static void _Wmgr_SigAVptr(int sig)
{
	longjmp(_jmp_sig_av, sig);
}

/* ============================================================================== */
BOOL WmgrMessage(short *msg)
{
	static BOOL color_changed = xFalse;

	volatile BOOL reset = xFalse;
	BOOL inv_save = WIND_SaveDone;
	WINDOW *wind;

	switch (msg[0])
	{
	case WM_REDRAW:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			GRECT *rect = (GRECT *) (msg + 4);

			if (WIND_SaveDone
				&& rect->g_x >= WIND_SaveArea->lu.p_x
				&& rect->g_y >= WIND_SaveArea->lu.p_y
				&& rect->g_x + rect->g_w - 1 <= WIND_SaveArea->rd.p_x
				&& rect->g_y + rect->g_h - 1 <= WIND_SaveArea->rd.p_y)
			{
				inv_save = xFalse;
			} else
			{
				if (wind->GwmIcon)
					_Wmgr_DrawIcon(wind, rect);
				else
					WindDrawSection(wind, rect);
			}
		}
		break;

	case WM_MOVED:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			wind_set_curr(msg[3], (GRECT *) (msg + 4));
			if (!wind->GwmIcon)
			{
				CARD32 above = (wind->PrevSibl ? wind->PrevSibl->Id : None);
				GRECT work;

				wind_get_work(msg[3], &work);
				work.g_x -= WIND_Root.Rect.g_x - WMGR_DECOR;
				work.g_y -= WIND_Root.Rect.g_y;
				wind->Rect.g_x = work.g_x;
				wind->Rect.g_y = work.g_y;
				work.g_x -= wind->BorderWidth;
				work.g_y -= wind->BorderWidth;
				work.g_w = wind->Rect.g_w;
				work.g_h = wind->Rect.g_h;
				EvntConfigureNotify(wind, wind->Id, above, &work, wind->BorderWidth, wind->Override);
			}
			WindPointerWatch(xFalse);
		}
		break;

	case WM_CLOSED:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			CLIENT *clnt = ClntFind(wind->Id);

			if (clnt)
			{
				if (wind->Properties && (wind->Properties->wm_protocols & ProtoWmDeleteWindow))
				{
					static Atom const data[5] = { WM_DELETE_WINDOW, 0, 0, 0, 0 };
					EvntClientMsg(clnt, wind->Id, WM_PROTOCOLS, 32, data);
				} else
				{
					reset = !ClntDelete(clnt) && !WMGR_Active;
				}
			}
		}
		break;

	case WM_TOPPED:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			if (color_changed)
			{
				CmapPalette(0);
				color_changed = xFalse;
			}
			_WMGR_HasFocus = (WMGR_OpenCounter > 1 ? 2 : 1);
			WindCirculate(wind, PlaceOnTop);
		}
		break;

	case WM_BOTTOMED:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			_WMGR_HasFocus = 0;
			WindCirculate(wind, PlaceOnBottom);
		}
		break;

	case WM_ONTOP:
		if (color_changed)
		{
			CmapPalette(0);
			color_changed = xFalse;
		}
		if (msg[3] == _WMGR_FocusHolder)
		{
			WINDOW *w = WIND_Root.StackTop;

			while (w != NULL)
			{
				if (w->isMapped || w->GwmIcon)
				{
					wind_set(w->Handle, WF_TOP, 0, 0, 0, 0);
					_WMGR_HasFocus++;
					break;
				} else
				{
					w = w->PrevSibl;
				}
			}
			wind_set(_WMGR_FocusHolder, WF_BOTTOM, 0, 0, 0, 0);
		} else
		{
			_WMGR_HasFocus++;
		}
		WindPointerWatch(xFalse);
		break;

	case WM_UNTOPPED:
		if (msg[3] != _WMGR_FocusHolder)
		{
			_WMGR_HasFocus--;
		}
	case 22360:						/* winshade */
	case 22361:						/* winunshade */
		WindPointerWatch(xFalse);
		break;

	case WM_ICONIFY:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			if (wind->Properties && wind->Properties->IconName)
			{
				wind_set_str(wind->Handle, WF_NAME, wind->Properties->IconName);
			}
			if (msg[3] == WMGR_Focus)
			{
				WMGR_Focus = 0;
			}
			_Wmgr_SetWidgets(msg[3], 0);
			WindClrMapped(wind, xFalse);
			wind_set_grect(msg[3], WF_ICONIFY, (GRECT *) (msg + 4));
			wind_set(msg[3], WF_BEVENT, 0x0000, 0, 0, 0);
			wind_set(_WMGR_FocusHolder, WF_BOTTOM, 0, 0, 0, 0);
			wind->GwmIcon = xTrue;
			WindClrMapped(wind, xFalse);
			if (!--WMGR_OpenCounter)
			{
				wind_close(_WMGR_FocusHolder);
			}
			WindPointerWatch(xFalse);
		}
		break;

	case WM_UNICONIFY:
		if ((wind = _Wmgr_WindByHandle(msg[3])) != NULL)
		{
			GRECT curr;

			if (WmgrWindMap(wind, &curr))
				WindPointerWatch(xFalse);
			else
				MainSetWatch(&curr, MO_ENTER);
		}
		break;

	case COLORS_CHANGED:
		color_changed = xTrue;
		break;

	case 0x4711: /* VA_START */
		if (*(char **) (msg + 3) == NULL)
		{
			x_printf("\033pIgnored\033q empty AV_START from #%i.\n", msg[1]);
		} else
		{
			signal(SIGBUS, _Wmgr_SigAVptr);
			signal(SIGSEGV, _Wmgr_SigAVptr);
			if (setjmp(_jmp_sig_av))
			{
				char buf[150];
				char name[50];
				short typ, pid;

				if (appl_search(-msg[1], buf, &typ, &pid))
				{
					char *b = buf;
					char *e;

					while (*b == ' ')
						b++;
					e = strchr(b, '\0');
					while (*(--e) == ' ')
						;
					e[1] = '\0';
					sprintf(name, " (%i,'%s')", pid, b);
				}
				sprintf(buf, "[1]"
						"[ Application #%i%s"
						"| sent illegal pointer %p"
						"| in AV_START message.| ][Bummer!]", msg[1], name, *(char **) (msg + 3));
				form_alert(1, buf);

				x_printf("\033pBUMMER\033q"
					" Application #%i sent illegal pointer %p in AV_START.\n", msg[1], *(char **) (msg + 3));
			} else
			{
				char *str = *(char **) (msg + 3);
				char *arg = strchr(str, ' ');
				char **argv = (arg ? alloca(strlen(arg) / 2) : NULL);
				int argc = 0;
				size_t len = (arg ? arg - str : strlen(str));
				char *prg = alloca(len);

				((char *) memcpy(prg, str, len))[len] = '\0';
				while ((str = arg) != NULL)
				{
					while (*(++str) == ' ')
						;
					if ((arg = strchr(str, ' ')))
						len = arg - str;
					else
						len = strlen(str);
					if (!len || !(argv[argc] = malloc(len)))
						break;
					((char *) memcpy(argv[argc++], str, len))[len] = '\0';
				}

				x_printf("VA_START '%s'\n", *(char **) (msg + 3));
				msg[0] = 0x4738;		/* AV_STARTED */
				appl_write(msg[1], 16, msg);

				WmgrLaunch(prg, argc, (const char **) argv);
				while (argc)
					free(argv[--argc]);
			}
		}
		break;

	default:
		x_printf("event #%i(%X) by #%i = %i/%X, %04X,%04X,%04X,%04X \n",
			msg[0], MAIN_KeyButMask, msg[1], msg[3], msg[3], msg[4], msg[5], msg[6], msg[7]);
		break;
	}

	if (inv_save)
		WindSaveFlush(xFalse);

	return reset;
}

/* ============================================================================== */
BOOL WmgrButton(WINDOW *wind)
{
	BOOL move = (wind != NULL);
	BOOL size = xFalse;

	if (!wind && !(wind = (WMGR_Cursor ? _Wmgr_WindByPointer() : NULL)))
	{
		return xFalse;

	} else
	{
		EVMULT_IN ev_i = {
			MU_BUTTON | MU_M1 | MU_TIMER, 1, 0x03, 0x00,
			MO_LEAVE, {MAIN_PointerPos->p_x, MAIN_PointerPos->p_y, 1, 1},
			0, {0, 0, 0, 0}, 20, 0
		};
		EVMULT_OUT ev_o;
		short ev;
		short dummy[8];
		CARD16 cursor;
		PXY pc[5];
		PXY pw[5];
		int c_l, c_r, c_u, c_d;
		int w_l, w_r, w_u, w_d;
		int mx, my;
		int ml = 0;
		int mu = 0;
		int mr = WIND_Root.Rect.g_w;
		int md = WIND_Root.Rect.g_h;
		int magnet = 0x1111;
		GRECT magn;

		if (move)
		{
			if (!WMGR_Cursor)
			{
				WmgrCursor(NULL, NULL);
			}
		} else
		{								/* (!move) */
			move = (WMGR_Cursor == 0x1111);
			size = !move && (MAIN_KeyButMask & K_CTRL);
		}
		cursor = WMGR_Cursor;

		wind_get_curr(wind->Handle, (GRECT *) pc);
		pc[2].p_x = pc[1].p_x += (pc[3].p_x = pc[4].p_x = pc[0].p_x) - 1;
		pc[2].p_y = pc[3].p_y = pc[0].p_y + pc[1].p_y - 1;
		pc[4].p_y = (pc[1].p_y = pc[0].p_y) + 1;
		pw[1].p_x = pw[2].p_x = wind->Rect.g_x + WIND_Root.Rect.g_x;
		pw[4].p_y = (pw[0].p_y = pw[1].p_y = wind->Rect.g_y + WIND_Root.Rect.g_y) + 1;
		pw[0].p_x = pw[3].p_x = pw[4].p_x = pw[1].p_x + wind->Rect.g_w - 1;
		pw[2].p_y = pw[3].p_y = pw[1].p_y + wind->Rect.g_h - 1;
		c_l = pc[0].p_x - MAIN_PointerPos->p_x;
		c_u = pc[1].p_y - MAIN_PointerPos->p_y;
		c_r = pc[2].p_x - MAIN_PointerPos->p_x;
		c_d = pc[3].p_y - MAIN_PointerPos->p_y;
		w_l = pw[1].p_x - MAIN_PointerPos->p_x;
		w_u = pw[0].p_y - MAIN_PointerPos->p_y;
		w_r = pw[3].p_x - MAIN_PointerPos->p_x;
		w_d = pw[2].p_y - MAIN_PointerPos->p_y;

		if (!move)
		{
			int min_x = 0;
			int min_y = 0;
			PROPERTIES *pool;

			if (!size && (pool = wind->Properties))
			{
				if (pool->Base.valid)
				{
					min_x = pool->Base.Size.p_x;
					min_y = pool->Base.Size.p_y;
					if (pool->Inc.valid)
					{
						min_x *= pool->Inc.Step.p_x;
						min_y *= pool->Inc.Step.p_y;
					}
				} else if (pool->Min.valid)
				{
					min_x = pool->Min.Size.p_x;
					min_y = pool->Min.Size.p_y;
				}
				if (pool->Max.valid)
				{
					if (cursor & 0x0001)
						mr = pw[1].p_x + pool->Max.Size.p_x - w_r;
					else if (cursor & 0x0100)
						ml = pw[3].p_x - pool->Max.Size.p_x - w_l;
					if (cursor & 0x0010)
						md = pw[0].p_y + pool->Max.Size.p_y - w_d;
				}
			}
			if (cursor & 0x0001)
				ml = pw[1].p_x + min_x - w_r;
			else if (cursor & 0x0100)
				mr = pw[3].p_x - min_x - w_l;
			if (cursor & 0x0010)
				mu = pw[0].p_y + min_y - w_d;

		} else
		{								/* (move == xTrue) */
			mu = WIND_Root.Rect.g_y - c_u - 1;
			magn.g_x = WIND_Root.Rect.g_x - c_l;
			magn.g_w = WIND_Root.Rect.g_x + WIND_Root.Rect.g_w - c_r - magn.g_x;
			magn.g_y = -1;
			magn.g_h = WIND_Root.Rect.g_y + WIND_Root.Rect.g_h - c_d - magn.g_y;
			ev_i.emi_m2leave = MO_LEAVE;
			ev_i.emi_m2 = magn;
			ev_i.emi_flags |= MU_M2;
		}

		vswr_mode(GRPH_Vdi, MD_XOR);
		vsl_color(GRPH_Vdi, G_BLACK);
		vsl_udsty(GRPH_Vdi, 0xAAAA);
		vsl_type(GRPH_Vdi, USERLINE);
		v_hide_c(GRPH_Vdi);
		do
		{
			v_pline(GRPH_Vdi, 5, (short *) pc);
			v_pline(GRPH_Vdi, 5, (short *) pw);
			v_show_c(GRPH_Vdi, 1);
			ev = evnt_multi_fast(&ev_i, dummy, &ev_o);
			mx = (ev_o.emo_mouse.p_x <= ml ? ml : ev_o.emo_mouse.p_x >= mr ? mr : ev_o.emo_mouse.p_x);
			my = (ev_o.emo_mouse.p_y <= mu ? mu : ev_o.emo_mouse.p_y >= md ? md : ev_o.emo_mouse.p_y);
			v_hide_c(GRPH_Vdi);
			v_pline(GRPH_Vdi, 5, (short *) pw);
			v_pline(GRPH_Vdi, 5, (short *) pc);
			if (ev & MU_TIMER)
			{
				ev_i.emi_flags &= ~MU_TIMER;
			}
			if (ev & MU_M2)
			{
				if (ev_i.emi_m2leave == MO_ENTER)
				{
					ev_i.emi_m2leave = MO_LEAVE;
				} else
				{
					if (mx < magn.g_x)
					{
						ev_i.emi_m2.g_x = magn.g_x - WMGR_DECOR * 2 + 1;
						ev_i.emi_m2.g_w = WMGR_DECOR * 2;
						magnet = 0x1010;
					} else if (mx >= magn.g_x + magn.g_w)
					{
						ev_i.emi_m2.g_x = magn.g_x + magn.g_w;
						ev_i.emi_m2.g_w = WMGR_DECOR * 2;
						magnet = 0x1010;
					} else
					{
						ev_i.emi_m2.g_x = magn.g_x;
						ev_i.emi_m2.g_w = magn.g_w;
						magnet = 0x1111;
					}
					if (my >= magn.g_y + magn.g_h)
					{
						ev_i.emi_m2.g_y = magn.g_y + magn.g_h;
						ev_i.emi_m2.g_h = WMGR_DECOR * 2;
						magnet &= ~0x1010;
					} else
					{
						ev_i.emi_m2.g_y = magn.g_y;
						ev_i.emi_m2.g_h = magn.g_h;
					}
					if (magnet != 0x1111)
					{
						PXY m = { mx, my };
						if (!PXYinRect(&m, &ev_i.emi_m2))
						{
							ev_i.emi_m2leave = MO_ENTER;
							ev_i.emi_m2 = magn;
							magnet = 0x1111;
						}
					}
				}
			}
			if (ev & MU_M1)
			{
				cursor = WMGR_Cursor & magnet;
				if (cursor & 0x1000)
				{
					pc[4].p_y = (pc[0].p_y = pc[1].p_y = my + c_u) + 1;
					pw[4].p_y = (pw[0].p_y = pw[1].p_y = my + w_u) + 1;
				}
				if (cursor & 0x0010)
				{
					pc[2].p_y = pc[3].p_y = my + c_d;
					pw[2].p_y = pw[3].p_y = my + w_d;
				}
				if (cursor & 0x0100)
				{
					pc[0].p_x = pc[3].p_x = pc[4].p_x = mx + c_l;
					pw[1].p_x = pw[2].p_x = mx + w_l;
				}
				if (cursor & 0x0001)
				{
					pc[1].p_x = pc[2].p_x = mx + c_r;
					pw[0].p_x = pw[3].p_x = pw[4].p_x = mx + w_r;
				}
				ev_i.emi_m1.g_x = ev_o.emo_mouse.p_x;
				ev_i.emi_m1.g_y = ev_o.emo_mouse.p_y;
			}
		} while (!(ev & MU_BUTTON));
		v_show_c(GRPH_Vdi, 1);
		vsl_type(GRPH_Vdi, SOLID);

		if (ev_i.emi_flags & MU_TIMER)
		{
			WindCirculate(wind, (wind_get_top() != wind->Handle ? PlaceOnTop : PlaceOnBottom));

		} else if (move)
		{
			if (pw[1].p_x != wind->Rect.g_x + WIND_Root.Rect.g_x || pw[1].p_y != wind->Rect.g_y + WIND_Root.Rect.g_y)
			{
				short msg[8] = {
					WM_MOVED, gl_apid, 0, wind->Handle,
					pc[0].p_x, pc[0].p_y, pc[2].p_x - pc[0].p_x + 1, pc[2].p_y - pc[0].p_y + 1
				};
				WmgrMessage(msg);
			}

		} else
		{
			pw[2].p_x = pw[1].p_x - WIND_Root.Rect.g_x - wind->Rect.g_x;
			pw[2].p_y = 0;
			pw[3].p_x -= pw[1].p_x - 1 + wind->Rect.g_w;
			pw[3].p_y -= pw[1].p_y - 1 + wind->Rect.g_h;
			if (pw[2].p_x || pw[3].p_x || pw[3].p_y)
			{
				WindResize(wind, (GRECT *) (pw + 2));
			}
			WindPointerWatch(xFalse);
		}
	}
	return xTrue;
}

/* ============================================================================== */
void WmgrKeybd(short chng_meta)
{
	if (chng_meta & MAIN_KeyButMask & K_ALT)
	{
		WmgrCursor(NULL, NULL);
	} else
	{
		WINDOW *wind = _Wmgr_WindByPointer();

		if (wind)
			WmgrCursor(wind, NULL);
	}
}


/* ------------------------------------------------------------------------------ */
static void FT_Wmgr_reply(p_CLIENT clnt, CARD32 size, const char *form)
{
	int req = (_WMGR_Client.iBuf.Mem ? ((xReq *) _WMGR_Client.iBuf.Mem)->reqType : -1);

	_WMGR_Obuf.generic.type = X_Reply;
	_WMGR_Obuf.generic.sequenceNumber = _WMGR_Client.SeqNum;
	_WMGR_Obuf.generic.length = Units(size - sizeof(xReply));

	_WMGR_Client.oBuf.Done = Align(size);

	PRINT(0, "reply to '%s'",
		  (req < 0 ? "<n/a>" : req > 0 && req < FirstExtensionError ? RequestTable[req].Name : "<invalid>"));
}

/* ------------------------------------------------------------------------------ */
static void FT_Wmgr_error(p_CLIENT c, CARD8 code, CARD8 majOp, CARD16 minOp, CARD32 val)
{
}

/* ------------------------------------------------------------------------------ */
static void FT_Wmgr_event(p_CLIENT c, p_WINDOW w, const xEvent *ev)
{
	FT_Evnt_send_Unswapped(&_WMGR_Client, w, ev);
	_WMGR_Client.oBuf.Done = _WMGR_Client.oBuf.Left;
	_WMGR_Client.oBuf.Left = 0;
}
