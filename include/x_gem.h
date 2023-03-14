/*
 *==============================================================================
 *
 * x_gem.h -- extensions to the gem-lib.
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-05 - Initial Version.
 *==============================================================================
 */
#ifndef __X_GEM_H__
#define __X_GEM_H__

#include <gem.h>
#include "types.h"

#define K_LOCK    0x10					// bitmask returned by Kbshift()
#define K_ALTGR   0x80					/* bitmask returned by Kbshift() */
#define K_XALTGR  0x20			/* bitmask used internally */

#define ApplId() (gl_apid)

#define wind_get_curr(h,r)        wind_get_grect (h, WF_CURRXYWH,      r)
#define wind_get_first(h,r)       wind_get_grect (h, WF_FIRSTXYWH,     r)
#define wind_get_full(h,r)        wind_get_grect (h, WF_FULLXYWH,      r)
#define wind_get_next(h,r)        wind_get_grect (h, WF_NEXTXYWH,      r)
#define wind_get_prev(h,r)        wind_get_grect (h, WF_PREVXYWH,      r)
#define wind_get_uniconify(h,r)   wind_get_grect (h, WF_UNICONIFYXYWH, r)
#define wind_get_work(h,r)        wind_get_grect (h, WF_WORKXYWH,      r)
short wind_get_one(int WindowHandle, int What);

#define wind_get_bottom()         wind_get_one   (0, WF_BOTTOM)
#define wind_get_top()            wind_get_one   (0, WF_TOP)
#define wind_get_bevent(h)        wind_get_one   (h, WF_BEVENT)
#define wind_get_hslide(h)        wind_get_one   (h, WF_HSLIDE)
#define wind_get_hslsize(h)       wind_get_one   (h, WF_HSLSIZE)
#define wind_get_kind(h)          wind_get_one   (h, WF_KIND)
#define wind_get_vslide(h)        wind_get_one   (h, WF_VSLIDE)
#define wind_get_vslsize(h)       wind_get_one   (h, WF_VSLSIZE)
#define wind_set_curr(h,r)        wind_set_grect (h, WF_CURRXYWH, r)
short wind_set_proc(int WindowHandle, CICONBLK *icon);

#define WIND_UPDATE_BEG           wind_update (BEG_UPDATE); {
#define WIND_UPDATE_END           } wind_update (END_UPDATE);

extern short _app;

#ifndef COLORS_CHANGED
#define COLORS_CHANGED   84
#endif

#if __GEMLIB_MINOR__ >= 44
#define v_gtext16n(hdl, pos, wstr, num) v_gtext16n(hdl, (pos).p_x, (pos).p_y, wstr, num)
#endif

#endif /* __X_GEM_H__ */
