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
#include "window_P.h"
#include "event.h"
#include "gcontext.h"
#include "grph.h"
#include "pixmap.h"
#include "wmgr.h"
#include "x_gem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h> // printf

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
			dst->rd.x = (dst->lu.x = orig.x + wind->Rect.x) + wind->Rect.w -1;
			dst->rd.y = (dst->lu.y = orig.y + wind->Rect.y) + wind->Rect.h -1;
			if (b) {
				dst->lu.x -= b;
				dst->lu.y -= b;
				dst->rd.x += b;
				dst->rd.y += b;
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
			short x = dst->lu.x;
			short y = dst->lu.y;
			int   j = i -1;
			while (j >= 0  &&  list[j]->lu.y > y) {
				list[j+1] = list[j];
				j--;
			}
			while (j >= 0  &&  list[j]->lu.y == y  &&  list[j]->lu.x > x) {
				list[j+1] = list[j];
				j--;
			}
			list[j+1] = dst++;
		}
		dst -= num;
		
		// find free rectangles between child geometries
					
		beg = 0;
		bot = clip->lu.y;
		do {
			short x = clip->lu.x;
			if (bot < list[beg]->lu.y) { // found area above first child geometry
				top = list[beg]->lu.y;
				area->lu.x = x;
				area->lu.y = bot;
				area->rd.x = clip->rd.x;
				area->rd.y = top -1;
				area++;
				cnt++;
			} else {
				top = bot;
			}
			bot = list[beg]->rd.y;
			
			for (end = beg +1; end < num; ++end) {
				if (list[end]->lu.y > top) {
					if (list[end]->lu.y <= bot) bot = list[end]->lu.y -1;
					break;
				} else if (list[end]->rd.y < bot) {
					bot = list[end]->rd.y;
				}
			}
			i = beg;
			while (i < end) {
				if (x < list[i]->lu.x) { // free area on left side of child
					area->lu.x = x;
					area->lu.y = top;
					area->rd.x = list[i]->lu.x -1;
					area->rd.y = bot;
					area++;
					cnt++;
				}
				if (x <= list[i]->rd.x) {
					x = list[i]->rd.x +1;
				}
				if      (list[i]->rd.y > bot) i++;
				else if (i == beg)            beg = ++i;
				else {
					short j = i;
					while (++j < num) list[j-1] = list[j];
					if (--num < end)  end = num;
				}
			}
			if (x <= clip->rd.x) { // free area on right side of last child
				area->lu.x = x;
				area->lu.y = top;
				area->rd.x = clip->rd.x;
				area->rd.y = bot;
				area++;
				cnt++;
			}
			
			if (i > beg) {
				while (i < num  &&  list[i]->lu.y == bot
				       &&  list[i]->lu.x < list[i-1]->lu.x) {
					short   j    = i;
					PRECT * save = list[i];
					do {
						list[j] = list[j-1];
					} while (--j > beg  &&  save->lu.x < list[j]->lu.x);
					list[j] = save;
				}
				i++;
			}
			bot++; // to be the top for the next loop
		
		} while (beg < num);
		
		if (bot <= clip->rd.y) { // free area at the bottom
			area->lu.x = clip->lu.x;
			area->lu.y = bot;
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
		work.x -= border;
		work.y -= border;
		border *= 2;
		work.w += border;
		work.h += border;
	}
	if (work.x < 0) { work.w += work.x; work.x = 0; }
	if (work.y < 0) { work.h += work.y; work.y = 0; }
	
	*orig = *(PXY*)&wind->Rect;
	
	while ((pwnd = twnd->Parent)) {
		if (work.x + work.w > pwnd->Rect.w) work.w = pwnd->Rect.w - work.x;
		if (work.y + work.h > pwnd->Rect.h) work.h = pwnd->Rect.h - work.y;
		if ((work.x += pwnd->Rect.x) < 0) { work.w += work.x; work.x = 0; }
		if ((work.y += pwnd->Rect.y) < 0) { work.h += work.y; work.y = 0; }
		orig->x += pwnd->Rect.x;
		orig->y += pwnd->Rect.y;
		if (pwnd == &WIND_Root  || !(visb &= pwnd->isMapped)) break;
		twnd = pwnd;
	}
	if (!visb ||  work.w <= 0  ||  work.h <= 0) return 0;
	
	work.w += work.x -1;
	work.h += work.y -1;
	
	if (clip  &&  n_clip > 0) {
		PRECT * c = p_clip = alloca (sizeof(PRECT) * n_clip);
		n         = 0;
		while (n_clip--) {
			c->rd.x = (c->lu.x = clip->x + orig->x) + clip->w -1;
			c->rd.y = (c->lu.y = clip->y + orig->y) + clip->h -1;
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
			c.rd.x = (c.lu.x = clip->x) + clip->w -1;
			c.rd.y = (c.lu.y = clip->y) + clip->h -1;
			if (!GrphIntersectP (p_clip, &c)) return 0;
		}
		n_clip = 1;
	}
	
	WindUpdate (xTrue);
	wind_get (0, WF_SCREEN, &a, &b, &n,&n);
	*pBuf = sect = (PRECT*)(((long)a << 16) | (b & 0xFFFF));
	
	wind_get_first (twnd->Handle, &rect);
	while (rect.w > 0  &&  rect.h > 0) {
		PRECT * c = p_clip;
		n         = n_clip;
		rect.w   += rect.x -1;
		rect.h   += rect.y -1;
		do {
			*sect = *(PRECT*)&rect;
			if (GrphIntersectP (sect, c++)) {
				if (l > sect->lu.x) l = sect->lu.x;
				if (u > sect->lu.y) u = sect->lu.y;
				if (r < sect->rd.x) r = sect->rd.x;
				if (d < sect->rd.y) d = sect->rd.y;
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
		p_clip->lu.x = l;
		p_clip->lu.y = u;
		p_clip->rd.x = r;
		p_clip->rd.y = d;
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
		sect->lu.x = l;
		sect->lu.y = u;
		sect->rd.x = r;
		sect->rd.y = d;
	}
	return nClp;
}


//==============================================================================
void
WindDrawPmap (PIXMAP * pmap, PXY orig, p_PRECT sect)
{
	MFDB  scrn = { NULL, };
	PXY   offs = { sect->lu.x - orig.x, sect->lu.y - orig.y };
	short rd_x = pmap->W -1, rd_y = pmap->H -1;
	short color[2] = { G_BLACK, G_WHITE };
	PXY   pxy[4];
	#define s_lu pxy[0]
	#define s_rd pxy[1]
	#define d_lu pxy[2]
	#define d_rd pxy[3]
	int  d;
	
	if (offs.x >= pmap->W) offs.x %= pmap->W;
	if (offs.y >= pmap->H) offs.y %= pmap->H;
	
	s_lu.y = offs.y;
	s_rd.y = rd_y;
	d_lu.y = sect->lu.y;
	d_rd   = sect->rd;
	while ((d = d_rd.y - d_lu.y) >= 0) {
		if (d < rd_y - s_lu.y) {
			s_rd.y = s_lu.y + d;
		}
		s_lu.x = offs.x;
		s_rd.x = rd_x;
		d_lu.x = sect->lu.x;
		while ((d = d_rd.x - d_lu.x) >= 0) {
			if (d < rd_x - s_lu.x) {
				s_rd.x = s_lu.x + d;
			}
			if (pmap->Depth == 1) {
				vrt_cpyfm (GRPH_Vdi, MD_REPLACE,
				           (short*)pxy, PmapMFDB(pmap), &scrn, color);
			} else {
				vro_cpyfm (GRPH_Vdi, S_ONLY, (short*)pxy, PmapMFDB(pmap), &scrn);
			}
			d_lu.x += pmap->W - s_lu.x;
			s_lu.x =  0;
		}
		d_lu.y += pmap->H - s_lu.y;
		s_lu.y =  0;
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
					exps->w = rect.rd.x - rect.lu.x +1;
					exps->h = rect.rd.y - rect.lu.y +1;
					exps->x = rect.lu.x - orig.x;
					exps->y = rect.lu.y - orig.y;
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
					exps->w = rect.rd.x - rect.lu.x +1;
					exps->h = rect.rd.y - rect.lu.y +1;
					exps->x = rect.lu.x - orig.x;
					exps->y = rect.lu.y - orig.y;
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
	short l = work->lu.x - i;
	short r = work->rd.x + i;
	int   n = 0;
	PRECT brdr[4];
	
	brdr[0].lu.x = l;
	brdr[0].lu.y = work->lu.y - i;
	brdr[0].rd.x = r;
	brdr[0].rd.y = work->lu.y - 1;
	if (GrphIntersectP (brdr, area)) n++;
	brdr[n].lu.x = l;
	brdr[n].lu.y = work->lu.y;
	brdr[n].rd.x = work->lu.x - 1;
	brdr[n].rd.y = work->rd.y;
	if (GrphIntersectP (brdr + n, area)) n++;
	brdr[n].lu.x = work->rd.x + 1;
	brdr[n].lu.y = work->lu.y;
	brdr[n].rd.x = work->rd.x + i;
	brdr[n].rd.y = work->rd.y;
	if (GrphIntersectP (brdr + n, area)) n++;
	brdr[n].lu.x = l;
	brdr[n].lu.y = work->rd.y + 1;
	brdr[n].rd.x = r;
	brdr[n].rd.y = work->rd.y + i;
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
					exps->w -= exps->x -1;
					exps->h -= exps->y -1;
					exps->x -= orig.x;
					exps->y -= orig.y;
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
			               base.x + wind->Rect.w -1, base.y + wind->Rect.h -1 } };
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
		base.x -= wind->Rect.x;
		base.y -= wind->Rect.y;
	}
	if (!nClp) return;
	
	do {
		if (enter && wind->isMapped) {
			PXY     orig = { base.x + wind->Rect.x, base.y + wind->Rect.y };
			PRECT * work = area +1;
			work->lu   = orig;
			work->rd.x = orig.x + wind->Rect.w -1;
			work->rd.y = orig.y + wind->Rect.h -1;
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
				base.x -= wind->Rect.x;
				base.y -= wind->Rect.y;
			}
		}
	} while (level);
	
	WindClipOff();
}


//==============================================================================
void
WindPutMono (p_WINDOW wind, p_GC gc, p_GRECT rct, p_MFDB src)
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
		
		orig.x -= rct[0].x - rct[1].x;
		orig.y -= rct[0].y - rct[1].y;
		v_hide_c (GRPH_Vdi);
		do {
			pxy[1]      = *(sect++);
			pxy[0].lu.x = pxy[1].lu.x - orig.x;
			pxy[0].lu.y = pxy[1].lu.y - orig.y;
			pxy[0].rd.x = pxy[1].rd.x - orig.x;
			pxy[0].rd.y = pxy[1].rd.y - orig.y;
			vrt_cpyfm (GRPH_Vdi, mode, (short*)pxy, src, &dst, col);
		} while (--nClp);
		
		v_show_c (GRPH_Vdi, 1);
		WindClipOff();
	}
}

//==============================================================================
void
WindPutColor (p_WINDOW wind, p_GC gc, p_GRECT rct, p_MFDB src)
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
		
		orig.x -= rct[0].x - rct[1].x;
		orig.y -= rct[0].y - rct[1].y;
		v_hide_c (GRPH_Vdi);
		do {
			pxy[1]      = *(sect++);
			pxy[0].lu.x = pxy[1].lu.x - orig.x;
			pxy[0].lu.y = pxy[1].lu.y - orig.y;
			pxy[0].rd.x = pxy[1].rd.x - orig.x;
			pxy[0].rd.y = pxy[1].rd.y - orig.y;
			vro_cpyfm (GRPH_Vdi, gc->Function, (short*)pxy, src, &dst);
		} while (--nClp);
		
		v_show_c (GRPH_Vdi, 1);
		WindClipOff();
	}
}


//------------------------------------------------------------------------------
/*
static void
_put_mono (p_WINDOW wind, p_GC gc, PXY offs, p_MFDB src,
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
_put_color (p_WINDOW wind, p_GC gc, PXY offs, p_MFDB src,
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
				pxy[0].lu.x = offs.x + pxy[1].lu.x;
				pxy[0].rd.x = offs.x + pxy[1].rd.x;
				pxy[0].lu.y = offs.y + pxy[1].lu.y;
				pxy[0].rd.y = offs.y + pxy[1].rd.y;
				vro_cpyfm (GRPH_Vdi, mode, (short*)pxy, src, &dst);
			}
		} while (--n);
		clip++;
	}
}

//==============================================================================
void
WindPutImg (p_WINDOW wind, p_GC gc, p_GRECT rct, p_MFDB src,
            PXY orig, PRECT * sect, CARD16 nSct)
{
	PRECT * clip;
	CARD16  nClp;
	PXY     offs = { rct[0].x - rct[1].x - orig.x, rct[0].y - rct[1].y - orig.y };
	
	if (gc->ClipNum > 0) {
		CARD16  n = gc->ClipNum;
		GRECT * c = gc->ClipRect;
		GRECT * r = (GRECT*)(clip = alloca (sizeof(PRECT) * gc->ClipNum));
		nClp      = 0;
		do {
			*r = *(c++);
			if (GrphIntersect (r, &rct[1])) {
				r->w += (r->x += orig.x) -1;
				r->h += (r->y += orig.y) -1;
				r++;
				nClp++;
			}
		} while (--n);
	} else {
		clip = alloca (sizeof(PRECT));
		nClp = 1;
		clip->rd.x = (clip->lu.x = rct[1].x + orig.x) + rct[1].w -1;
		clip->rd.y = (clip->lu.y = rct[1].y + orig.y) + rct[1].h -1;
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
	short  d_x     = (diff.x < 0 ? -diff.x : diff.x);
	short  d_y     = (diff.y < 0 ? -diff.y : diff.y);
	CARD16 nExp = 0;
	PRECT  area;     // to be intersected with sect[]
	PRECT * cLst;
	CARD16  nLst;
	PRECT  pxy[2];
	
	pxy->rd.x = (pxy->lu.x = rect[1].x + orig.x) + rect[1].w -1;
	pxy->rd.y = (pxy->lu.y = rect[1].y + orig.y) + rect[1].h -1;
	if (gc->ClipNum > 0) {
		PRECT * c = cLst = alloca (sizeof(PRECT) * gc->ClipNum);
		GRECT * r = gc->ClipRect;
		CARD16  n = gc->ClipNum;
		nLst      = 0;
		while (n--) {
			c->rd.x = (c->lu.x = r->x + orig.x) + r->w -1;
			c->rd.y = (c->lu.y = r->y + orig.y) + r->h -1;
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
	area.rd.x = (area.lu.x = rect[0].x + orig.x) + rect[0].w -1;
	area.rd.y = (area.lu.y = rect[0].y + orig.y) + rect[0].h -1;
	
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
			if (diff.y < 0) {   //................scroll.up......
				offs++; // start tileing at left/right area
				pxy[0].lu.y += d_y;
				pxy[0].rd.y += d_y;
				if (pxy[0].rd.y > clip.rd.y) {
					pxy[1].rd.y -= pxy[0].rd.y - clip.rd.y;
					pxy[0].rd.y =  clip.rd.y;
					if (do_tile) {               // tile below
						tile[2].lu.x = pxy[0].lu.x;
						tile[2].lu.y = pxy[1].rd.y +1;
						tile[2].rd   = pxy[0].rd;
						nTil++;
					}
				}
			} else if (diff.y > 0) {   //.........scroll.down....
				pxy[0].lu.y -= d_y;
				pxy[0].rd.y -= d_y;
				if (pxy[0].lu.y < clip.lu.y) {
					pxy[1].lu.y -= pxy[0].lu.y - clip.lu.y;
					pxy[0].lu.y =  clip.lu.y;
					if (do_tile) {               // tile above
						tile[0].lu   = pxy[0].lu;
						tile[0].rd.x = pxy[1].rd.x;
						tile[0].rd.y = pxy[1].lu.y -1;
						nTil++;
					}
				}
			}
			if (diff.x < 0) {   //................scroll.left....
				pxy[0].lu.x += d_x;
				pxy[0].rd.x += d_x;
				if (pxy[0].rd.x > clip.rd.x) {
					pxy[1].rd.x -= pxy[0].rd.x - clip.rd.x;
					pxy[0].rd.x =  clip.rd.x;
					if (do_tile) {               // tile right
						tile[1].lu.x = pxy[1].rd.x +1;
						tile[1].lu.y = pxy[1].lu.y;
						tile[1].rd.x = pxy[0].rd.x;
						tile[1].rd.y = pxy[1].rd.y;
						nTil++;
					}
				}
			} else if (diff.x > 0) {   //.........scroll.right...
				pxy[0].lu.x -= d_x;
				pxy[0].rd.x -= d_x;
				if (pxy[0].lu.x < clip.lu.x) {
					pxy[1].lu.x -= pxy[0].lu.x - clip.lu.x;
					pxy[0].lu.x =  clip.lu.x;
					if (do_tile) {               // tile left
						tile[1].lu.x = pxy[0].lu.x;
						tile[1].lu.y = pxy[1].lu.y;
						tile[1].rd.x = pxy[1].lu.x -1;
						tile[1].rd.y = pxy[1].rd.y;
						nTil++;
					}
				}
			} else if (offs) {
				offs++; // start tileing at below area
			}
			if (pxy[0].lu.x <= pxy[0].rd.x  &&  pxy[0].lu.y <= pxy[0].rd.y) {
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
					exps->w = c->rd.x - c->lu.x +1;
					exps->h = c->rd.y - c->lu.y +1;
					exps->x = c->lu.x - orig.x;
					exps->y = c->lu.y - orig.y;
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
		
		if ((short)q->width <= 0 || q->x + q->width > wind->Rect.w) {
			q->width = wind->Rect.w - q->x;
		}
		if((short)q->height <= 0 || q->y + q->height > wind->Rect.h) {
			q->height = wind->Rect.h - q->y;
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
						sect->rd.x -= sect->lu.x -1;
						sect->rd.y -= sect->lu.y -1;
						objc_draw (&WMGR_Desktop, 0, 1,
						           sect->lu.x, sect->lu.y, sect->rd.x, sect->rd.y);
						sect->lu.x -= orig.x;
						sect->lu.y -= orig.y;
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
							exps->w -= exps->x -1;
							exps->h -= exps->y -1;
							exps->x -= orig.x;
							exps->y -= orig.y;
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
