/*
 *==============================================================================
 *
 * wind_util.c -- Several helper functions for window handling.
 *
 * Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2001-01-22 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>

#include "window_P.h"
#include "Cursor.h"

#include <X11/Xproto.h>


/* ============================================================================== */
BOOL WindVisible(WINDOW *wind)
{
	/* Returns True if the window and all of its anchestors are mapped. */
	/*........................................................................... */

	BOOL vis;

	while ((vis = wind->isMapped) && (wind = wind->Parent) != NULL)
		;

	return vis;
}


/* ============================================================================== */
PXY WindOrigin(WINDOW *wind)
{
	/* Returns the origin of the window in absolute screen coordinates. */
	/*........................................................................... */
	PXY orig;

	orig.p_x = wind->Rect.g_x;
	orig.p_y = wind->Rect.g_y;
	while ((wind = wind->Parent) != NULL)
	{
		orig.p_x += wind->Rect.g_x;
		orig.p_y += wind->Rect.g_y;
	}

	return orig;
}

/* ============================================================================== */
short WindGeometry(WINDOW *wind, GRECT *dst, CARD16 border)
{
	/*
	 * Sets dst to the rectangle of the window in absolute screen coordinates,
	 * including the given border width, and returns the AES handle of its
	 * anchestor that is a top window.
	 *..........................................................................
	 */
	PXY org = WindOrigin(wind);
	dst->g_x = org.p_x;
	dst->g_y = org.p_y;
	dst->g_w = wind->Rect.g_w;
	dst->g_h = wind->Rect.g_h;
	if (border)
	{
		dst->g_x -= border;
		dst->g_y -= border;
		border <<= 1;
		dst->g_w += border;
		dst->g_h += border;
	}
	return WindHandle(wind);
}


/* ============================================================================== */
PXY WindPointerPos(WINDOW *wind)
{
	/* Returns the mouse pointer position relative to the window. */
	/*........................................................................... */

	PXY pos = *MAIN_PointerPos;

	if (!wind)
	{
		pos.p_x -= WIND_Root.Rect.g_x;
		pos.p_y -= WIND_Root.Rect.g_y;

	} else
		do
		{
			pos.p_x -= wind->Rect.g_x;
			pos.p_y -= wind->Rect.g_y;
		} while ((wind = wind->Parent));

	return pos;
}


/* ============================================================================== */
void WindSetHandles(WINDOW *wind)
{
	/* Set Handle of all children to the negative value of their top-window */
	/* anchestor. */
	/*........................................................................... */

	short hdl = (wind->Handle > 0 ? -wind->Handle : wind->Handle);
	short n = 1;
	BOOL b = xTrue;

	if ((wind = wind->StackBot))
	{
		do
		{
			if (b)
			{
				wind->Handle = hdl;
				while (wind->StackBot)
				{
					wind = wind->StackBot;
					n++;
				}
			}
			if (wind->NextSibl)
			{
				wind = wind->NextSibl;
				b = xTrue;
			} else if (--n)
			{
				wind = wind->Parent;
				b = xFalse;
			}
		} while (n);
	}
}


/* ============================================================================== */
CARD32 WindChildOf(WINDOW *wind, WINDOW *candid)
{
	/* If candid is an inferior of wind returns the child of wind that is an */
	/* anchestor of (or is) candid.  Otherwise returns 'None'. */
	/*........................................................................... */

	do
	{
		if (candid->Parent == wind)
		{
			return candid->Id;
		}
	} while ((candid = candid->Parent));

	return None;
}


/* ============================================================================== */
/* */
/* The following functions are private to  wind..-modules. */

/* ------------------------------------------------------------------------------ */
BOOL _Wind_IsInferior(WINDOW *wind, WINDOW *inferior)
{
	/* Returns True if wind is an anchestor of or is itself the inferior. */
	/*........................................................................... */

	while (inferior)
	{
		if (inferior == wind)
			return xTrue;
		inferior = inferior->Parent;
	}
	return xFalse;
}

/* ------------------------------------------------------------------------------ */
void _Wind_Cursor(WINDOW *wind)
{
	/* If window has a mouse cursor struct stored, set the mouse form to this. */
	/* Else find the next anchestor, which has one stored and uses that. */
	/*........................................................................... */

	CURSOR *crsr = NULL;

	while (wind && !(crsr = wind->Cursor))
	{
		wind = wind->Parent;
	}
	CrsrSelect(crsr);
}

/* ------------------------------------------------------------------------------ */
int _Wind_PathStack(WINDOW **stack, int *anc, WINDOW *beg, WINDOW *end)
{
	/*
	 * Flatten the hirachycal relationship from window beg to window end into the
	 * stack pointer list and returns the last valid stack entry (not number of!)
	 * anc is set to:
	 * anc == 0:   beg is an anchestor of end
	 * anc == top: beg is an inferior of end
	 * else:       stack[anc] is pointing to the least common anchestor of beg
	 *             and end
	 *...........................................................................
	 */
	int top = 0;

	if ((stack[0] = beg) == NULL)
	{
		stack[++top] = &WIND_Root;
	} else
	{
		while ((beg = beg->Parent) != NULL)
		{
			stack[++top] = beg;
		}
	}
	*anc = top;

	if (end)
	{
		WINDOW *r_stk[32];
		int num = 0;

		do
		{
			int n = top;

			do
			{
				if (end == stack[n])
				{
					*anc = top = n;
					while (num--)
					{
						stack[++top] = r_stk[num];
					}
					return top;
				}
			} while (n--);
			r_stk[num++] = end;
		} while ((end = end->Parent));
	}
	return top;
}
