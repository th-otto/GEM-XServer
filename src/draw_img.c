/*
 *==============================================================================
 *
 * draw_img.c
 *
 * Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2001-02-03 - Initial Version.
 *==============================================================================
 */
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "clnt.h"
#include "pixmap.h"
#include "window.h"
#include "event.h"
#include "gcontext.h"
#include "grph.h"
#include "x_gem.h"
#include "x_printf.h"


/* ============================================================================== */
/* */
/* Callback Functions */

#include "Request.h"

/* ------------------------------------------------------------------------------ */
void RQ_PutImage(CLIENT *clnt, xPutImageReq *q)
{
	p_DRAWABLE draw = DrawFind(q->drawable);
	GC *gc = GcntFind(q->gc);

	if (!draw.p)
	{
		Bad(BadDrawable, q->drawable, X_PutImage, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->gc, X_PutImage, "_");
	} else if ((q->format == XYBitmap && q->depth != 1) || (q->format != XYBitmap && q->depth != draw.p->Depth))
	{
		Bad(BadMatch, None, X_PutImage, "_" /* q->depth */);
	} else if ((q->format == ZPixmap && q->leftPad) || q->leftPad >= PADD_BITS)
	{
		Bad(BadMatch, None, X_PutImage, "_" /* q->leftPad */ );
	} else if ((short) q->width <= 0 || (short) q->height <= 0)
	{
		Bad(BadValue, (short) ((short) q->width <= 0 ? q->width : q->height), X_PutImage, "_ width = %i height = %i", (short) q->width, (short) q->height);
	} else
	{
		GRECT r[2] = { {0, 0, q->width, q->height}
		};

		if (q->dstX < 0)
		{
			r[1].g_x = 0;
			r[0].g_w -= r[0].g_x = -q->dstX;
		} else
		{
			r[1].g_x = q->dstX;
		}
		if (q->dstY < 0)
		{
			r[1].g_y = 0;
			r[0].g_h -= r[0].g_y = -q->dstY;
		} else
		{
			r[1].g_y = q->dstY;
		}
		if (r[0].g_x + r[0].g_w > draw.p->W)
			r[0].g_w = draw.p->W - r[0].g_x;
		if (r[0].g_y + r[0].g_h > draw.p->H)
			r[0].g_h = draw.p->H - r[0].g_y;

		if (r[0].g_w > 0 && r[0].g_h > 0)
		{
			MFDB mfdb = { (q + 1), q->width, q->height,
				(q->width + q->leftPad + PADD_BITS - 1) / 16,
				0, q->depth, 0, 0, 0
			};

			r[0].g_x += q->leftPad;
			r[1].g_w = r[0].g_w;
			r[1].g_h = r[0].g_h;

			if (q->depth == 1)
			{							/* all possible formats are matching */

				DEBUG(X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
					  "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
					  (q->format == XYBitmap ? "Bitmap" :
					   q->format == XYPixmap ? "XYPixmap" :
					   q->format == ZPixmap ? "ZPixmap" : "???"),
					  (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
					  q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
					  (q->length * 4) - sizeof(xPutImageReq),
					  r[0].g_x, r[0].g_y, r[0].g_w, r[0].g_h, r[1].g_x, r[1].g_y, r[1].g_w, r[1].g_h);

				if (draw.p->isWind)
					WindPutMono(draw.Window, gc, r, &mfdb);
				else
					PmapPutMono(draw.Pixmap, gc, r, &mfdb);

			} else if (q->format == XYPixmap)
			{

				PRINT(-X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
					  "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
					  "XYPixmap", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
					  q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
					  (q->length * 4) - sizeof(xPutImageReq),
					  r[0].g_x, r[0].g_y, r[0].g_w, r[0].g_h, r[1].g_x, r[1].g_y, r[1].g_w, r[1].g_h);

			} else if (!GrphRasterPut(&mfdb, q->width, q->height))
			{
				x_printf("PutImage: Can't allocate buffer.\n");
			} else
			{							/* q->format == ZPixmap */

				DEBUG(X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
					  "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
					  "ZPixmap", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
					  q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
					  (q->length * 4) - sizeof(xPutImageReq),
					  r[0].g_x, r[0].g_y, r[0].g_w, r[0].g_h, r[1].g_x, r[1].g_y, r[1].g_w, r[1].g_h);

				if (draw.p->isWind)
					WindPutColor(draw.Window, gc, r, &mfdb);
				else
					PmapPutColor(draw.Pixmap, gc, r, &mfdb);

				if (mfdb.fd_addr != (q + 1))
					free(mfdb.fd_addr);
			}

		} else
		{
			PRINT(-X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
				  "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
				  (q->format == XYBitmap ? "Bitmap" :
				   q->format == XYPixmap ? "XYPixmap" :
				   q->format == ZPixmap ? "ZPixmap" : "???"),
				  (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
				  q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
				  (q->length * 4) - sizeof(xPutImageReq),
				  r[0].g_x, r[0].g_y, r[0].g_w, r[0].g_h, r[1].g_x, r[1].g_y, r[1].g_w, r[1].g_h);
		}
	}
}

/* ------------------------------------------------------------------------------ */
void RQ_GetImage(CLIENT *clnt, xGetImageReq *q)
{
	/*
	 * CARD8    format:
	 * Drawable drawable:
	 * INT16    x, y:
	 * CARD16   width, height:
	 * CARD32   planeMask
	 *
	 * Reply:
	 * CARD8    depth:
	 * VisualID visual:
	 * (char*)  (r +1):
	 *...........................................................................
	 */
	p_DRAWABLE draw = { NULL };
	PRECT rec[2] = {
		{ { q->x, q->y }, { q->width - 1, q->height - 1 } },
		{ { 0, 0 }, { 0, 0 } }
	};
	BOOL ok = xFalse;

	if ((q->drawable & ~RID_MASK) && !(draw = DrawFind(q->drawable)).p)
	{
		Bad(BadDrawable, q->drawable, X_GetImage, "_");
	} else if (!(q->drawable & ~RID_MASK) && !wind_get_work(q->drawable & 0x7FFF, (GRECT *) & rec[1]))
	{
		Bad(BadDrawable, q->drawable, X_GetImage, "_");
	} else if (q->format != XYPixmap && q->format != ZPixmap)
	{
		Bad(BadValue, q->format, X_GetImage, "_");
	} else
	{
		short xy, w, h;

		if (draw.p)
		{
			xy = (draw.p->isWind ? -draw.Window->BorderWidth : 0);
			w = draw.p->W - xy;
			h = draw.p->H - xy;
		} else
		{
			xy = -1;
			w = rec[1].rd.p_x;
			h = rec[1].rd.p_y;
		}
		if (q->x < xy || q->x + q->width > w || q->y < xy || q->y + q->height > h)
		{
			Bad(BadMatch, None, X_GetImage, "_");
		} else
		{
			rec[1].rd = rec[0].rd;
			ok = xTrue;
		}
	}
	if (ok)
	{
		short dpth = (draw.p ? draw.p->Depth : GRPH_Depth);
		size_t size = (q->width * dpth + 7) / 8 * q->height;

		ClntReplyPtr(GetImage, r, size);

		if (!r)
		{
			Bad(BadMatch, None, X_GetImage, "_ memory exhausted in output buffer (%li).", size);
		} else
		{
			MFDB dst = { (r + 1), q->width, q->height,
				(q->width + PADD_BITS - 1) / 16, 0, dpth, 0, 0, 0
			};
			MFDB *src = NULL;

			PRINT(-X_GetImage,
				  " D:%lX [%i,%i/%u,%u] form=%i mask=%lX -> dpth = %i size = %lu",
				  q->drawable, q->x, q->y, q->width, q->height, q->format, q->planeMask, dpth, size);

			if (!draw.p)
			{
				rec[0].rd.p_x += (rec[0].lu.p_x += rec[1].lu.p_x);
				rec[0].rd.p_y += (rec[0].lu.p_y += rec[1].lu.p_y);
				rec[1].lu.p_x = rec[1].lu.p_y = 0;
			} else if (draw.p->isWind)
			{
				PXY pos;

				pos = WindOrigin(draw.Window);
				rec[0].rd.p_x += (rec[0].lu.p_x += pos.p_x);
				rec[0].rd.p_y += (rec[0].lu.p_y += pos.p_y);
			} else
			{
				src = PmapMFDB(draw.Pixmap);
			}

			if (dpth == 1)
			{							/* all possible formats are matching */
				if (!src)
				{
					src = alloca(sizeof(MFDB));
					src->fd_addr = NULL;
				}
				vro_cpyfm(GRPH_Vdi, S_ONLY, (short *) rec, src, &dst);

			} else if (!GrphRasterGet(&dst, rec, src))
			{
				x_printf("GetImage: Can't allocate buffer.\n");
			} else
			{
				if (dst.fd_addr != (q + 1))
					free(dst.fd_addr);
			}

			if (!draw.p)
			{
				r->visual = (GRPH_Depth > 1 ? DFLT_VISUAL + 1 : DFLT_VISUAL);
			} else if (draw.p->isWind)
			{
				r->visual = (draw.p->Depth > 1 ? DFLT_VISUAL + 1 : DFLT_VISUAL);
			} else
			{
				r->visual = None;
			}
			r->depth = dpth;

			ClntReply(GetImage, size, NULL);
		}
	}
}


/* ------------------------------------------------------------------------------ */
#define CPsrcP 0x01
#define CPsrcW 0x02
#define CPdstP 0x10
#define CPdstW 0x20
/* ------------------------------------------------------------------------------ */
static short clip_dst(GRECT *r, p_DRAWABLE draw, const short *coord)
{
	/*
	 * intersects source and destination to the size of destination, the result
	 * are the parts of both which has to be handled at all.
	 *
	 * coord[0,1] src x/y   r[0] src clip
	 * coord[2,3] dst x/y   r[1] dst clip
	 * coord[4,5] w/h
	 */
	short w = coord[4];
	short h = coord[5];
	short t;

	if (!draw.p->isWind)
		t = CPdstP;
	else if (WindVisible(draw.Window))
		t = CPdstW;
	else
		return 0;

	*(PXY *) & r[0] = *(PXY *) (coord + 0);	/* src */
	*(PXY *) & r[1] = *(PXY *) (coord + 2);	/* dst */

	if (r[1].g_x < 0)
	{
		r[0].g_x -= r[1].g_x;
		w += r[1].g_x;
		r[1].g_x = 0;
	}
	if (r[1].g_x + w > draw.p->W)
		w = draw.p->W - r[1].g_x;
	if (r[1].g_y < 0)
	{
		r[0].g_y -= r[1].g_y;
		h += r[1].g_y;
		r[1].g_y = 0;
	}
	if (r[1].g_y + h > draw.p->H)
		h = draw.p->H - r[1].g_y;

	if (w <= 0 || h <= 0)
		return 0;

	r[0].g_w = r[1].g_w = w;
	r[0].g_h = r[1].g_h = h;

	return t;
}

/* ------------------------------------------------------------------------------ */
static short clip_src(GRECT *r, p_DRAWABLE draw)
{
	/* r[0]   src clip */
	/* r[1]   dst clip */
	/* r[2..] dst to be tiled */
	short num = 0;						/* -1: no copy, 0: no tiles, 1..4: num of tiles in r */
	GRECT *tile = r + 2;
	BOOL below = xFalse;
	short x = r[1].g_x;
	short y = r[1].g_y;
	short w = r[1].g_w;
	short h = r[1].g_h;
	short d;

#define SET(t, _x,_y,_w,_h)   t->g_x = _x; t->g_y = _y; t->g_w = _w; t->g_h = _h

	if (h > (d = draw.p->H - r[0].g_y))
	{									/* below src */
		below = xTrue;
		SET((r + 5), x, y + d, w, h - d);
		h = d;
		if (h <= 0)
			return -1;
	}
	if (r[0].g_y < 0)
	{									/* above src */
		if ((h += r[0].g_y) <= 0)
			return -1;
		num++;
		SET(tile, x, y, w, -r[0].g_y);
		tile++;
		y -= r[0].g_y;
		r[0].g_y = 0;
	}
	if (r[0].g_x < 0)
	{									/* left of src */
		if ((w += r[0].g_x) <= 0)
			return -1;
		num++;
		SET(tile, x, y, -r[0].g_x, h);
		tile++;
		x -= r[0].g_x;
		r[0].g_x = 0;
	}
	if (w > (d = draw.p->W - r[0].g_x))
	{									/* right of src */
		num++;
		SET(tile, x + d, y, w - d, h);
		tile++;
		w = d;
		if (w <= 0)
			return -1;
	}
	if (below && (++num < 4))
	{
		*tile = r[5];
	}
	r[1].g_x = x;
	r[1].g_y = y;
	r[1].g_w = r[0].g_w = w;
	r[1].g_h = r[0].g_h = h;

#undef SET

	return num;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void RQ_CopyArea(CLIENT *clnt, xCopyAreaReq *q)
{
	p_DRAWABLE src_d = DrawFind(q->srcDrawable);
	p_DRAWABLE dst_d = DrawFind(q->dstDrawable);
	GC *gc = GcntFind(q->gc);

	if (!src_d.p)
	{
		Bad(BadDrawable, q->srcDrawable, X_CopyArea, "_");
	} else if (!dst_d.p)
	{
		Bad(BadDrawable, q->dstDrawable, X_CopyArea, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->gc, X_CopyArea, "_");
	} else if (src_d.p->Depth != dst_d.p->Depth)
	{
		Bad(BadMatch, None, X_CopyArea, "_(%c:%X,%c:%X):\n           depth %u != %u.",
			(src_d.p->isWind ? 'W' : 'P'), src_d.p->Id,
			(dst_d.p->isWind ? 'W' : 'P'), dst_d.p->Id, src_d.p->Depth, dst_d.p->Depth);
	} else if ((short) q->width < 0 || (short) q->height < 0)
	{
		if (gc->GraphExpos)
		{
			EvntNoExposure(clnt, dst_d.p->Id, X_CopyArea);
		}
		return;
		Bad(BadValue, (short) ((short) q->width <= 0 ? q->width : q->height), X_CopyArea, "_(%c:%X,%c:%X) [%i,%i] -> [%i,%i] \n"
			"           width = %i height = %i",
			(src_d.p->isWind ? 'W' : 'P'), src_d.p->Id,
			(dst_d.p->isWind ? 'W' : 'P'), dst_d.p->Id,
			q->srcX, q->srcY, q->dstX, q->dstY,
			(short) q->width, (short) q->height);
	} else
	{
		BOOL debug = xTrue;
		GRECT rect[6];
		GRECT work;
		GRECT *exps = rect + 2;
		short nExp = -1;
		PRECT *sect = NULL;
		short nSct = 0;
		short action = 0xFF;

		if (gc->ClipNum < 0 || !q->width || !q->height || !(action = clip_dst(rect, dst_d, &q->srcX)))
		{
			/*
			 * the area doesn't intersects the destination drawable or the
			 * drawable isn't viewable
			 */
			DEBUG(-X_CopyArea, " G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX (%i,%i)"
				  " %c <%02X>",
				  q->gc, (src_d.p->isWind ? 'W' : 'P'), q->srcDrawable,
				  q->srcX, q->srcY, q->width, q->height,
				  (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable,
				  q->dstX, q->dstY, (gc->GraphExpos ? '*' : '-'), action);
			if (gc->GraphExpos)
			{
				EvntNoExposure(clnt, dst_d.p->Id, X_CopyArea);
			}
			return;
		}
		if (!src_d.p->isWind)
		{
			action |= CPsrcP;
		} else if (src_d.p == dst_d.p || (action == CPdstP && WindVisible(src_d.Window)))
		{
			action |= CPsrcW;
		}
		/* else: action = 0x00|CPdstW */

		if (action == (CPsrcW | CPdstW))
		{
			/*
			 * copy inside one window means scroll part(s) of this window
			 */
			PXY diff = { rect[1].g_x - rect[0].g_x, rect[1].g_y - rect[0].g_y };

			if (diff.p_x || diff.p_y)
			{
				WINDOW *wind = dst_d.Window;
				PXY orig;

				GrphCombine(&rect[0], &rect[1]);
				nSct = WindClipLock(wind, 0, rect, 1, &orig, &sect, IncludeInferiors);
				if (nSct)
				{
					exps = (gc->GraphExpos ? (GRECT *) (sect + nSct + 1) : NULL);
					nExp = WindScroll(wind, gc, rect, diff, orig, sect, nSct, exps);
				}
				debug = xFalse;
			}

		} else
		{
			/*
			 * action: 0x00|CPdstW, CPsrcP|CPdstP, CPsrcP|CPdstW, CPsrcW|CPdstP
			 */
			work = rect[1];				/* save the uncropped destination area */

			if (action & (CPsrcP | CPsrcW))
			{
				/*
				 * the action is one of: CPsrcP|CPdstP, CPsrcP|CPdstW, CPsrcW|CPdstP
				 * now get this regions of destination which can't be copied from
				 * source into r[2..nExp+1] (refferenced by exps[])
				 */
				nExp = clip_src(rect, src_d);
			}
			if (nExp)
			{
				if (nExp < 0)
				{
					/*
					 * no part of the source intersects the destination, or both are
					 * different windows, so only generate events for the visible
					 * parts
					 */
					exps = &rect[1];	/* set to uncropped destination now */
					nExp = 1;
					action &= ~(CPsrcP | CPsrcW);	/* don't handle source anymore */
				}
				if (gc->ClipNum > 0)
				{
					/*
					 * intersects destination and clipping rectangels for events
					 */
					GRECT *r = exps;

					exps = alloca(sizeof(GRECT) * nExp * gc->ClipNum);
					nExp = GrphInterList(exps, gc->ClipRect, gc->ClipNum, r, nExp);

					if (!nExp && !(action & (CPsrcP | CPsrcW)))
					{
						/*
						 * no intersections found and due to no source regions to be
						 * handled, so no further actions are necessary
						 */
						action = 0x00;
					}
				}
			}
			if (action & CPdstW)
			{
				/*
				 * for _all_ cases of copy to a window, there might it be necessary
				 * to generate events and/or fill background for visible regions of
				 * destination which can't be copied from source
				 */
				WINDOW *wind = dst_d.Window;
				PXY orig;

				nSct = WindClipLock(wind, 0, &work, 1, &orig, &sect, gc->SubwindMode);
				if (nSct == 0)
				{
					/*
					 * if no visible region of the whole source rectangle exists no
					 * more action is taken
					 */
					action = 0x00;
				} else if (nExp)
				{
					GRECT *r = (gc->GraphExpos ? (GRECT *) (sect + nSct) : NULL);
					short e = 0;

					if (wind->hasBackGnd)
					{
						if (!wind->hasBackPix)
						{
							vswr_mode(GRPH_Vdi, MD_REPLACE);
							vsf_color(GRPH_Vdi, wind->Back.Pixel);
						}
						do
						{
							PRECT area;
							int n;

							area.rd.p_x = (area.lu.p_x = orig.p_x + exps->g_x) + exps->g_w - 1;
							area.rd.p_y = (area.lu.p_y = orig.p_y + exps->g_y) + exps->g_h - 1;
							n = WindDrawBgnd(wind, orig, &area, sect, nSct, r);
							if (n && r)
							{
								e += n;
								r += n;
							}
							exps++;
						} while (--nExp);

					} else if (gc->GraphExpos)
					{
						PRECT *s = sect;
						short n = nSct;

						do
						{
							GRECT a = { s->lu.p_x - orig.p_x, s->lu.p_y - orig.p_y,
								s->rd.p_x - s->lu.p_x + 1, s->rd.p_y - s->lu.p_y + 1
							};
							e += GrphInterList(r + e, &a, 1, exps, nExp);
							s++;
						} while (--n);
					}
					nExp = e;
					exps = (GRECT *) (sect + nSct);
				}
				if (action == (CPsrcP | CPdstW))
				{
					WindPutImg(dst_d.Window, gc, rect, PmapMFDB(src_d.Pixmap), orig, sect, nSct);
				}
				debug = xFalse;
			} else
			{							/* destination is pixmap */
				if (action == (CPsrcP | CPdstP))
				{
					if (src_d.p->Depth == 1)
					{
						PmapPutMono(dst_d.Pixmap, gc, rect, PmapMFDB(src_d.Pixmap));
					} else
					{
						PmapPutColor(dst_d.Pixmap, gc, rect, PmapMFDB(src_d.Pixmap));
					}
					debug = xFalse;

				} else
				{						/* (action == (CPsrcW|CPdstP)) */
					/*
					 * not handled yet
					 */
				}
			}
		}

		if (gc->GraphExpos)
		{
			if (nExp)
				EvntGraphExpose(clnt, dst_d, X_CopyArea, nExp, exps);
			else
				EvntNoExposure(clnt, dst_d.p->Id, X_CopyArea);
		}

		if (sect)
			WindClipOff();

		if (debug)
		{
			PRINT(X_CopyArea, " G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX (%i,%i)\n"
				  "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]  %c <%02X>",
				  q->gc, (src_d.p->isWind ? 'W' : 'P'), q->srcDrawable,
				  q->srcX, q->srcY, q->width, q->height,
				  (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable,
				  q->dstX, q->dstY,
				  rect[0].g_x, rect[0].g_y, rect[0].g_w, rect[0].g_h,
				  rect[1].g_x, rect[1].g_y, rect[1].g_w, rect[1].g_h, (gc->GraphExpos ? '*' : '-'), action);
		}
	}
}

/* ------------------------------------------------------------------------------ */
static int _mask2plane(CARD32 mask, CARD16 depth)
{
	int plane = -1;

	if (mask)
		while (++plane < depth)
		{
			if (mask & 1)
			{
				mask &= ~1uL;
				break;
			}
			mask >>= 1;
		}
	return (mask ? -1 : plane);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void RQ_CopyPlane(CLIENT *clnt, xCopyPlaneReq *q)
{
	p_DRAWABLE src_d = DrawFind(q->srcDrawable);
	p_DRAWABLE dst_d = DrawFind(q->dstDrawable);
	GC *gc = GcntFind(q->gc);
	int plane;

	if (!src_d.p)
	{
		Bad(BadDrawable, q->srcDrawable, X_CopyPlane, "_");
	} else if (!dst_d.p)
	{
		Bad(BadDrawable, q->dstDrawable, X_CopyPlane, "_");
	} else if (!gc)
	{
		Bad(BadGC, q->gc, X_CopyPlane, "_");
	} else if ((plane = _mask2plane(q->bitPlane, src_d.p->Depth)) < 0)
	{
		Bad(BadValue, q->bitPlane, X_CopyPlane, "_ bit-plane = %lX", q->bitPlane);
	} else if ((short) q->width <= 0 || (short) q->height <= 0)
	{
		Bad(BadValue, (short) ((short) q->width <= 0 ? q->width : q->height), X_CopyPlane, "_ width = %i height = %i", (short) q->width, (short) q->height);
	} else
	{
		BOOL debug = xTrue;
		GRECT r[4] = {
			{ 0, 0, src_d.p->W, src_d.p->H },
			{ 0, 0, dst_d.p->W, dst_d.p->H },
			{ q->srcX, q->srcY, q->width, q->height },
			{ q->dstX, q->dstY, q->width, q->height }
		};

		if (GrphIntersect(&r[0], &r[2]) && GrphIntersect(&r[1], &r[3]))
		{
			if (r[0].g_w > r[1].g_w)
				r[0].g_w = r[1].g_w;
			else
				r[1].g_w = r[0].g_w;
			if (r[0].g_h > r[1].g_h)
				r[0].g_h = r[1].g_h;
			else
				r[1].g_h = r[0].g_h;

			if (src_d.p->isWind)
			{
				/* */

			} else
			{							/* src_d is Pixmap */
				MFDB mfdb = *PmapMFDB(src_d.Pixmap);

				DEBUG(X_CopyPlane, " #%i(0x%lx)"
					  " G:%lX %c:%lX [%i,%i/%u,%u *%i] to %c:%lX %i,%i",
					  plane, q->bitPlane, q->gc, (src_d.p->isWind ? 'W' : 'P'),
					  q->srcDrawable, q->srcX, q->srcY, q->width, q->height,
					  src_d.p->Depth, (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable, q->dstX, q->dstY);

				if (src_d.p->Depth > 1)
				{
					if (plane)
					{
						mfdb.fd_addr = src_d.Pixmap->Mem + src_d.Pixmap->nPads * 2 * src_d.Pixmap->H * plane;
					}
					mfdb.fd_nplanes = 1;
				}
				if (dst_d.p->isWind)
					WindPutMono(dst_d.Window, gc, r, &mfdb);
				else
					PmapPutMono(dst_d.Pixmap, gc, r, &mfdb);
				debug = xFalse;
			}

		}
		if (debug)
		{
			PRINT(-X_CopyPlane, " #%i(0x%lx)"
				  " G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX %i,%i",
				  plane, q->bitPlane, q->gc, (src_d.p->isWind ? 'W' : 'P'),
				  q->srcDrawable, q->srcX, q->srcY, q->width, q->height,
				  (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable, q->dstX, q->dstY);
		}
		/*
		 * Hacking: GraphicsExposure should be generated if necessary instead!
		 */
		if (gc->GraphExpos)
		{
			EvntNoExposure(clnt, dst_d.p->Id, X_CopyPlane);
		}
	}
}
