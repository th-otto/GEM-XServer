//==============================================================================
//
// wmgr_draw.c
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-07-03 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "window.h"
#include "grph.h"
#include "x_gem.h"
#include "wmgr.h"
#include "Property.h"


//------------------------------------------------------------------------------
static short
dcr_xyxy (PXY * p, short x1, short y1, short x2, short y2)
{
	p[1].p_y          = y1;
	p[1].p_x = p[2].p_x = x1;
	p[2].p_y = p[3].p_y = y2;
	p[3].p_x          = x2;
	
	return (p->p_y = 3) +1;
}
#define DCR_XYXY(p, r, X1,Y1, X2,Y2) p += dcr_xyxy (p, r->X1, r->Y1, r->X2, r->Y2)

//------------------------------------------------------------------------------
static short
dcr_xxy (PXY * p, short x1, short x2, short y2)
{
	p[1].p_x          = x1;
	p[1].p_y = p[2].p_y = y2;
	p[2].p_x          = x2;
	
	return (p->p_y = 2) +1;
}
#define DCR_XXY(p, r, X1, X2,Y2)   p += dcr_xxy (p, r->X1, r->X2, r->Y2)

//------------------------------------------------------------------------------
static short
dcr_xyy (PXY * p, short x1, short y1, short y2)
{
	p[1].p_y          = y1;
	p[1].p_x = p[2].p_x = x1;
	p[2].p_y          = y2;
	
	return (p->p_y = 2) +1;
}
#define DCR_XYY(p, r, X1,Y1, Y2)   p += dcr_xyy (p, r->X1, r->Y1, r->Y2)

//==============================================================================
void
WmgrDrawDeco (WINDOW * wind, PRECT * work, PRECT * area, PRECT * sect, int num)
{
	short decor_size = (WMGR_Decor *3) /2;
	BOOL  sizeable   = !(wind->Properties && wind->Properties->FixedSize);
	int   f = 0, i;
	PRECT frm[3];
	int   d = 0;
	PXY   __d[28] = { {0,4}, {work->lu.p_x -1, work->lu.p_y   },
	                         {work->lu.p_x -1, work->rd.p_y +1},
	                         {work->rd.p_x +1, work->rd.p_y +1},
	                         {work->rd.p_x +1, work->lu.p_y   }, },
	               * drk = __d +5;
	PXY   __w[25], * wht = __w;
	PXY   __b[13], * blk = __b;
	BOOL  inside;
	
	#define DECOR(n)   (WMGR_Decor - n)
	#define EDGE(n)    (decor_size + n)
	
	//__________________________________left_side___
	frm[f].lu.p_x = 0;
	frm[f].lu.p_y = 0;
	frm[f].rd.p_x = work->lu.p_x -1;
	frm[f].rd.p_y = work->rd.p_y;
	if ((inside = GrphIntersectP (frm +f, area))) {
		f++;
	}
	if (sizeable) {
		if (inside) {
			if (area->lu.p_y < work->rd.p_y - decor_size) {
				DCR_XYXY (wht, work, lu.p_x - DECOR(0), rd.p_y - EDGE(+3),
				                     lu.p_x        -3,  lu.p_y           );
				DCR_XYXY (drk, work, lu.p_x        -2,  lu.p_y        +1,
				                     lu.p_x - DECOR(1), rd.p_y - EDGE(+2));
				DCR_XXY  (blk, work, lu.p_x - DECOR(0),
				                     lu.p_x        -2,  rd.p_y - EDGE(+1));
			}
		} else {
			__d[1].p_y = 3;
			d        = 1;
		}
		if (inside || area->rd.p_y > work->rd.p_y +1) {
			DCR_XYXY (wht, work, lu.p_x - DECOR(0), rd.p_y + DECOR(1),
			                     lu.p_x        -3,  rd.p_y - EDGE(00));
			DCR_XYY  (drk, work, lu.p_x        -2,  rd.p_y - EDGE(-1),
			                                      rd.p_y        +1 );
			DCR_XXY  (wht, work, lu.p_x        -1,
			                     lu.p_x + EDGE(-1), rd.p_y        +2 );
			DCR_XYXY (drk, work, lu.p_x + EDGE(00), rd.p_y +      +3,
			                     lu.p_x - DECOR(1), rd.p_y + DECOR(0));
		}
	}
	//_____________________________________bottom___
	frm[f].lu.p_x = 0;
	frm[f].lu.p_y = work->rd.p_y +1;
	frm[f].rd.p_x = 0x7FFF;
	frm[f].rd.p_y = 0x7FFF;
	if (GrphIntersectP (frm +f, area)) {
		f++;
		if (sizeable) {
			if (area->rd.p_x > work->lu.p_x + decor_size &&
			    area->lu.p_x < work->rd.p_x - decor_size) {
				DCR_XYY  (blk, work, lu.p_x + EDGE(+1), rd.p_y        +2,
				                                      rd.p_y + DECOR(0));
				DCR_XYXY (wht, work, lu.p_x + EDGE(+2), rd.p_y + DECOR(1),
				                     rd.p_x - EDGE(+3), rd.p_y        +2 );
				DCR_XYXY (drk, work, rd.p_x - EDGE(+2), rd.p_y        +3,
				                     lu.p_x + EDGE(+3), rd.p_y + DECOR(0));
				DCR_XYY  (blk, work, rd.p_x - EDGE(+1), rd.p_y        +2,
				                                      rd.p_y + DECOR(0));
			}
		} else if (d) {
			__d[2].p_y = 2;
			d        = 2;
		}
	}
	//_________________________________right_side___
	frm[f].lu.p_x = work->rd.p_x + 1;
	frm[f].lu.p_y = 0;
	frm[f].rd.p_x = 0x7FFF;
	frm[f].rd.p_y = work->rd.p_y;
	if ((inside = GrphIntersectP (frm +f, area))) {
		f++;
	}
	if (sizeable) {
		if (inside) {
			if (area->lu.p_y < work->rd.p_y - decor_size) {
				DCR_XYXY (wht, work, rd.p_x        +2,  rd.p_y - EDGE(+3),
				                     rd.p_x + DECOR(1), lu.p_y           );
				DCR_XYXY (drk, work, rd.p_x + DECOR(0), lu.p_y        +1,
				                     rd.p_x        +3,  rd.p_y - EDGE(+2));
				DCR_XXY  (blk, work, rd.p_x        +2,
				                     rd.p_x + DECOR(0), rd.p_y - EDGE(+1));
			}
		} else if (d == 2) {
			d = 5;
		} else { // d == 1 || d == 0
			__d[4] = __d[3];
			__d[3] = __d[2];
			if (d) {
				__d[2].p_y = 2;
				d        = 2;
			} else {
				__d[2]   = __d[1];
				__d[1].p_y = 3;
				d        = 1;
			}
		}
		if (inside || area->rd.p_y > work->rd.p_y +1) {
			DCR_XYXY (wht, work, rd.p_x - EDGE(00), rd.p_y + DECOR(1),
			                     rd.p_x        +2,  rd.p_y        +2 );
			wht[0].p_x            = work->rd.p_x        +2;
			wht[0].p_y = wht[1].p_y = work->rd.p_y - EDGE(00);
			wht[1].p_x            = work->rd.p_x + DECOR(1);
			wht[-4].p_y += 2;
			wht       += 2;
			DCR_XYXY (drk, work, rd.p_x + DECOR(0), rd.p_y - EDGE(-1),
			                     rd.p_x - EDGE(-1), rd.p_y + DECOR(0));
		}
	}
	if (!f) return;
	
	if (!sizeable) {
		DCR_XYXY (wht, work, lu.p_x - DECOR(0), rd.p_y + DECOR(1),
		                     lu.p_x        -3,  lu.p_y           );
		DCR_XYY  (drk, work, lu.p_x        -2,  lu.p_y        +1,
		                                      rd.p_y        +1 );
		DCR_XXY  (wht, work, lu.p_x        -1,
		                     rd.p_x        +2,  rd.p_y        +2 );
		DCR_XYXY (wht, work, rd.p_x        +2,  rd.p_y        +1,
		                     rd.p_x + DECOR(1), lu.p_y           );
		DCR_XYXY (drk, work, rd.p_x + DECOR(0), lu.p_y - DECOR(1),
		                     lu.p_x - DECOR(1), rd.p_y + DECOR(0));
	}
	
	drk->p_y = 0;
	wht->p_y = 0;
	blk->p_y = 0;
	
	vsf_color (GRPH_Vdi, G_LWHITE);
	v_hide_c  (GRPH_Vdi);
	do {
		vs_clip_pxy (GRPH_Vdi, (PXY*)(sect++));
		for (i = 0; i < f; v_bar (GRPH_Vdi, (short*)&frm[i++].lu));
		drk = __d + d;
		if (drk->p_y) {
			vsl_color (GRPH_Vdi, G_LBLACK);
			do {
				v_pline (GRPH_Vdi, drk->p_y, (short*)(drk +1));
				drk += drk->p_y +1;
			} while (drk->p_y);
		}
		wht = __w;
		if (wht->p_y) {
			vsl_color (GRPH_Vdi, G_WHITE);
			do {
				v_pline (GRPH_Vdi, wht->p_y, (short*)(wht +1));
				wht += wht->p_y +1;
			} while (wht->p_y);
		}
		blk = __b;
		if (blk->p_y) {
			vsl_color (GRPH_Vdi, G_BLACK);
			do {
				v_pline (GRPH_Vdi, blk->p_y, (short*)(blk +1));
				blk += blk->p_y +1;
			} while (blk->p_y);
		}
	} while (--num);
	v_show_c    (GRPH_Vdi, 1);
	vs_clip_off (GRPH_Vdi);
}
