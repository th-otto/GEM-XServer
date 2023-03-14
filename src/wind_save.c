//==============================================================================
//
// wind_save.c -- Save and restore save-under area.
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-03-01 - Initial Version.
//==============================================================================
//
#include "window_P.h"
#include "grph.h"
#include "x_gem.h"

#include <stdlib.h>

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
	size_t width = (rect->w +15) /16;
	void * addr  = malloc (width *2 * rect->h * GRPH_Depth);
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
		_WIND_SaveMfdb.fd_w       = rect->w;
		_WIND_SaveMfdb.fd_h       = rect->h;
		_WIND_SaveMfdb.fd_wdwidth = width;
		_WIND_SaveMfdb.fd_nplanes = GRPH_Depth;
		src->rd.x = (src->lu.x = rect->x) + (dst->rd.x = rect->w -1);
		src->rd.y = (src->lu.y = rect->y) + (dst->rd.y = rect->h -1);
		dst->lu.x = 0;
		dst->lu.y = 0;
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
				while (dst->rd.x > 0  &&  dst->rd.y > 0) {
					dst->rd.x += dst->lu.x -1;
					dst->rd.y += dst->lu.y -1;
					if (GrphIntersectP (dst, &WIND_SaveArea[0])) {
						src->lu.x = dst->lu.x - WIND_SaveArea[0].lu.x;
						src->lu.y = dst->lu.y - WIND_SaveArea[0].lu.y;
						src->rd.x = dst->rd.x - WIND_SaveArea[0].lu.x;
						src->rd.y = dst->rd.y - WIND_SaveArea[0].lu.y;
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
