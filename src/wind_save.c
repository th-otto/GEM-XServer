//==============================================================================
//
// wind_save.c -- Save and restore save-under area.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-03-01 - Initial Version.
//==============================================================================
//
#include <stdlib.h>

#include "window_P.h"
#include "grph.h"
#include "x_gem.h"

#include <X11/Xproto.h>


BOOL         WIND_SaveDone   = xFalse;
PRECT        WIND_SaveArea[3];
CARD32       _WIND_SaveUnder  = 0ul;
static short _WIND_SaveHandle = 0;
static MFDB  _WIND_SaveMfdb   = { NULL, 0,0,0,0,0,0,0,0 };


//==============================================================================
void
WindSaveUnder (CARD32 id, GRECT * rect, short hdl)
{
	size_t width = (rect->g_w +15) /16;
	void * addr  = malloc (width *2 * rect->g_h * GRPH_Depth);
	if (addr) {
		MFDB    scrn = { NULL, };
		PRECT * src  = &WIND_SaveArea[0];
		PRECT * dst  = &WIND_SaveArea[1];
		if (_WIND_SaveMfdb.fd_addr) {
			free (_WIND_SaveMfdb.fd_addr);
		}
		WIND_SaveDone             = xFalse;
		_WIND_SaveUnder           = id;
		_WIND_SaveHandle          = -hdl;
		_WIND_SaveMfdb.fd_addr    = addr;
		_WIND_SaveMfdb.fd_w       = rect->g_w;
		_WIND_SaveMfdb.fd_h       = rect->g_h;
		_WIND_SaveMfdb.fd_wdwidth = width;
		_WIND_SaveMfdb.fd_nplanes = GRPH_Depth;
		src->rd.p_x = (src->lu.p_x = rect->g_x) + (dst->rd.p_x = rect->g_w -1);
		src->rd.p_y = (src->lu.p_y = rect->g_y) + (dst->rd.p_y = rect->g_h -1);
		dst->lu.p_x = 0;
		dst->lu.p_y = 0;
		v_hide_c    (GRPH_Vdi);
		vro_cpyfm (GRPH_Vdi, S_ONLY,
		           (short*)&WIND_SaveArea[0], &scrn, &_WIND_SaveMfdb);
		v_show_c    (GRPH_Vdi, 0);
		WIND_SaveArea[2] = *src;
	}
}

//==============================================================================
void
WindSaveFlush (BOOL restore)
{
	WIND_SaveDone = xFalse;
	
	if (_WIND_SaveMfdb.fd_addr) {
		if (restore) {
			MFDB scrn = { NULL, };
			v_hide_c (GRPH_Vdi);
			if (_WIND_SaveHandle <= 0) {
				vro_cpyfm (GRPH_Vdi, S_ONLY,
				           (short*)&WIND_SaveArea[1], &_WIND_SaveMfdb, &scrn);
				WIND_SaveDone = xTrue;
			} else {
				PRECT * src = &WIND_SaveArea[1];
				PRECT * dst = &WIND_SaveArea[2];
				WIND_UPDATE_BEG;
				wind_get_first (_WIND_SaveHandle, (GRECT*)dst);
				while (dst->rd.p_x > 0  &&  dst->rd.p_y > 0) {
					dst->rd.p_x += dst->lu.p_x -1;
					dst->rd.p_y += dst->lu.p_y -1;
					if (GrphIntersectP (dst, &WIND_SaveArea[0])) {
						src->lu.p_x = dst->lu.p_x - WIND_SaveArea[0].lu.p_x;
						src->lu.p_y = dst->lu.p_y - WIND_SaveArea[0].lu.p_y;
						src->rd.p_x = dst->rd.p_x - WIND_SaveArea[0].lu.p_x;
						src->rd.p_y = dst->rd.p_y - WIND_SaveArea[0].lu.p_y;
						vro_cpyfm (GRPH_Vdi, S_ONLY,
						           (short*)&WIND_SaveArea[1], &_WIND_SaveMfdb, &scrn);
					}
					wind_get_next (_WIND_SaveHandle, (GRECT*)dst);
				}
				WIND_UPDATE_END;
			}
			v_show_c (GRPH_Vdi, 0);
		}
		free (_WIND_SaveMfdb.fd_addr);
		_WIND_SaveMfdb.fd_addr = NULL;
	}
	_WIND_SaveUnder = 0ul;
}
