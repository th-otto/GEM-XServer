/*
 *==============================================================================
 *
 * main.h
 *
 * Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
 *------------------------------------------------------------------------------
 * 2000-12-14 - Module released for beta state.
 * 2000-05-30 - Initial Version.
 *==============================================================================
 */
#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef NODEBUG
#undef TRACE
#endif

#include <stddef.h>
#include <X11/Xmd.h>
#include <gem.h>


#define RID_MASK_BITS    18
#define RID_MASK         ((1uL << (RID_MASK_BITS)) -1)
#define RID_Base(id)     ((id) >>  RID_MASK_BITS)
#define RID_Match(b,r)   ((b) == RID_Base(r))

#define ROOT_WINDOW     0x8000uL		/* alien AES handles are ORed to this */
#define DFLT_VISUAL     0x0100uL		/* further visuals IDs start from here */
#define DFLT_COLORMAP   0x0200uL

#define PADD_BITS   16

struct s_GC;
typedef struct s_GC *p_GC;
struct s_FONt;
typedef struct s_FONT *p_FONT;
struct s_FONTABLE;
typedef union u_FONTABLE
{
	struct s_FONTABLE *p;
	p_GC Gc;
	p_FONT Font;
} p_FONTABLE;
struct s_PIXMAP;
typedef struct s_PIXMAP *p_PIXMAP;
struct s_WINDOW;
typedef struct s_WINDOW *p_WINDOW;
struct s_DRAWABLE;
typedef union u_DRAWABLE
{
	struct s_DRAWABLE *p;
	p_PIXMAP Pixmap;
	p_WINDOW Window;
} p_DRAWABLE;
struct s_CLIENT;
typedef struct s_CLIENT *p_CLIENT;
struct s_CURSOR;
typedef struct s_CURSOR *p_CURSOR;


extern CARD32 MAIN_TimeStamp;
extern PXY *MAIN_PointerPos;
extern CARD16 MAIN_KeyButMask;

#define             MAIN_Key_Mask (((CARD8*)&MAIN_KeyButMask)[1])
#define             MAIN_But_Mask (((CARD8*)&MAIN_KeyButMask)[0])

extern long MAIN_FDSET_wr;
extern long MAIN_FDSET_rd;

void MainSetMove(BOOL onNoff);
void MainSetWatch(const GRECT *area, BOOL leaveNenter);

#define MainClrWatch() MainSetWatch (NULL, 0)


BOOL WindButton(CARD16 prev_mask, int count);
void WindPointerWatch(BOOL movedNreorg);
void WindPointerMove(const PXY *mouse);


extern short _MAIN_Wupdt;
extern short _MAIN_Mctrl;

static inline void WindUpdate(int onNoff)
{
	if (onNoff)
	{
		wind_update(BEG_UPDATE);
		_MAIN_Wupdt++;
	} else if (_MAIN_Wupdt)
	{
		wind_update(END_UPDATE);
		_MAIN_Wupdt--;
	}
}

static inline void WindMctrl(int onNoff)
{
	if (onNoff)
	{
		wind_update(BEG_MCTRL);
		_MAIN_Mctrl++;
	} else if (_MAIN_Mctrl)
	{
		wind_update(END_MCTRL);
		_MAIN_Mctrl--;
	}
}


/* ___constants_from_config.c___ */

extern CARD32 CNFG_MaxReqLength;		/* length in units (longs) */

#define CNFG_MaxReqBytes (CNFG_MaxReqLength *4)	/* same in bytes */

extern const char PATH_DEBUG_OUT[];
extern const char PATH_XBIN_RSRC[];
extern const char *PATH_RSRC;
extern const char PATH_Xconsole[];
extern const char PATH_Xmodmap[];
extern const char PATH_XmodmapRc[];
extern const char PATH_FontsAlias[];
extern const char PATH_LibDir[];
extern const char PATH_FontsDb[];


#endif /* __MAIN_H__ */
