//==============================================================================
//
// wind_draw.c
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-07 - Module released for beta state.
// 2000-09-19 - Initial Version.
//==============================================================================
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "window_P.h"
#include "event.h"
#include "gcontext.h"
#include "grph.h"
#include "pixmap.h"
#include "wmgr.h"
#include "x_gem.h"

#include <X11/Xproto.h>


//==============================================================================
void
WindClipOff (void)
{
	WindUpdate (xFalse);
}

//------------------------------------------------------------------------------
static CARD16
clip_children (WINDOW * wind, PXY orig, PRECT * dst, PRECT * clip)
{
	CARD16 num = 0, cnt = 0, i;
	
	wind = wind->StackBot;
	while (wind) {
		if (wind->isMapped) {
			register int b = wind->BorderWidth;
			dst->rd.p_x = (dst->lu.p_x = orig.p_x + wind->Rect.g_x) + wind->Rect.g_w -1;
			dst->rd.p_y = (dst->lu.p_y = orig.p_y + wind->Rect.g_y) + wind->Rect.g_h -1;
			if (b) {
				dst->lu.p_x -= b;
				dst->lu.p_y -= b;
				dst->rd.p_x += b;
				dst->rd.p_y += b;
			}
			if (GrphIntersectP (dst, clip)) {
				dst++;
				num++;
			}
		}
		wind = wind->NextSibl;
	}
	if (num) {
		short top, bot;
		short beg, end;
		PRECT ** list = (PRECT**)dst;
		PRECT  * area = (PRECT*)(list + num);

		// sort child geometries into list, first by y, then by x
		
		dst    -= num;
		list[0] = dst++;
		for (i = 1; i < num; i++) {
			short x = dst->lu.p_x;
			short y = dst->lu.p_y;
			int   j = i -1;
			while (j >= 0  &&  list[j]->lu.p_y > y) {
				list[j+1] = list[j];
				j--;
			}
			while (j >= 0  &&  list[j]->lu.p_y == y  &&  list[j]->lu.p_x > x) {
				list[j+1] = list[j];
				j--;
			}
			list[j+1] = dst++;
		}
		dst -= num;
		
		// find free rectangles between child geometries
					
		beg = 0;
		bot = clip->lu.p_y;
		do {
			short x = clip->lu.p_x;
			if (bot < list[beg]->lu.p_y) { // found area above first child geometry
				top = list[beg]->lu.p_y;
				area->lu.p_x = x;
				area->lu.p_y = bot;
				area->rd.p_x = clip->rd.p_x;
				area->rd.p_y = top -1;
				area++;
				cnt++;
			} else {
				top = bot;
			}
			bot = list[beg]->rd.p_y;
			
			for (end = beg +1; end < num; ++end) {
				if (list[end]->lu.p_y > top) {
					if (list[end]->lu.p_y <= bot) bot = list[end]->lu.p_y -1;
					break;
				} else if (list[end]->rd.p_y < bot) {
					bot = list[end]->rd.p_y;
				}
			}
			i = beg;
			while (i < end) {
				if (x < list[i]->lu.p_x) { // free area on left side of child
					area->lu.p_x = x;
					area->lu.p_y = top;
					area->rd.p_x = list[i]->lu.p_x -1;
					area->rd.p_y = bot;
					area++;
					cnt++;
				}
				if (x <= list[i]->rd.p_x) {
					x = list[i]->rd.p_x +1;
				}
				if      (list[i]->rd.p_y > bot) i++;
				else if (i == beg)            beg = ++i;
				else {
					short j = i;
					while (++j < num) list[j-1] = list[j];
					if (--num < end)  end = num;
				}
			}
			if (x <= clip->rd.p_x) { // free area on right side of last child
				area->lu.p_x = x;
				area->lu.p_y = top;
				area->rd.p_x = clip->rd.p_x;
				area->rd.p_y = bot;
				area++;
				cnt++;
			}
			
			if (i > beg) {
				while (i < num  &&  list[i]->lu.p_y == bot
				       &&  list[i]->lu.p_x < list[i-1]->lu.p_x) {
					short   j    = i;
					PRECT * save = list[i];
					do {
						list[j] = list[j-1];
					} while (--j > beg  &&  save->lu.p_x < list[j]->lu.p_x);
					list[j] = save;
				}
				i++;
			}
			bot++; // to be the top for the next loop
		
		} while (beg < num);
		
		if (bot <= clip->rd.p_y) { // free area at the bottom
			area->lu.p_x = clip->lu.p_x;
			area->lu.p_y = bot;
			area->rd   = clip->rd;
			area++;
			cnt++;
		}
		
		if (cnt) {
			memmove (dst, area - cnt, sizeof(PRECT) * cnt);
		}
	
	} else { // no children
		*dst = *clip;
		cnt  = 1;
	}
	
	return cnt;
}

//==============================================================================
CARD16
WindClipLock (WINDOW * wind, CARD16 border, const GRECT * clip, short n_clip,
              PXY * orig, PRECT ** pBuf, BOOL incl_chlds)
{
	WINDOW * pwnd, * twnd = wind;
	GRECT    work = wind->Rect, rect;
	BOOL     visb = wind->isMapped;
	CARD16   nClp = 0;
	PRECT  * sect, * p_clip;
	short    a, b, n;
	short    l = 0x7FFF, u = 0x7FFF, r = 0x8000, d = 0x8000;
	
	if (border) {
		work.g_x -= border;
		work.g_y -= border;
		border *= 2;
		work.g_w += border;
		work.g_h += border;
	}
	if (work.g_x < 0) { work.g_w += work.g_x; work.g_x = 0; }
	if (work.g_y < 0) { work.g_h += work.g_y; work.g_y = 0; }
	
	*orig = *(PXY*)&wind->Rect;
	
	while ((pwnd = twnd->Parent)) {
		if (work.g_x + work.g_w > pwnd->Rect.g_w) work.g_w = pwnd->Rect.g_w - work.g_x;
		if (work.g_y + work.g_h > pwnd->Rect.g_h) work.g_h = pwnd->Rect.g_h - work.g_y;
		if ((work.g_x += pwnd->Rect.g_x) < 0) { work.g_w += work.g_x; work.g_x = 0; }
		if ((work.g_y += pwnd->Rect.g_y) < 0) { work.g_h += work.g_y; work.g_y = 0; }
		orig->p_x += pwnd->Rect.g_x;
		orig->p_y += pwnd->Rect.g_y;
		if (pwnd == &WIND_Root  || !(visb &= pwnd->isMapped)) break;
		twnd = pwnd;
	}
	if (!visb ||  work.g_w <= 0  ||  work.g_h <= 0) return 0;
	
	work.g_w += work.g_x -1;
	work.g_h += work.g_y -1;
	
	if (clip  &&  n_clip > 0) {
		PRECT * c = p_clip = alloca (sizeof(PRECT) * n_clip);
		n         = 0;
		while (n_clip--) {
			c->rd.p_x = (c->lu.p_x = clip->g_x + orig->p_x) + clip->g_w -1;
			c->rd.p_y = (c->lu.p_y = clip->g_y + orig->p_y) + clip->g_h -1;
			clip++;
			if (GrphIntersectP (c, (PRECT*)&work)) {
				c++;
				n++;
			}
		}
		if (!(n_clip = n)) return 0;
	
	} else {
		p_clip = (PRECT*)&work;
		if (clip  &&  n_clip < 0) {
			PRECT c;
			c.rd.p_x = (c.lu.p_x = clip->g_x) + clip->g_w -1;
			c.rd.p_y = (c.lu.p_y = clip->g_y) + clip->g_h -1;
			if (!GrphIntersectP (p_clip, &c)) return 0;
		}
		n_clip = 1;
	}
	
	WindUpdate (xTrue);
	wind_get (0, WF_SCREEN, &a, &b, &n,&n);
	*pBuf = sect = (PRECT*)(((long)a << 16) | (b & 0xFFFF));
	
	wind_get_first (twnd->Handle, &rect);
	while (rect.g_w > 0  &&  rect.g_h > 0) {
		PRECT * c = p_clip;
		n         = n_clip;
		rect.g_w   += rect.g_x -1;
		rect.g_h   += rect.g_y -1;
		do {
			*sect = *(PRECT*)&rect;
			if (GrphIntersectP (sect, c++)) {
				if (l > sect->lu.p_x) l = sect->lu.p_x;
				if (u > sect->lu.p_y) u = sect->lu.p_y;
				if (r < sect->rd.p_x) r = sect->rd.p_x;
				if (d < sect->rd.p_y) d = sect->rd.p_y;
				sect++;
				nClp++;
			}
		} while (--n);
		wind_get_next (twnd->Handle, &rect);
	}
	if (!nClp) {
		*pBuf = NULL;
		WindUpdate (xFalse);
	
	} else if (!incl_chlds && wind->StackBot) {
		PRECT * rct;
		p_clip->lu.p_x = l;
		p_clip->lu.p_y = u;
		p_clip->rd.p_x = r;
		p_clip->rd.p_y = d;
		n   = 0;
		a   = clip_children (wind, *orig, sect, p_clip);
		rct = sect + a;
		while (a--) {
			PRECT * c = *pBuf;
			b         = nClp;
			while (b--) {
				*rct = *sect;
				if (GrphIntersectP (rct, c++)) {
					rct++;
					n++;
				}
			}
			sect++;
		}
		if ((nClp = n)) {
			memcpy (*pBuf, rct - n, n * sizeof(PRECT));
			(*pBuf)[nClp] = *p_clip;
		} else {
			*pBuf = NULL;
			WindUpdate (xFalse);
		}		
		
	} else {
		sect->lu.p_x = l;
		sect->lu.p_y = u;
		sect->rd.p_x = r;
		sect->rd.p_y = d;
	}
	return nClp;
}


//==============================================================================
void
WindDrawPmap (PIXMAP * pmap, PXY orig, p_PRECT sect)
{
	MFDB  scrn = { NULL, };
	PXY   offs = { sect->lu.p_x - orig.p_x, sect->lu.p_y - orig.p_y };
	short rd_x = pmap->W -1, rd_y = pmap->H -1;
	short color[2] = { G_BLACK, G_WHITE };
	PXY   pxy[4];
	#define s_lu pxy[0]
	#define s_rd pxy[1]
	#define d_lu pxy[2]
	#define d_rd pxy[3]
	int  d;
	
	if (offs.p_x >= pmap->W) offs.p_x %= pmap->W;
	if (offs.p_y >= pmap->H) offs.p_y %= pmap->H;
	
	s_lu.p_y = offs.p_y;
	s_rd.p_y = rd_y;
	d_lu.p_y = sect->lu.p_y;
	d_rd   = sect->rd;
	while ((d = d_rd.p_y - d_lu.p_y) >= 0) {
		if (d < rd_y - s_lu.p_y) {
			s_rd.p_y = s_lu.p_y + d;
		}
		s_lu.p_x = offs.p_x;
		s_rd.p_x = rd_x;
		d_lu.p_x = sect->lu.p_x;
		while ((d = d_rd.p_x - d_lu.p_x) >= 0) {
			if (d < rd_x - s_lu.p_x) {
				s_rd.p_x = s_lu.p_x + d;
			}
			if (pmap->Depth == 1) {
				vrt_cpyfm (GRPH_Vdi, MD_REPLACE,
				           (short*)pxy, PmapMFDB(pmap), &scrn, color);
			} else {
				vro_cpyfm (GRPH_Vdi, S_ONLY, (short*)pxy, PmapMFDB(pmap), &scrn);
			}
			d_lu.p_x += pmap->W - s_lu.p_x;
			s_lu.p_x =  0;
		}
		d_lu.p_y += pmap->H - s_lu.p_y;
		s_lu.p_y =  0;
	}
	#undef s_lu
	#undef s_rd
	#undef d_lu
	#undef d_rd
}

//------------------------------------------------------------------------------
int
WindDrawBgnd (WINDOW * wind, PXY orig, PRECT * area,
              PRECT * sect, CARD16 nSct, GRECT * exps)
{
	int cnt = 0;
	
	v_hide_c (GRPH_Vdi);
	
	if (wind->hasBackPix) {
		do {
			PRECT rect = *area;
			if (GrphIntersectP (&rect, sect++)) {
				WindDrawPmap (wind->Back.Pixmap, orig, &rect);
				if (exps) {
					exps->g_w = rect.rd.p_x - rect.lu.p_x +1;
					exps->g_h = rect.rd.p_y - rect.lu.p_y +1;
					exps->g_x = rect.lu.p_x - orig.p_x;
					exps->g_y = rect.lu.p_y - orig.p_y;
					exps++;
				}
				cnt++;
			}
		} while (--nSct);
	
	} else {
		do {
			PRECT rect = *area;
			if (GrphIntersectP (&rect, sect++)) {
				v_bar (GRPH_Vdi, (short*)&rect.lu);
				if (exps) {
					exps->g_w = rect.rd.p_x - rect.lu.p_x +1;
					exps->g_h = rect.rd.p_y - rect.lu.p_y +1;
					exps->g_x = rect.lu.p_x - orig.p_x;
					exps->g_y = rect.lu.p_y - orig.p_y;
					exps++;
				}
				cnt++;
			}
		} while (--nSct);
	}
	v_show_c (GRPH_Vdi, 1);
	
	return cnt;
}

//------------------------------------------------------------------------------
static void
draw_brdr (WINDOW * wind, PRECT * work, PRECT * area, PRECT * sect, int num)
{
	int   i = wind->BorderWidth;
	short l = work->lu.p_x - i;
	short r = work->rd.p_x + i;
	int   n = 0;
	PRECT brdr[4];
	
	brdr[0].lu.p_x = l;
	brdr[0].lu.p_y = work->lu.p_y - i;
	brdr[0].rd.p_x = r;
	brdr[0].rd.p_y = work->lu.p_y - 1;
	if (GrphIntersectP (brdr, area)) n++;
	brdr[n].lu.p_x = l;
	brdr[n].lu.p_y = work->lu.p_y;
	brdr[n].rd.p_x = work->lu.p_x - 1;
	brdr[n].rd.p_y = work->rd.p_y;
	if (GrphIntersectP (brdr + n, area)) n++;
	brdr[n].lu.p_x = work->rd.p_x + 1;
	brdr[n].lu.p_y = work->lu.p_y;
	brdr[n].rd.p_x = work->rd.p_x + i;
	brdr[n].rd.p_y = work->rd.p_y;
	if (GrphIntersectP (brdr + n, area)) n++;
	brdr[n].lu.p_x = l;
	brdr[n].lu.p_y = work->rd.p_y + 1;
	brdr[n].rd.p_x = r;
	brdr[n].rd.p_y = work->rd.p_y + i;
	if (GrphIntersectP (brdr + n, area)) n++;
	else if (!n)                          return;
	
	vsf_color (GRPH_Vdi, wind->BorderPixel);
	v_hide_c  (GRPH_Vdi);
	do {
		vs_clip_pxy (GRPH_Vdi, (PXY*)(sect++));
		for (i = 0; i < n; v_bar (GRPH_Vdi, (short*)&brdr[i++].lu));
	} while (--num);
	v_show_c    (GRPH_Vdi, 1);
	vs_clip_off (GRPH_Vdi);
}

//------------------------------------------------------------------------------
static BOOL
draw_wind (WINDOW * wind, PRECT * work,
           PRECT * area, PRECT * sect, int nClp)
{
	// work: inside area of the window
	// area: clip rectangle to be drawn
	// sect: list of sections of the top-window to be drawn
	// nClp: list length
	
	GRECT * exps = (GRECT*)(wind->u.List.AllMasks & ExposureMask
	                        ? area +2 : NULL);
	PXY orig = work->lu;
	int nEvn = 0;
	
	if (GrphIntersectP (work, area)) {
		if (wind->hasBackGnd) {
			if (!wind->hasBackPix) {
				vsf_color (GRPH_Vdi, wind->Back.Pixel);
			}
			nEvn = WindDrawBgnd (wind, orig, work, sect, nClp, exps);
		
		} else if (exps) {
			do {
				*exps = *(GRECT*)area;
				if (GrphIntersectP ((PRECT*)exps, sect)) {
					exps->g_w -= exps->g_x -1;
					exps->g_h -= exps->g_y -1;
					exps->g_x -= orig.p_x;
					exps->g_y -= orig.p_y;
					exps++;
					nEvn++;
				}
				sect++;
			} while (--nClp);
			exps -= nEvn;
		
		} else {
			nEvn = -1; // return xTrue due to existing intersection
		}
		if (nEvn && exps) EvntExpose (wind, nEvn, exps);
	}
	return (nEvn != 0);
}

//==============================================================================
void
WindDrawSection (WINDOW * wind, const GRECT * clip)
{
	BOOL    enter = xTrue;
	int     level = 0;
	PXY     base;
	PRECT * area;  // overall rectangle containing all sections
	PRECT * sect;  // list of sections to be drawn
	short   nClp;  // list length
	
	vswr_mode (GRPH_Vdi, MD_REPLACE);
	
	if (wind->GwmDecor) {
		nClp = WindClipLock (wind, WMGR_Decor,
		                     clip, -1, &base, &sect, IncludeInferiors);
		if (nClp) {
			PRECT work = { base, {
			               base.p_x + wind->Rect.g_w -1, base.p_y + wind->Rect.g_h -1 } };
			area = (sect + nClp);
			WmgrDrawDeco (wind, &work, area, sect, nClp);
			if (draw_wind (wind, &work, area, sect, nClp)
			     && (wind = wind->StackBot)) {
				*area = work;
				level = 1;
			} else {
				WindClipOff();
				nClp = 0;
			}
		}
	} else {
		nClp  = WindClipLock (wind, wind->BorderWidth,
		                      clip, -1, &base, &sect, IncludeInferiors);
		level = 0;
		area  = (sect + nClp);
		base.p_x -= wind->Rect.g_x;
		base.p_y -= wind->Rect.g_y;
	}
	if (!nClp) return;
	
	do {
		if (enter && wind->isMapped) {
			PXY     orig = { base.p_x + wind->Rect.g_x, base.p_y + wind->Rect.g_y };
			PRECT * work = area +1;
			work->lu   = orig;
			work->rd.p_x = orig.p_x + wind->Rect.g_w -1;
			work->rd.p_y = orig.p_y + wind->Rect.g_h -1;
			if (wind->ClassInOut) {
				if (wind->hasBorder && wind->BorderWidth) {
					draw_brdr (wind, work, area, sect, nClp);
				}
				if (draw_wind (wind, work, area, sect, nClp) && wind->StackBot) {
					level++;
					area = work;
					base = orig;
					wind = wind->StackBot;
					continue;
				}
			}
		}
		if (level) {
			if (wind->NextSibl) {
				enter = xTrue;
				wind  = wind->NextSibl;
			} else if (--level) {
				area--;
				enter  =  xFalse;
				wind   =  wind->Parent;
				base.p_x -= wind->Rect.g_x;
				base.p_y -= wind->Rect.g_y;
			}
		}
	} while (level);
	
	WindClipOff();
}


//==============================================================================
void
WindPutMono (p_WINDOW wind, p_GC gc, GRECT * rct, MFDB *src)
{
	PRECT * sect;
	PXY     orig;
	GRECT * clip;
	CARD16  nClp;
	
	if (gc->ClipNum > 0) {
		clip = alloca (sizeof(GRECT) * gc->ClipNum);
		nClp = GrphInterList (clip, (rct +1),1, gc->ClipRect,gc->ClipNum);
		if (!nClp) return;
	} else {
		clip = (rct +1);
		nClp = 1;
	}
	nClp = WindClipLock (wind, 0, clip, nClp, &orig, &sect, gc->SubwindMode);
	if (nClp) {
		MFDB  dst    = { NULL };
		short col[2] = { gc->Foreground, gc->Background };
		short mode   = (gc->Function == GXor ? MD_TRANS :
		                gc->Function == GXor ? MD_XOR   : MD_REPLACE);
		PRECT pxy[2];
		
		orig.p_x -= rct[0].g_x - rct[1].g_x;
		orig.p_y -= rct[0].g_y - rct[1].g_y;
		v_hide_c (GRPH_Vdi);
		do {
			pxy[1]      = *(sect++);
			pxy[0].lu.p_x = pxy[1].lu.p_x - orig.p_x;
			pxy[0].lu.p_y = pxy[1].lu.p_y - orig.p_y;
			pxy[0].rd.p_x = pxy[1].rd.p_x - orig.p_x;
			pxy[0].rd.p_y = pxy[1].rd.p_y - orig.p_y;
			vrt_cpyfm (GRPH_Vdi, mode, (short*)pxy, src, &dst, col);
		} while (--nClp);
		
		v_show_c (GRPH_Vdi, 1);
		WindClipOff();
	}
}

//==============================================================================
void
WindPutColor (p_WINDOW wind, p_GC gc, GRECT * rct, MFDB *src)
{
	PRECT * sect;
	PXY     orig;
	GRECT * clip;
	CARD16  nClp;
	
	if (gc->ClipNum > 0) {
		clip = alloca (sizeof(GRECT) * gc->ClipNum);
		nClp = GrphInterList (clip, (rct +1),1, gc->ClipRect,gc->ClipNum);
		if (!nClp) return;
	} else {
		clip = (rct +1);
		nClp = 1;
	}
	nClp = WindClipLock (wind, 0, clip, nClp, &orig, &sect, gc->SubwindMode);
	if (nClp) {
		MFDB  dst = { NULL };
		PRECT pxy[2];
		
		orig.p_x -= rct[0].g_x - rct[1].g_x;
		orig.p_y -= rct[0].g_y - rct[1].g_y;
		v_hide_c (GRPH_Vdi);
		do {
			pxy[1]      = *(sect++);
			pxy[0].lu.p_x = pxy[1].lu.p_x - orig.p_x;
			pxy[0].lu.p_y = pxy[1].lu.p_y - orig.p_y;
			pxy[0].rd.p_x = pxy[1].rd.p_x - orig.p_x;
			pxy[0].rd.p_y = pxy[1].rd.p_y - orig.p_y;
			vro_cpyfm (GRPH_Vdi, gc->Function, (short*)pxy, src, &dst);
		} while (--nClp);
		
		v_show_c (GRPH_Vdi, 1);
		WindClipOff();
	}
}


//------------------------------------------------------------------------------
/*
static void
_put_mono (p_WINDOW wind, p_GC gc, PXY offs, MFDB *src,
           PRECT * sect, CARD16 nSct, PRECT * clip, CARD16 nClp)
{
	short mode      = (gc->Function == GXor ? MD_TRANS : MD_REPLACE);
	short colors[2] = { gc->Foreground, gc->Background };
	MFDB  dst       = { NULL };
	PRECT pxy[2];
	
	while (nClp--) {
		PRECT * s = sect;
		CARD16  n = nSct;
		do {
			pxy[1] = *clip;
			if (GrphIntersectP (&pxy[1], s++)) {
				pxy[0].lu.x = offs.x + pxy[1].lu.x;
				pxy[0].rd.x = offs.x + pxy[1].rd.x;
				pxy[0].lu.y = offs.y + pxy[1].lu.y;
				pxy[0].rd.y = offs.y + pxy[1].rd.y;
				vrt_cpyfm (GRPH_Vdi, mode, (short*)pxy, src, &dst, colors);
			}
		} while (--n);
		clip++;
	}
}
*/

//------------------------------------------------------------------------------
static void
_put_color (p_WINDOW wind, p_GC gc, PXY offs, MFDB *src,
               PRECT * sect, CARD16 nSct, PRECT * clip, CARD16 nClp)
{
	short mode = gc->Function;
	MFDB  dst  = { NULL };
	PRECT pxy[2];
	
	while (nClp--) {
		PRECT * s = sect;
		CARD16  n = nSct;
		do {
			pxy[1] = *clip;
			if (GrphIntersectP (&pxy[1], s++)) {
				pxy[0].lu.p_x = offs.p_x + pxy[1].lu.p_x;
				pxy[0].rd.p_x = offs.p_x + pxy[1].rd.p_x;
				pxy[0].lu.p_y = offs.p_y + pxy[1].lu.p_y;
				pxy[0].rd.p_y = offs.p_y + pxy[1].rd.p_y;
				vro_cpyfm (GRPH_Vdi, mode, (short*)pxy, src, &dst);
			}
		} while (--n);
		clip++;
	}
}

//==============================================================================
void
WindPutImg (p_WINDOW wind, p_GC gc, GRECT *rct, MFDB *src,
            PXY orig, PRECT * sect, CARD16 nSct)
{
	PRECT * clip;
	CARD16  nClp;
	PXY     offs = { rct[0].g_x - rct[1].g_x - orig.p_x, rct[0].g_y - rct[1].g_y - orig.p_y };
	
	if (gc->ClipNum > 0) {
		CARD16  n = gc->ClipNum;
		GRECT * c = gc->ClipRect;
		GRECT * r = (GRECT*)(clip = alloca (sizeof(PRECT) * gc->ClipNum));
		nClp      = 0;
		do {
			*r = *(c++);
			if (GrphIntersect (r, &rct[1])) {
				r->g_w += (r->g_x += orig.p_x) -1;
				r->g_h += (r->g_y += orig.p_y) -1;
				r++;
				nClp++;
			}
		} while (--n);
	} else {
		clip = alloca (sizeof(PRECT));
		nClp = 1;
		clip->rd.p_x = (clip->lu.p_x = rct[1].g_x + orig.p_x) + rct[1].g_w -1;
		clip->rd.p_y = (clip->lu.p_y = rct[1].g_y + orig.p_y) + rct[1].g_h -1;
	}
	v_hide_c (GRPH_Vdi);
	//
	// src and dst must always have the same color depth !!!
	//
	_put_color (wind, gc, offs, src, sect,nSct, clip,nClp);
	v_show_c (GRPH_Vdi, 1);
}


//==============================================================================
CARD16
WindScroll (p_WINDOW wind, p_GC gc, GRECT * rect,
            PXY diff, PXY orig, PRECT * sect, CARD16 nSct, GRECT * exps)
{
	// rect[0]: combined source and destination area (relative position)
	//     [1]: destination only area (relative position)
	// diff:    direction to scroll, < 0 = left/up, > 0 = right/down
	// exps:    buffer for exposure rectangles storing or NULL
	//...........................................................................
	
	MFDB   s_mfdb  = { NULL, }, d_mfdb = { NULL, };
	BOOL   do_tile = (exps != NULL || wind->hasBackGnd);
	short  d_x     = (diff.p_x < 0 ? -diff.p_x : diff.p_x);
	short  d_y     = (diff.p_y < 0 ? -diff.p_y : diff.p_y);
	CARD16 nExp = 0;
	PRECT  area;     // to be intersected with sect[]
	PRECT * cLst;
	CARD16  nLst;
	PRECT  pxy[2];
	
	pxy->rd.p_x = (pxy->lu.p_x = rect[1].g_x + orig.p_x) + rect[1].g_w -1;
	pxy->rd.p_y = (pxy->lu.p_y = rect[1].g_y + orig.p_y) + rect[1].g_h -1;
	if (gc->ClipNum > 0) {
		PRECT * c = cLst = alloca (sizeof(PRECT) * gc->ClipNum);
		GRECT * r = gc->ClipRect;
		CARD16  n = gc->ClipNum;
		nLst      = 0;
		while (n--) {
			c->rd.p_x = (c->lu.p_x = r->g_x + orig.p_x) + r->g_w -1;
			c->rd.p_y = (c->lu.p_y = r->g_y + orig.p_y) + r->g_h -1;
			if (GrphIntersectP (c, pxy)) {
				c++;
				nLst++;
			}
		}
		if (!nLst) return 0;
		
	} else {
		cLst  = alloca (sizeof(PRECT));
		*cLst = *pxy;
		nLst  = 1;
	}
	area.rd.p_x = (area.lu.p_x = rect[0].g_x + orig.p_x) + rect[0].g_w -1;
	area.rd.p_y = (area.lu.p_y = rect[0].g_y + orig.p_y) + rect[0].g_h -1;
	
	if (!wind->hasBackPix) {
		vswr_mode (GRPH_Vdi, MD_REPLACE);
		vsf_color (GRPH_Vdi, wind->Back.Pixel);
	}
	while (nSct--) {
		PRECT * dest;
		CARD16  nDst;
		PRECT   clip = area;
		if (!GrphIntersectP (&clip, sect++)) continue;
		
		dest = cLst;
		nDst = nLst;
		while (nDst--) {
			PRECT  tile[3]; // above,left/right,below tile areas
			CARD16 nTil;    // number of tiles
			short  offs;
			pxy[1] = *(dest++);
			if (!GrphIntersectP (&pxy[1], &clip)) continue;
			
			pxy[0] = pxy[1];
			nTil   = 0;
			offs   = 0;
			if (diff.p_y < 0) {   //................scroll.up......
				offs++; // start tileing at left/right area
				pxy[0].lu.p_y += d_y;
				pxy[0].rd.p_y += d_y;
				if (pxy[0].rd.p_y > clip.rd.p_y) {
					pxy[1].rd.p_y -= pxy[0].rd.p_y - clip.rd.p_y;
					pxy[0].rd.p_y =  clip.rd.p_y;
					if (do_tile) {               // tile below
						tile[2].lu.p_x = pxy[0].lu.p_x;
						tile[2].lu.p_y = pxy[1].rd.p_y +1;
						tile[2].rd   = pxy[0].rd;
						nTil++;
					}
				}
			} else if (diff.p_y > 0) {   //.........scroll.down....
				pxy[0].lu.p_y -= d_y;
				pxy[0].rd.p_y -= d_y;
				if (pxy[0].lu.p_y < clip.lu.p_y) {
					pxy[1].lu.p_y -= pxy[0].lu.p_y - clip.lu.p_y;
					pxy[0].lu.p_y =  clip.lu.p_y;
					if (do_tile) {               // tile above
						tile[0].lu   = pxy[0].lu;
						tile[0].rd.p_x = pxy[1].rd.p_x;
						tile[0].rd.p_y = pxy[1].lu.p_y -1;
						nTil++;
					}
				}
			}
			if (diff.p_x < 0) {   //................scroll.left....
				pxy[0].lu.p_x += d_x;
				pxy[0].rd.p_x += d_x;
				if (pxy[0].rd.p_x > clip.rd.p_x) {
					pxy[1].rd.p_x -= pxy[0].rd.p_x - clip.rd.p_x;
					pxy[0].rd.p_x =  clip.rd.p_x;
					if (do_tile) {               // tile right
						tile[1].lu.p_x = pxy[1].rd.p_x +1;
						tile[1].lu.p_y = pxy[1].lu.p_y;
						tile[1].rd.p_x = pxy[0].rd.p_x;
						tile[1].rd.p_y = pxy[1].rd.p_y;
						nTil++;
					}
				}
			} else if (diff.p_x > 0) {   //.........scroll.right...
				pxy[0].lu.p_x -= d_x;
				pxy[0].rd.p_x -= d_x;
				if (pxy[0].lu.p_x < clip.lu.p_x) {
					pxy[1].lu.p_x -= pxy[0].lu.p_x - clip.lu.p_x;
					pxy[0].lu.p_x =  clip.lu.p_x;
					if (do_tile) {               // tile left
						tile[1].lu.p_x = pxy[0].lu.p_x;
						tile[1].lu.p_y = pxy[1].lu.p_y;
						tile[1].rd.p_x = pxy[1].lu.p_x -1;
						tile[1].rd.p_y = pxy[1].rd.p_y;
						nTil++;
					}
				}
			} else if (offs) {
				offs++; // start tileing at below area
			}
			if (pxy[0].lu.p_x <= pxy[0].rd.p_x  &&  pxy[0].lu.p_y <= pxy[0].rd.p_y) {
				v_hide_c  (GRPH_Vdi);
				vro_cpyfm (GRPH_Vdi, S_ONLY, (short*)pxy, &s_mfdb, &d_mfdb);
				v_show_c  (GRPH_Vdi, 1);
			} else if (do_tile) {
				offs    = 0;
				tile[0] = clip;
				nTil    = 1;
			}
			if (nTil) {
				if (wind->hasBackGnd) {
					int n = WindDrawBgnd (wind, orig, &clip, tile + offs, nTil, exps);
					if (exps) {
						exps += n;
						nExp += n;
					}
				} else if (exps) do {
					PRECT * c = (tile + offs++);
					exps->g_w = c->rd.p_x - c->lu.p_x +1;
					exps->g_h = c->rd.p_y - c->lu.p_y +1;
					exps->g_x = c->lu.p_x - orig.p_x;
					exps->g_y = c->lu.p_y - orig.p_y;
					exps++;
					nExp++;
				} while (nTil--);
			}
		}
	}
	return nExp;
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_ClearArea (CLIENT * clnt, xClearAreaReq * q)
{
	// Fill the window's area with it's background.
	//
	// BOOL   exposures:
	// Window window:
	// INT16  x, y:
	// CARD16 width, height:
	//...........................................................................
	
	WINDOW * wind = WindFind (q->window);
	BOOL     evn;
	
	if (!wind) {
		Bad(Window, q->window, ClearArea,);
	
	} else if (!wind->ClassInOut) {
		Bad(Match,, ClearArea,"(W:%X) class InputOnly", wind->Id);
	
	} else if ((evn = (q->exposures && (wind->u.List.AllMasks & ExposureMask)))
	           || wind->hasBackGnd) {
		PRECT * sect;
		PXY     orig;
		CARD16  nClp;
		GRECT * exps = NULL;
		CARD16  nEvn = 0;
		
		DEBUG (ClearArea," W:%X [%i,%i/%i,%i]",
		       wind->Id, q->x, q->y, q->width, q->height);
		
		if ((short)q->width <= 0 || q->x + q->width > wind->Rect.g_w) {
			q->width = wind->Rect.g_w - q->x;
		}
		if((short)q->height <= 0 || q->y + q->height > wind->Rect.g_h) {
			q->height = wind->Rect.g_h - q->y;
		}
		if (q->width > 0  &&  q->height > 0
		    && (nClp = WindClipLock (wind, 0, (GRECT*)&q->x, 1, &orig, &sect,
		                             IncludeInferiors))) {
			PRECT clip = sect[nClp];
			short num;
			
			if (wind->Id == ROOT_WINDOW) {
				if (evn) {
					exps = (GRECT*)sect;
					nEvn = nClp;
				}
				if (wind->hasBackGnd) {
					extern OBJECT WMGR_Desktop;
					do {
						sect->rd.p_x -= sect->lu.p_x -1;
						sect->rd.p_y -= sect->lu.p_y -1;
						objc_draw (&WMGR_Desktop, 0, 1,
						           sect->lu.p_x, sect->lu.p_y, sect->rd.p_x, sect->rd.p_y);
						sect->lu.p_x -= orig.p_x;
						sect->lu.p_y -= orig.p_y;
						sect++;
					} while (--nClp);
				}
				WindClipOff();
			
			} else if ((num = clip_children (wind, orig, sect + nClp, &clip))) {
				PRECT * area = sect + nClp;
				
				if (evn) {
					exps = (GRECT*)(area + num);
				}
				if (wind->hasBackGnd) {
					if (!wind->hasBackPix) {
						vswr_mode (GRPH_Vdi, MD_REPLACE);
						vsf_color (GRPH_Vdi, wind->Back.Pixel);
					}
					do {
						CARD16 n = WindDrawBgnd (wind, orig, sect++, area, num, exps);
						if (evn) {
							nEvn += n;
							exps += n;
						}
					} while (--nClp);
				
				} else if (evn) do {
					int i;
					for (i = 0; i < num; i++) {
						*exps = *(GRECT*)sect;
						if (GrphIntersectP ((PRECT*)exps, area + i)) {
							exps->g_w -= exps->g_x -1;
							exps->g_h -= exps->g_y -1;
							exps->g_x -= orig.p_x;
							exps->g_y -= orig.p_y;
							exps++;
							nEvn++;
						}
					}
					sect++;
				} while (--nClp);
				
				if (evn) {
					exps -= nEvn;
				}
			}
			
			if (nEvn) {
				EvntExpose (wind, nEvn, exps);
			}
			WindClipOff();
		}
	}
}
