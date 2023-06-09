/*
 *==============================================================================
 *
 * drawable.c
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-06-05 - Initial Version.
 *==============================================================================
 */
#include <stdio.h>
#include <alloca.h>

#include "main.h"
#include "tools.h"
#include "clnt.h"
#include "pixmap.h"
#include "window.h"
#include "gcontext.h"
#include "font.h"
#include "grph.h"
#include "x_gem.h"
#include "gemx.h"

#include <X11/X.h>


/* ============================================================================== */
void DrawDelete(p_DRAWABLE draw, p_CLIENT clnt)
{
	if (draw.p->isWind)
	{
		WINDOW *wind = draw.Window;

		while (RID_Match(clnt->Id, wind->Parent->Id))
		{
			wind = wind->Parent;
		}
		WindDelete(wind, clnt);

	} else
	{
		PmapFree(draw.Pixmap, clnt);
	}
}


/* ------------------------------------------------------------------------------ */
static inline CARD16 SizeToPXY(PRECT *dst, const CARD16 *src)
{
#if defined(__GNUC__) && 0
	__asm__ volatile (
		"\tmoveq.l	#0, %%d0\n"
		"\tmove.l	(%0), %%d1\n"
		"\tsub.l		#0x00010001, %%d1\n"
		"\tmovem.l	%%d0/%%d1, (%1)\n"
		:                   /* output */
		: "a"(src),"a"(dst) /* input */
		: "d0","d1"         /* clobbered */
	);
#else
	dst->lu.p_x = 0;
	dst->lu.p_y = 0;
	dst->rd.p_x = src[0] - 1;
	dst->rd.p_y = src[1] - 1;
#endif
	return 1;
}

/* ------------------------------------------------------------------------------ */
static inline CARD16 SizeToLst(PRECT *dst, GRECT *clip, CARD16 nClp, const CARD16 *src)
{
	CARD16 cnt = 0;
	PRECT size;

	SizeToPXY(&size, src);

	while (nClp--)
	{
		dst->rd.p_x = (dst->lu.p_x = clip->g_x) + clip->g_w - 1;
		dst->rd.p_y = (dst->lu.p_y = clip->g_y) + clip->g_h - 1;
		if (GrphIntersectP(dst, &size))
		{
			dst++;
			cnt++;
		}
		clip++;
	}
	return cnt;
}


/* ------------------------------------------------------------------------------ */
static inline BOOL gc_mode(CARD32 *color, BOOL *set, int hdl, const GC *gc)
{
	BOOL fg;
	short m;

	switch (gc->Function)
	{
	case GXequiv:						/* !src XOR  dst */
	case GXset:							/*  1 */
	case GXcopy:
		fg = xTrue;
		m = MD_REPLACE;
		break;							/*  src */

	case GXcopyInverted:				/* !src */
	case GXclear:
		fg = xFalse;
		m = MD_REPLACE;
		break;							/*  0 */

	case GXxor:
		fg = xTrue;
		m = MD_XOR;
		break;							/*  src XOR  dst */

	case GXnor:							/* !src AND !dst */
	case GXinvert:
		fg = xFalse;
		m = MD_XOR;
		break;							/*          !dst */

	case GXnand:						/* !src OR  !dst */
	case GXorReverse:					/*  src OR  !dst */
	case GXor:							/*  src OR   dst */
	case GXandReverse:
		fg = xTrue;
		m = MD_TRANS;
		break;							/*  src AND !dst */


	case GXorInverted:					/* !src OR   dst */
	case GXandInverted:					/* !src AND  dst */
	case GXand:							/*  src AND  dst */
	case GXnoop:						/*           dst */
	default:
		return xFalse;
	}
	if (fg)
	{
		*color = gc->Foreground;
	} else
	{
		*color = gc->Background;
		*set = xTrue;
	}
	vswr_mode(hdl, m);

	return xTrue;
}


/* ============================================================================== */
/* */
/* Callback Functions */

#include "Request.h"

/* ------------------------------------------------------------------------------ */
void RQ_GetGeometry(CLIENT *clnt, xGetGeometryReq *q)
{
	DRAWABLE *draw = NULL;

	if ((q->id & ~RID_MASK) && !(draw = DrawFind(q->id).p))
	{
		Bad(BadDrawable, q->id, X_GetGeometry, "_");
	} else
	{
		ClntReplyPtr(GetGeometry, r, 0);
		r->root = ROOT_WINDOW;

		if (!draw)
		{
			wind_get_work(q->id & 0x7FFF, (GRECT *) & r->x);
			r->depth = WIND_Root.Depth;
			r->x -= WIND_Root.Rect.g_x + 1;
			r->y -= WIND_Root.Rect.g_y + 1;
			r->borderWidth = 1;

		} else
		{
			r->depth = draw->Depth;
			if (draw->isWind)
			{
				WINDOW *wind = (WINDOW *) draw;

				*(GRECT *) & r->x = wind->Rect;
				r->x -= wind->BorderWidth;
				r->y -= wind->BorderWidth;
				r->borderWidth = wind->BorderWidth;
			} else
			{
				PIXMAP *pmap = (PIXMAP *) draw;

				r->x = 0;
				r->y = 0;
				r->width = pmap->W;
				r->height = pmap->H;
				r->borderWidth = 0;
			}

			DEBUG(X_GetGeometry, " 0x%lX", q->id);
		}

		ClntReply(GetGeometry, 0, "wR.");
	}
}


/* ------------------------------------------------------------------------------ */
void RQ_FillPoly(CLIENT *clnt, xFillPolyReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xFillPolyReq)) / sizeof(PXY);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_FillPoly, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_FillPoly, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		PXY *pxy = (PXY *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			PXY orig;

			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				clnt->Fnct->grph_shift_pnt(&orig, pxy, len, q->coordMode);
				v_hide_c(hdl);
			}
			DEBUG(X_FillPoly, " W:%lX G:%lX (%lu)", q->drawable, q->gc, len);

		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			clnt->Fnct->grph_shift_pnt(NULL, pxy, len, q->coordMode);
			set = (draw.Pixmap->Vdi > 0);
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			if (set)
			{
				vsf_color(hdl, color);
			}
			do
			{
				vs_clip_pxy(hdl, (PXY *) (sect++));
				v_fillarea(hdl, len, (short *) pxy);
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}

	} else
	{
		PRINT(X_FillPoly, " D:%lX G:%lX (0)", q->drawable, q->gc);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyArc(CLIENT *clnt, xPolyArcReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyArcReq)) / sizeof(xArc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyArc, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyArc, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		xArc *arc = (xArc *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		PXY orig;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				v_hide_c(hdl);
			}
			DEBUG(X_PolyArc, " P:%lX G:%lX (%lu)", q->drawable, q->gc, len);

		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			orig.p_x = 0;
			orig.p_y = 0;
			set = draw.Pixmap->Vdi > 0;
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			clnt->Fnct->grph_shift_arc(&orig, arc, len, ArcPieSlice);
			if (set)
			{
				vsl_width(hdl, gc->LineWidth);
				vsl_color(hdl, color);
			}
			do
			{
				int i;

				vs_clip_pxy(hdl, (PXY *) (sect++));
				for (i = 0; i < len; ++i)
				{
					v_ellarc(hdl, arc[i].x, arc[i].y, arc[i].width, arc[i].height, arc[i].angle1, arc[i].angle2);
				}
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyFillArc(CLIENT *clnt, xPolyFillArcReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyFillArcReq)) / sizeof(xArc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyFillArc, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyFillArc, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		xArc *arc = (xArc *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		PXY orig;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				v_hide_c(hdl);
			}
			DEBUG(X_PolyFillArc, " P:%lX G:%lX (%lu)", q->drawable, q->gc, len);

		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			orig.p_x = 0;
			orig.p_y = 0;
			set = (draw.Pixmap->Vdi > 0);
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			clnt->Fnct->grph_shift_arc(&orig, arc, len, gc->ArcMode);
			if (set)
			{
				vsf_color(hdl, color);
			}
			do
			{
				int i;

				vs_clip_pxy(hdl, (PXY *) (sect++));
				for (i = 0; i < len; ++i)
				{
					v_ellpie(hdl, arc[i].x, arc[i].y, arc[i].width, arc[i].height, arc[i].angle1, arc[i].angle2);
				}
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyLine(CLIENT *clnt, xPolyLineReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyLineReq)) / sizeof(PXY);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyLine, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyLine, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		PXY *pxy = (PXY *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			PXY orig;

			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				clnt->Fnct->grph_shift_pnt(&orig, pxy, len, q->coordMode);
				v_hide_c(hdl);
			}
			DEBUG(X_PolyLine, " W:%lX G:%lX (%lu)", q->drawable, q->gc, len);
		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			clnt->Fnct->grph_shift_pnt(NULL, pxy, len, q->coordMode);
			set = (draw.Pixmap->Vdi > 0);
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			if (set)
			{
				vsl_color(hdl, color);
				vsl_width(hdl, gc->LineWidth);
			}
			do
			{
				vs_clip_pxy(hdl, (PXY *) (sect++));
				v_pline(hdl, len, (short *) pxy);
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}

	} else
	{
		PRINT(X_PolyLine, " D:%lX G:%lX (0)", q->drawable, q->gc);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyPoint(CLIENT *clnt, xPolyPointReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyPointReq)) / sizeof(PXY);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyPoint, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyPoint, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		PXY *pxy = (PXY *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			PXY orig;

			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				clnt->Fnct->grph_shift_pnt(&orig, pxy, len, q->coordMode);
				v_hide_c(hdl);
			}
			DEBUG(X_PolyPoint, " W:%lX G:%lX (%lu)", q->drawable, q->gc, len);
		} else
		{								/* Pixmap */
			clnt->Fnct->grph_shift_pnt(NULL, pxy, len, q->coordMode);
			if (draw.p->Depth == 1)
			{
				PmapDrawPoints(draw.Pixmap, gc, pxy, len);
				nClp = 0;

			} else
			{
				if (gc->ClipNum > 0)
				{
					sect = alloca(sizeof(PRECT) * gc->ClipNum);
					nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
				} else
				{
					sect = alloca(sizeof(PRECT));
					nClp = SizeToPXY(sect, &draw.Pixmap->W);
				}
				set = (draw.Pixmap->Vdi > 0);
				hdl = PmapVdi(draw.Pixmap, gc, xFalse);
			}
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			if (set)
			{
				vsm_color(hdl, color);
			}
			do
			{
				vs_clip_pxy(hdl, (PXY *) (sect++));
				v_pmarker(hdl, len, (short *) pxy);
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}

	} else
	{
		PRINT(X_PolyPoint, " D:%lX G:%lX (0)", q->drawable, q->gc);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyFillRectangle(CLIENT *clnt, xPolyFillRectangleReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyFillRectangleReq)) / sizeof(GRECT);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyFillRectangle, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyFillRectangle, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		GRECT *rec = (GRECT *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		DEBUG(X_PolyFillRectangle, " %c:%lX G:%lX (%lu)", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc, len);

		if (draw.p->isWind)
		{
			PXY orig;

			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				clnt->Fnct->grph_shift_r2p(&orig, rec, len);
				v_hide_c(hdl);
			}

		} else
		{								/* Pixmap */
#if 0
			if (draw.p->Depth == 1)	/* disabled */
			{
				if (clnt->DoSwap)
				{
					size_t num = len;
					GRECT *rct = rec;

					while (num--)
					{
						SwapRCT(rct, rct);
						rct++;
					}
				}
				PmapFillRects (draw.Pixmap, gc, rec, len);
				nClp = 0;
			} else
#endif
			{
				if (gc->ClipNum > 0)
				{
					sect = alloca(sizeof(PRECT) * gc->ClipNum);
					nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
				} else
				{
					sect = alloca(sizeof(PRECT));
					nClp = SizeToPXY(sect, &draw.Pixmap->W);
				}
				clnt->Fnct->grph_shift_r2p(NULL, rec, len);
				set = (draw.Pixmap->Vdi > 0);
				hdl = PmapVdi(draw.Pixmap, gc, xFalse);
			}
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			if (set)
			{
				vsf_color(hdl, color);
			}
			do
			{
				int i;

				for (i = 0; i < len; i++)
				{
					PRECT clip = *(PRECT *) (rec + i);

					if (GrphIntersectP(&clip, sect))
					{
						v_bar(hdl, (short *) &clip.lu);
					}
				}
				sect++;
			} while (--nClp);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	} else
	{
		PRINT(X_PolyFillRectangle, " D:%lX G:%lX (0)", q->drawable, q->gc);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyRectangle(CLIENT *clnt, xPolyRectangleReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolyRectangleReq)) / sizeof(GRECT);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyRectangle, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyRectangle, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		GRECT *rec = (GRECT *) (q + 1);
		short hdl = GRPH_Vdi;
		PXY orig;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				v_hide_c(hdl);
			}
			DEBUG(X_PolyRectangle, " P:%lX G:%lX (%lu)", q->drawable, q->gc, len);
		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			orig.p_x = 0;
			orig.p_y = 0;
			set = (draw.Pixmap->Vdi > 0);
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			short d = vsl_width(hdl, gc->LineWidth);

			if (set)
			{
				vsl_color(hdl, color);
			}
			if (clnt->DoSwap)
			{
				size_t n = len;
				GRECT *r = rec;

				while (n--)
				{
					SwapRCT(r, r);
					r++;
				}
			}
			do
			{
				int i;

				vs_clip_pxy(hdl, (PXY *) (sect++));
				for (i = 0; i < len; ++i)
				{
					PXY p[5];

					p[0].p_x = p[3].p_x = p[4].p_x = rec[i].g_x + orig.p_x;
					p[1].p_x = p[2].p_x = p[0].p_x + rec[i].g_w - 1;
					p[0].p_y = p[1].p_y = rec[i].g_y + orig.p_y;
					p[2].p_y = p[3].p_y = p[0].p_y + rec[i].g_h - 1;
					p[4].p_y = p[0].p_y + d;
					v_pline(hdl, 5, &p[0].p_x);
				}
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolySegment(CLIENT *clnt, xPolySegmentReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);
	size_t len = ((q->length * 4) - sizeof(xPolySegmentReq)) / sizeof(PXY);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolySegment, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolySegment, "_");
	} else if (len && gc->ClipNum >= 0)
	{
		BOOL set = xTrue;
		PXY *pxy = (PXY *) (q + 1);
		short hdl = GRPH_Vdi;
		PRECT *sect;
		CARD16 nClp;
		CARD32 color;

		if (draw.p->isWind)
		{
			PXY orig;

			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				clnt->Fnct->grph_shift_pnt(&orig, pxy, len, CoordModeOrigin);
				v_hide_c(hdl);
			}
			DEBUG(X_PolySegment, " W:%lX G:%lX (%lu)", q->drawable, q->gc, len);
		} else
		{								/* Pixmap */
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			clnt->Fnct->grph_shift_pnt(NULL, pxy, len, CoordModeOrigin);
			set = (draw.Pixmap->Vdi > 0);
			hdl = PmapVdi(draw.Pixmap, gc, xFalse);
		}
		if (nClp && gc_mode(&color, &set, hdl, gc))
		{
			if (set)
			{
				vsl_color(hdl, color);
				vsl_width(hdl, gc->LineWidth);
			}
			do
			{
				int i;

				vs_clip_pxy(hdl, (PXY *) (sect++));
				for (i = 0; i < len; i += 2)
				{
					v_pline(hdl, 2, (short *) &pxy[i]);
				}
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	}
}

/* ------------------------------------------------------------------------------ */
static void _Image_Text(p_DRAWABLE draw, GC *gc, BOOL is8N16, void *text, short len, PXY *pos)
{
	short hdl = GRPH_Vdi;
	PRECT *sect;
	PXY orig;
	CARD16 nClp;

	if (draw.p->isWind)
	{
		nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
		if (nClp)
		{
			short dmy;

			orig.p_x += pos->p_x;
			orig.p_y += pos->p_y;
			vst_font(hdl, gc->FontIndex);
			vst_effects(hdl, gc->FontEffects);
			if (gc->FontWidth)
			{
				vst_height(hdl, gc->FontPoints, &dmy, &dmy, &dmy, &dmy);
				vst_width(hdl, gc->FontWidth, &dmy, &dmy, &dmy, &dmy);
			} else
			{
				vst_point(hdl, gc->FontPoints, &dmy, &dmy, &dmy, &dmy);
			}
			v_hide_c(hdl);
		}

	} else
	{									/* Pixmap */
		if (gc->ClipNum > 0)
		{
			sect = alloca(sizeof(PRECT) * gc->ClipNum);
			nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
		} else
		{
			sect = alloca(sizeof(PRECT));
			nClp = SizeToPXY(sect, &draw.Pixmap->W);
		}
		orig = *pos;
		hdl = PmapVdi(draw.Pixmap, gc, xTrue);
	}
	if (nClp)
	{
		BOOL bg_draw = (gc->Background != G_WHITE);
		const short *arr;
		short *tmp;

		if (is8N16)
		{
			tmp = alloca(sizeof(short) * len);
			arr = FontTrans_C(tmp, text, len, gc->FontFace);
		} else
		{
			tmp = text;
			arr = FontTrans_W(tmp, text, len, gc->FontFace);
		}
		if (!bg_draw)
		{
			vswr_mode(hdl, MD_REPLACE);
			vst_color(hdl, gc->Foreground);
		}
		do
		{
			vs_clip_pxy(hdl, (PXY *) (sect++));
			if (bg_draw)
			{
				vswr_mode(hdl, MD_ERASE);
				vst_color(hdl, gc->Background);
				v_gtext16n(hdl, orig, arr, len);
				vswr_mode(hdl, MD_TRANS);
				vst_color(hdl, gc->Foreground);
			}
			v_gtext16n(hdl, orig, arr, len);
		} while (--nClp);
		vs_clip_off(hdl);

		if (draw.p->isWind)
		{
			v_show_c(hdl, 1);
			WindClipOff();
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_ImageText8(CLIENT *clnt, xImageTextReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_ImageText8, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_ImageText8, "_");
	} else if (q->nChars && gc->ClipNum >= 0)
	{
		DEBUG(X_ImageText8, " %c:%lX G:%lX (%i,%i)", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc, q->x, q->y);

		_Image_Text(draw, gc, xTrue, (char *) (q + 1), q->nChars, (PXY *) & q->x);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_ImageText16(CLIENT *clnt, xImageTextReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_ImageText16, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_ImageText16, "_");
	} else if (q->nChars && gc->ClipNum >= 0)
	{
		DEBUG(X_ImageText16, " %c:%lX G:%lX (%i,%i)", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc, q->x, q->y);

		_Image_Text(draw, gc, xFalse, (char *) (q + 1), q->nChars, (PXY *) & q->x);
	}
}

/* ------------------------------------------------------------------------------ */
static void _Poly_Text(p_DRAWABLE draw, GC *gc, BOOL is8N16, xTextElt *t, PXY *pos)
{
	if (t->len)
	{
		short hdl = GRPH_Vdi;
		PRECT *sect;
		PXY orig;
		CARD16 nClp;

		if (draw.p->isWind)
		{
			nClp = WindClipLock(draw.Window, 0, gc->ClipRect, gc->ClipNum, &orig, &sect, gc->SubwindMode);
			if (nClp)
			{
				short dmy;

				orig.p_x += pos->p_x;
				orig.p_y += pos->p_y;
				vst_font(hdl, gc->FontIndex);
				vst_color(hdl, gc->Foreground);
				vst_effects(hdl, gc->FontEffects);
				if (gc->FontWidth)
				{
					vst_height(hdl, gc->FontPoints, &dmy, &dmy, &dmy, &dmy);
					vst_width(hdl, gc->FontWidth, &dmy, &dmy, &dmy, &dmy);
				} else
				{
					vst_point(hdl, gc->FontPoints, &dmy, &dmy, &dmy, &dmy);
				}
				v_hide_c(hdl);
			}

		} else
		{								/* Pixmap */
#if 0
			DEBUG(X_PolyText8, " P:%lX G:%lX (%i,%i)", q->drawable, q->gc, q->x, q->y);
#endif
			if (gc->ClipNum > 0)
			{
				sect = alloca(sizeof(PRECT) * gc->ClipNum);
				nClp = SizeToLst(sect, gc->ClipRect, gc->ClipNum, &draw.Pixmap->W);
			} else
			{
				sect = alloca(sizeof(PRECT));
				nClp = SizeToPXY(sect, &draw.Pixmap->W);
			}
			orig = *pos;
			hdl = PmapVdi(draw.Pixmap, gc, xTrue);
		}
		if (nClp)
		{
			const short *arr;
			short *tmp;

			if (is8N16)
			{
				tmp = alloca(sizeof(short) * t->len);
				arr = FontTrans_C(tmp, (char *) (t + 1), t->len, gc->FontFace);
			} else
			{
				tmp = (short *) (t + 1);
				arr = FontTrans_W(tmp, tmp, t->len, gc->FontFace);
			}
			vswr_mode(hdl, MD_TRANS);
			do
			{
				vs_clip_pxy(hdl, (PXY *) (sect++));
				v_gtext16n(hdl, orig, arr, t->len);
			} while (--nClp);
			vs_clip_off(hdl);

			if (draw.p->isWind)
			{
				v_show_c(hdl, 1);
				WindClipOff();
			}
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyText8(CLIENT *clnt, xPolyTextReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyText8, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyText8, "_");
	} else if (gc->ClipNum >= 0)
	{
		DEBUG(X_PolyText8, " %c:%lX G:%lX (%i,%i)", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc, q->x, q->y);

		_Poly_Text(draw, gc, xTrue, (xTextElt *) (q + 1), (PXY *) & q->x);
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_PolyText16(CLIENT *clnt, xPolyTextReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	p_GC gc = GcntFind(q->gc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PolyText16, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->drawable, X_PolyText16, "_");
	} else if (gc->ClipNum >= 0)
	{
		DEBUG(X_PolyText16, " %c:%lX G:%lX (%i,%i)", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc, q->x, q->y);

		_Poly_Text(draw, gc, xFalse, (xTextElt *) (q + 1), (PXY *) & q->x);
	}
}
